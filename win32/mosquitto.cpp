#include "pch.h"
#include "mosquitto.h"

#include <cassert>

/* pub_client.c modes */
#define MSGMODE_NONE 0
#define MSGMODE_CMD 1
#define MSGMODE_STDIN_LINE 2
#define MSGMODE_STDIN_FILE 3
#define MSGMODE_FILE 4
#define MSGMODE_NULL 5

#define PORT_UNDEFINED		-1
#define PORT_UNIX			0

#define MQTT_MAX_TOPICS		1024

static mosq_config mqtt_cfg_pub;
static mosq_config mqtt_cfg_sub;

static int xmqtt_init_config(MemoryPoolContext mempool, struct mosq_config* cfg, int pub_or_sub, int max_topics)
{
	int r = MOSQ_ERR_ERRNO;

	if (mempool && cfg)
	{
		memset(cfg, 0, sizeof(struct mosq_config));
		cfg->port = PORT_UNDEFINED;
		cfg->max_inflight = 20;
		cfg->keepalive = 60;
		cfg->clean_session = true;
		cfg->qos = 1;
		cfg->eol = true;
		cfg->repeat_count = 1;
		cfg->repeat_delay.tv_sec = 0;
		cfg->repeat_delay.tv_usec = 0;
		cfg->random_filter = 10000;
#if 0
		if (pub_or_sub == CLIENT_RR)
		{
			cfg->protocol_version = MQTT_PROTOCOL_V5;
			cfg->msg_count = 1;
		}
		else {
			cfg->protocol_version = MQTT_PROTOCOL_V311;
		}
#endif 
		cfg->protocol_version = MQTT_PROTOCOL_V5;  /* we use V5 as the default protocal */
		cfg->msg_count = 1;
		cfg->session_expiry_interval = -1; /* -1 means unset here, the user can't set it to -1. */

		cfg->maxtopics = max_topics;
		if (max_topics < 1)
			cfg->maxtopics = 1;
		if (max_topics > MQTT_MAX_TOPICS)
			cfg->maxtopics = MQTT_MAX_TOPICS;

		cfg->topics = (char**)palloc(mempool, ((cfg->maxtopics) * sizeof(char*))); // an array of char* pointer
		if (cfg->topics)
			r = MOSQ_ERR_SUCCESS;
		else
			r = MOSQ_ERR_NOMEM;
	}

	return r;
}

static int xmqtt_cfg_add_topic(MemoryPoolContext mempool, struct mosq_config* cfg, int type, char* topic, const char* arg)
{
	int r = MOSQ_ERR_ERRNO;

	if (mempool)
	{
		char* tp = pstrdup(mempool, topic);
		if (!tp)
			return 1;

		if (mosquitto_validate_utf8(topic, (int)strlen(topic)))
		{
			//fprintf(stderr, "Error: Malformed UTF-8 in %s argument.\n\n", arg);
			return 2;
		}
		if (type == CLIENT_PUB || type == CLIENT_RR)
		{
			if (mosquitto_pub_topic_check(topic) == MOSQ_ERR_INVAL)
			{
				//fprintf(stderr, "Error: Invalid publish topic '%s', does it contain '+' or '#'?\n", topic);
				return 3;
			}
			cfg->topic = tp;
		}
		else if (type == CLIENT_RESPONSE_TOPIC)
		{
			if (mosquitto_pub_topic_check(topic) == MOSQ_ERR_INVAL)
			{
				//fprintf(stderr, "Error: Invalid response topic '%s', does it contain '+' or '#'?\n", topic);
				return 1;
			}
			cfg->response_topic = tp;
		}
		else
		{
			if (mosquitto_sub_topic_check(topic) == MOSQ_ERR_INVAL)
			{
				//fprintf(stderr, "Error: Invalid subscription topic '%s', are all '+' and '#' wildcards correct?\n", topic);
				return 1;
			}
			if (cfg->topic_count >= MQTT_MAX_TOPICS)
			{
				// we reached the maximum topics
				return 1;
			}
			cfg->topic_count++;
			cfg->topics[cfg->topic_count - 1] = tp;
		}
		r = MOSQ_ERR_SUCCESS;
	}

	return r;
}

static int xmqtt_client_set_config(MemoryPoolContext mempool, struct mosq_config* cfg, int pub_or_sub, char* host, int port, char* topic, int max_topic)
{
	int r = MOSQ_ERR_ERRNO;

	if (mempool && cfg && host && topic)
	{
		int rc;
		rc = xmqtt_init_config(mempool, cfg, pub_or_sub, max_topic);
		if (MOSQ_ERR_SUCCESS != rc)
			return 1;

		rc = xmqtt_cfg_add_topic(mempool, cfg, pub_or_sub, topic, nullptr);
		if (MOSQ_ERR_SUCCESS != rc)
			return 2;

		cfg->host = host;
		cfg->port = port;

		/* Deal with real argc/argv */
		if (cfg->will_payload && !cfg->will_topic)
		{
			//fprintf(stderr, "Error: Will payload given, but no will topic given.\n");
			return 3;
		}
		if (cfg->will_retain && !cfg->will_topic)
		{
			//fprintf(stderr, "Error: Will retain given, but no will topic given.\n");
			return 4;
		}

		if (cfg->protocol_version == 5)
		{
			if (cfg->clean_session == false && cfg->session_expiry_interval == -1)
			{
				/* User hasn't set session-expiry-interval, but has cleared clean
					session so default to persistent session. */
				cfg->session_expiry_interval = UINT32_MAX;
			}
			if (cfg->session_expiry_interval > 0)
			{
				if (cfg->session_expiry_interval == UINT32_MAX && (cfg->id_prefix || !cfg->id))
				{
					//fprintf(stderr, "Error: You must provide a client id if you are using an infinite session expiry interval.\n");
					return 5;
				}
				rc = mosquitto_property_add_int32(&cfg->connect_props, MQTT_PROP_SESSION_EXPIRY_INTERVAL, (uint32_t)cfg->session_expiry_interval);
				if (rc)
				{
					//fprintf(stderr, "Error adding property session-expiry-interval\n");
				}
			}
		}
		else
		{
			if (cfg->clean_session == false && (cfg->id_prefix || !cfg->id))
			{
				//fprintf(stderr, "Error: You must provide a client id if you are using the -c option.\n");
				return 6;
			}
		}

		if (pub_or_sub == CLIENT_SUB)
		{
			if (cfg->topic_count == 0)
			{
				//fprintf(stderr, "Error: You must specify a topic to subscribe to.\n");
				return 7;
			}
		}

		if (!cfg->host)
		{
			cfg->host = (char*)"www.boobooke.com";
		}

		rc = mosquitto_property_check_all(CMD_CONNECT, cfg->connect_props);
		if (rc)
		{
			//err_printf(cfg, "Error in CONNECT properties: %s\n", mosquitto_strerror(rc));
			return 1;
		}
		rc = mosquitto_property_check_all(CMD_PUBLISH, cfg->publish_props);
		if (rc)
		{
			//err_printf(cfg, "Error in PUBLISH properties: %s\n", mosquitto_strerror(rc));
			return 1;
		}
		rc = mosquitto_property_check_all(CMD_SUBSCRIBE, cfg->subscribe_props);
		if (rc)
		{
			//err_printf(cfg, "Error in SUBSCRIBE properties: %s\n", mosquitto_strerror(rc));
			return 1;
		}
		rc = mosquitto_property_check_all(CMD_UNSUBSCRIBE, cfg->unsubscribe_props);
		if (rc)
		{
			//err_printf(cfg, "Error in UNSUBSCRIBE properties: %s\n", mosquitto_strerror(rc));
			return 1;
		}
		rc = mosquitto_property_check_all(CMD_DISCONNECT, cfg->disconnect_props);
		if (rc)
		{
			//err_printf(cfg, "Error in DISCONNECT properties: %s\n", mosquitto_strerror(rc));
			return 1;
		}
		rc = mosquitto_property_check_all(CMD_WILL, cfg->will_props);
		if (rc)
		{
			//err_printf(cfg, "Error in Will properties: %s\n", mosquitto_strerror(rc));
			return 1;
		}
		r = MOSQ_ERR_SUCCESS;
	}

	return r;
}

// generate a random 22 bytes as the client id because MQTT only allow maxium 23 bytes as the client id
static int xmqtt_client_id_generate(MemoryPoolContext mempool, struct mosq_config* cfg)
{
	int r = MOSQ_ERR_ERRNO;
	NTSTATUS status;
	U8 randomize[11] = { 0 };

	status = BCryptGenRandom(NULL, randomize, 11, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

	if (STATUS_SUCCESS == status)
	{
		cfg->id = (char*)palloc(mempool, 23);
		if (cfg->id)
		{
			U8 i, idx;
			const U8* hex_chars = (const U8*)"0123456789ABCDEF";
			for (i = 0; i < 11; i++)
			{
				idx = ((randomize[i] >> 4) & 0x0F);
				cfg->id[(i << 1)] = hex_chars[idx];

				idx = (randomize[i] & 0x0F);
				cfg->id[(i << 1) + 1] = hex_chars[idx];
			}
			cfg->id[22] = '\0';
			r = MOSQ_ERR_SUCCESS;
		}
	}
	return r;
}

static int xmqtt_client_opts_set(struct mosquitto* mosq, struct mosq_config* cfg)
{
#if defined(WITH_TLS) || defined(WITH_SOCKS)
	int rc;
#endif

	mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, cfg->protocol_version);

	if (cfg->will_topic && mosquitto_will_set_v5(mosq, cfg->will_topic,
		cfg->will_payloadlen, cfg->will_payload, cfg->will_qos,
		cfg->will_retain, cfg->will_props))
	{
		//err_printf(cfg, "Error: Problem setting will.\n");
		return 1;
	}
	cfg->will_props = NULL;

	if ((cfg->username || cfg->password) && mosquitto_username_pw_set(mosq, cfg->username, cfg->password))
	{
		//err_printf(cfg, "Error: Problem setting username and/or password.\n");
		return 1;
	}
	mosquitto_max_inflight_messages_set(mosq, cfg->max_inflight);

#ifdef WITH_SOCKS
	if (cfg->socks5_host)
	{
		rc = mosquitto_socks5_set(mosq, cfg->socks5_host, cfg->socks5_port, cfg->socks5_username, cfg->socks5_password);
		if (rc) {
			mosquitto_lib_cleanup();
			return rc;
		}
	}
#endif
	if (cfg->tcp_nodelay)
	{
		mosquitto_int_option(mosq, MOSQ_OPT_TCP_NODELAY, 1);
	}

	if (cfg->msg_count > 0 && cfg->msg_count < 20)
	{
		/* 20 is the default "receive maximum"
			* If we don't set this, then we can receive > msg_count messages
			* before we quit.*/
		mosquitto_int_option(mosq, MOSQ_OPT_RECEIVE_MAXIMUM, cfg->msg_count);
	}
	return MOSQ_ERR_SUCCESS;
}

static int xmqtt_client_connect(struct mosquitto* mosq, struct mosq_config* cfg)
{
#ifndef WIN32
	char* err;
#else
	char err[1024];
#endif
	int rc;
	int port;

	if (cfg->port == PORT_UNDEFINED)
	{
		port = 1883;
	}
	else
	{
		port = cfg->port;
	}

#ifdef WITH_SRV
	if (cfg->use_srv) {
		rc = mosquitto_connect_srv(mosq, cfg->host, cfg->keepalive, cfg->bind_address);
	}
	else {
		rc = mosquitto_connect_bind_v5(mosq, cfg->host, port, cfg->keepalive, cfg->bind_address, cfg->connect_props);
	}
#else
	rc = mosquitto_connect_bind_v5(mosq, cfg->host, port, cfg->keepalive, cfg->bind_address, cfg->connect_props);
#endif
	if (rc > 0)
	{
		if (rc == MOSQ_ERR_ERRNO)
		{
#ifndef WIN32
			err = strerror(errno);
#else
			//	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errno, 0, (LPTSTR)&err, 1024, NULL);
#endif
			//err_printf(cfg, "Error: %s\n", err);
		}
		else
		{
			//err_printf(cfg, "Unable to connect (%s).\n", mosquitto_strerror(rc));
		}
		return rc;
	}
	return MOSQ_ERR_SUCCESS;
}

namespace MQTT
{
	int MQTT_Init()
	{
		return mosquitto_lib_init();
	}

	int MQTT_Term()
	{
		return mosquitto_lib_cleanup();
	}

	int MQTT_AddSubTopic(MemoryPoolContext mempool, int type, char* topic)
	{
		int ret = xmqtt_cfg_add_topic(mempool, &mqtt_cfg_sub, type, topic, NULL);
		return ret;
	}

	int MQTT_SubLoop(struct mosquitto* q, LONG* signal)
	{
		int ret;

		assert(q);

		ret = xmqtt_client_connect(q, &mqtt_cfg_sub);
		if (MOSQ_ERR_SUCCESS != ret)
		{
			return 1;
		}
		// Main Loop
		do
		{
			ret = mosquitto_loop(q, -1, 1);
			Sleep(1000);

		} while ((0 == *signal) && (MOSQ_ERR_SUCCESS == ret));

		mosquitto_disconnect_v5(q, 0, mqtt_cfg_sub.disconnect_props);

		return MOSQ_ERR_SUCCESS;
	}

	int MQTT_SubTerm(Mosquitto q)
	{
		if (nullptr != q)
		{
			mosquitto_destroy(q);
		}
		return MOSQ_ERR_SUCCESS;
	}

	Mosquitto MQTT_SubInit(MQTTPrivateData* privatedata, char* host, int port, MQTT_Methods* callback)
	{
		int ret;
		struct mosquitto* pMOSQ = NULL;

		assert(privatedata);

		MQTT_Methods* m = callback;
		assert(nullptr != m);

		ret = xmqtt_client_set_config(privatedata->mempool, &mqtt_cfg_sub, CLIENT_SUB, host, port, (char*)"WOCHAT", MQTT_MAX_TOPICS);
		if (MOSQ_ERR_SUCCESS != ret)
			return nullptr;

		if (mqtt_cfg_sub.no_retain && mqtt_cfg_sub.retained_only)
		{
			//fprintf(stderr, "\nError: Combining '-R' and '--retained-only' makes no sense.\n");
			return nullptr;
		}

		xmqtt_client_id_generate(privatedata->mempool, &mqtt_cfg_sub);
		mqtt_cfg_sub.userdata = privatedata;

		pMOSQ = mosquitto_new(mqtt_cfg_sub.id, mqtt_cfg_sub.clean_session, &mqtt_cfg_sub);
		if (nullptr == pMOSQ)
		{
			return nullptr;
		}

		ret = xmqtt_client_opts_set(pMOSQ, &mqtt_cfg_sub);
		if (MOSQ_ERR_SUCCESS != ret)
		{
			mosquitto_destroy(pMOSQ);
			return nullptr;
		}

		if (mqtt_cfg_sub.debug)
		{
			mosquitto_log_callback_set(pMOSQ, m->log_callback);
		}

		mosquitto_subscribe_callback_set(pMOSQ, m->subscribe_callback);
		mosquitto_connect_v5_callback_set(pMOSQ, m->connect_callback);
		mosquitto_message_v5_callback_set(pMOSQ, m->message_callback);

		return pMOSQ;
	}

	Mosquitto MQTT_PubInit(MQTTPrivateData* pd, char* host, int port, MQTT_Methods* callback)
	{
		int ret;
		struct mosquitto* pMOSQ = NULL;

		assert(pd);

		MQTT_Methods* m = callback;
		assert(nullptr != m);

		ret = xmqtt_client_set_config(pd->mempool, &mqtt_cfg_pub, CLIENT_PUB, host, port, (char*)"WOCHAT", 1);
		if (MOSQ_ERR_SUCCESS != ret)
			return nullptr;

		xmqtt_client_id_generate(pd->mempool, &mqtt_cfg_pub);
		mqtt_cfg_pub.userdata = pd;

		pMOSQ = mosquitto_new(mqtt_cfg_pub.id, mqtt_cfg_pub.clean_session, &mqtt_cfg_pub);
		if (nullptr == pMOSQ)
		{
			return nullptr;
		}

		ret = xmqtt_client_opts_set(pMOSQ, &mqtt_cfg_pub);
		if (MOSQ_ERR_SUCCESS != ret)
		{
			mosquitto_destroy(pMOSQ);
			return nullptr;
		}

		if (mqtt_cfg_pub.debug)
		{
			mosquitto_log_callback_set(pMOSQ, m->log_callback);
		}

		mosquitto_connect_v5_callback_set(pMOSQ, m->connect_callback);
		mosquitto_disconnect_v5_callback_set(pMOSQ, m->disconnect_callback);
		mosquitto_publish_v5_callback_set(pMOSQ, m->publish_callback);

		return pMOSQ;
	}

	int MQTT_PubTerm(Mosquitto q)
	{
		if (nullptr != q)
		{
			mosquitto_destroy(q);
		}
		return MOSQ_ERR_SUCCESS;
	}

	int MQTT_PubMessage(Mosquitto q, char* topic, char* message, int msglen)
	{
		int ret, mid;
		ret = xmqtt_client_connect(q, &mqtt_cfg_pub);

		if (MOSQ_ERR_SUCCESS != ret)
			return MOSQ_ERR_NO_CONN;

		do
		{
			ret = mosquitto_loop(q, -1, 1);
			ret = mosquitto_publish_v5(q, &mid, (const char*)topic, msglen, message, mqtt_cfg_pub.qos, 0, mqtt_cfg_pub.publish_props);

		} while (0);

		mosquitto_disconnect_v5(q, 0, mqtt_cfg_pub.disconnect_props);

		return MOSQ_ERR_SUCCESS;
	}

#if 0
	static void on_connect_wrapper(struct mosquitto* mosq, void* userdata, int rc)
	{
		class XMqtt* m = (class XMqtt*)userdata;

		UNUSED(mosq);

		m->on_connect(rc);
	}

	static void on_connect_with_flags_wrapper(struct mosquitto* mosq, void* userdata, int rc, int flags)
	{
		class XMqtt* m = (class XMqtt*)userdata;
		UNUSED(mosq);
		m->on_connect_with_flags(rc, flags);
	}

	static void on_disconnect_wrapper(struct mosquitto* mosq, void* userdata, int rc)
	{
		class XMqtt* m = (class XMqtt*)userdata;
		UNUSED(mosq);
		m->on_disconnect(rc);
	}

	static void on_publish_wrapper(struct mosquitto* mosq, void* userdata, int mid)
	{
		class XMqtt* m = (class XMqtt*)userdata;
		UNUSED(mosq);
		m->on_publish(mid);
	}

	static void on_message_wrapper(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message)
	{
		class XMqtt* m = (class XMqtt*)userdata;
		UNUSED(mosq);
		m->on_message(message);
	}

	static void on_subscribe_wrapper(struct mosquitto* mosq, void* userdata, int mid, int qos_count, const int* granted_qos)
	{
		class XMqtt* m = (class XMqtt*)userdata;
		UNUSED(mosq);
		m->on_subscribe(mid, qos_count, granted_qos);
	}

	static void on_unsubscribe_wrapper(struct mosquitto* mosq, void* userdata, int mid)
	{
		class XMqtt* m = (class XMqtt*)userdata;
		UNUSED(mosq);
		m->on_unsubscribe(mid);
	}


	static void on_log_wrapper(struct mosquitto* mosq, void* userdata, int level, const char* str)
	{
		class XMqtt* m = (class XMqtt*)userdata;
		UNUSED(mosq);
		m->on_log(level, str);
	}
#endif
	int lib_version(int* major, int* minor, int* revision)
	{
		if (major) *major = LIBMOSQUITTO_MAJOR;
		if (minor) *minor = LIBMOSQUITTO_MINOR;
		if (revision) *revision = LIBMOSQUITTO_REVISION;
		return LIBMOSQUITTO_VERSION_NUMBER;
	}

	const char* strerror(int mosq_errno)
	{
		return mosquitto_strerror(mosq_errno);
	}

	const char* connack_string(int connack_code)
	{
		return mosquitto_connack_string(connack_code);
	}

	int sub_topic_tokenise(const char* subtopic, char*** topics, int* count)
	{
		return mosquitto_sub_topic_tokenise(subtopic, topics, count);
	}

	int sub_topic_tokens_free(char*** topics, int count)
	{
		return mosquitto_sub_topic_tokens_free(topics, count);
	}

	int topic_matches_sub(const char* sub, const char* topic, bool* result)
	{
		return mosquitto_topic_matches_sub(sub, topic, result);
	}

	int validate_utf8(const char* str, int len)
	{
		return mosquitto_validate_utf8(str, len);
	}

	int subscribe_simple(
		struct mosquitto_message** messages,
		int msg_count,
		bool retained,
		const char* topic,
		int qos,
		const char* host,
		int port,
		const char* client_id,
		int keepalive,
		bool clean_session,
		const char* username,
		const char* password,
		const struct libmosquitto_will* will,
		const struct libmosquitto_tls* tls)
	{
		return mosquitto_subscribe_simple(
			messages, msg_count, retained,
			topic, qos,
			host, port, client_id, keepalive, clean_session,
			username, password,
			will, tls);
	}

	MQTT_EXPORT int subscribe_callback(
		int (*callback)(struct mosquitto*, void*, const struct mosquitto_message*),
		void* userdata,
		const char* topic,
		int qos,
		const char* host,
		int port,
		const char* client_id,
		int keepalive,
		bool clean_session,
		const char* username,
		const char* password,
		const struct libmosquitto_will* will,
		const struct libmosquitto_tls* tls)
	{
		return mosquitto_subscribe_callback(
			callback, userdata,
			topic, qos,
			host, port, client_id, keepalive, clean_session,
			username, password,
			will, tls);
	}

#if 0
	XMqtt::XMqtt(const char* id, bool clean_session)
	{
		m_mosq = mosquitto_new(id, clean_session, this);
		mosquitto_connect_callback_set(m_mosq, on_connect_wrapper);
		mosquitto_connect_with_flags_callback_set(m_mosq, on_connect_with_flags_wrapper);
		mosquitto_disconnect_callback_set(m_mosq, on_disconnect_wrapper);
		mosquitto_publish_callback_set(m_mosq, on_publish_wrapper);
		mosquitto_message_callback_set(m_mosq, on_message_wrapper);
		mosquitto_subscribe_callback_set(m_mosq, on_subscribe_wrapper);
		mosquitto_unsubscribe_callback_set(m_mosq, on_unsubscribe_wrapper);
		mosquitto_log_callback_set(m_mosq, on_log_wrapper);
	}

	XMqtt::~XMqtt()
	{
		mosquitto_destroy(m_mosq);
	}

	int XMqtt::reinitialise(const char* id, bool clean_session)
	{
		int rc;
		rc = mosquitto_reinitialise(m_mosq, id, clean_session, this);
		if (rc == MOSQ_ERR_SUCCESS) {
			mosquitto_connect_callback_set(m_mosq, on_connect_wrapper);
			mosquitto_connect_with_flags_callback_set(m_mosq, on_connect_with_flags_wrapper);
			mosquitto_disconnect_callback_set(m_mosq, on_disconnect_wrapper);
			mosquitto_publish_callback_set(m_mosq, on_publish_wrapper);
			mosquitto_message_callback_set(m_mosq, on_message_wrapper);
			mosquitto_subscribe_callback_set(m_mosq, on_subscribe_wrapper);
			mosquitto_unsubscribe_callback_set(m_mosq, on_unsubscribe_wrapper);
			mosquitto_log_callback_set(m_mosq, on_log_wrapper);
		}
		return rc;
	}

	int XMqtt::connect(const char* host, int port, int keepalive)
	{
		return mosquitto_connect(m_mosq, host, port, keepalive);
	}

	int XMqtt::connect(const char* host, int port, int keepalive, const char* bind_address)
	{
		return mosquitto_connect_bind(m_mosq, host, port, keepalive, bind_address);
	}

	int XMqtt::connect_async(const char* host, int port, int keepalive)
	{
		return mosquitto_connect_async(m_mosq, host, port, keepalive);
	}

	int XMqtt::connect_async(const char* host, int port, int keepalive, const char* bind_address)
	{
		return mosquitto_connect_bind_async(m_mosq, host, port, keepalive, bind_address);
	}

	int XMqtt::reconnect()
	{
		return mosquitto_reconnect(m_mosq);
	}

	int XMqtt::reconnect_async()
	{
		return mosquitto_reconnect_async(m_mosq);
	}

	int XMqtt::disconnect()
	{
		return mosquitto_disconnect(m_mosq);
	}

	int XMqtt::socket()
	{
		return mosquitto_socket(m_mosq);
	}

	int XMqtt::will_set(const char* topic, int payloadlen, const void* payload, int qos, bool retain)
	{
		return mosquitto_will_set(m_mosq, topic, payloadlen, payload, qos, retain);
	}

	int XMqtt::will_clear()
	{
		return mosquitto_will_clear(m_mosq);
	}

	int XMqtt::username_pw_set(const char* username, const char* password)
	{
		return mosquitto_username_pw_set(m_mosq, username, password);
	}

	int XMqtt::publish(int* mid, const char* topic, int payloadlen, const void* payload, int qos, bool retain)
	{
		return mosquitto_publish(m_mosq, mid, topic, payloadlen, payload, qos, retain);
	}

	void XMqtt::reconnect_delay_set(unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff)
	{
		mosquitto_reconnect_delay_set(m_mosq, reconnect_delay, reconnect_delay_max, reconnect_exponential_backoff);
	}

	int XMqtt::max_inflight_messages_set(unsigned int max_inflight_messages)
	{
		return mosquitto_max_inflight_messages_set(m_mosq, max_inflight_messages);
	}

	void XMqtt::message_retry_set(unsigned int message_retry)
	{
		mosquitto_message_retry_set(m_mosq, message_retry);
	}

	int XMqtt::subscribe(int* mid, const char* sub, int qos)
	{
		return mosquitto_subscribe(m_mosq, mid, sub, qos);
	}

	int XMqtt::unsubscribe(int* mid, const char* sub)
	{
		return mosquitto_unsubscribe(m_mosq, mid, sub);
	}

	int XMqtt::loop(int timeout, int max_packets)
	{
		return mosquitto_loop(m_mosq, timeout, max_packets);
	}

	int XMqtt::loop_misc()
	{
		return mosquitto_loop_misc(m_mosq);
	}

	int XMqtt::loop_read(int max_packets)
	{
		return mosquitto_loop_read(m_mosq, max_packets);
	}

	int XMqtt::loop_write(int max_packets)
	{
		return mosquitto_loop_write(m_mosq, max_packets);
	}

	int XMqtt::loop_forever(int timeout, int max_packets)
	{
		return mosquitto_loop_forever(m_mosq, timeout, max_packets);
	}

	int XMqtt::loop_start()
	{
		return mosquitto_loop_start(m_mosq);
	}

	int XMqtt::loop_stop(bool force)
	{
		return mosquitto_loop_stop(m_mosq, force);
	}

	bool XMqtt::want_write()
	{
		return mosquitto_want_write(m_mosq);
	}

	int XMqtt::opts_set(enum mosq_opt_t option, void* value)
	{
		return mosquitto_opts_set(m_mosq, option, value);
	}

	int XMqtt::threaded_set(bool threaded)
	{
		return mosquitto_threaded_set(m_mosq, threaded);
	}

	void XMqtt::user_data_set(void* userdata)
	{
		mosquitto_user_data_set(m_mosq, userdata);
	}

	int XMqtt::socks5_set(const char* host, int port, const char* username, const char* password)
	{
		return mosquitto_socks5_set(m_mosq, host, port, username, password);
	}


	int XMqtt::tls_set(const char* cafile, const char* capath, const char* certfile, const char* keyfile, int (*pw_callback)(char* buf, int size, int rwflag, void* userdata))
	{
		return mosquitto_tls_set(m_mosq, cafile, capath, certfile, keyfile, pw_callback);
	}

	int XMqtt::tls_opts_set(int cert_reqs, const char* tls_version, const char* ciphers)
	{
		return mosquitto_tls_opts_set(m_mosq, cert_reqs, tls_version, ciphers);
	}

	int XMqtt::tls_insecure_set(bool value)
	{
		return mosquitto_tls_insecure_set(m_mosq, value);
	}

	int XMqtt::tls_psk_set(const char* psk, const char* identity, const char* ciphers)
	{
		return mosquitto_tls_psk_set(m_mosq, psk, identity, ciphers);
	}
#endif 
}

