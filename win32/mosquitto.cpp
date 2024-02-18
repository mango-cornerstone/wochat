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

static MQTTConf mqtt_cfg_pub = { 0 };
static MQTTConf mqtt_cfg_sub = { 0 };

static int MQTTConfiugrationInit(MQTTConf* cfg, bool isPub)
{
	int r = MOSQ_ERR_NOMEM;
	MemoryPoolContext ctx;
	
	assert(cfg);

	if(isPub)
		ctx = wt_mempool_create("PubConfiguration", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	else
		ctx = wt_mempool_create("SubConfiguration", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);

	if (ctx)
	{
		memset(cfg, 0, sizeof(MQTTConf));
		cfg->pub_or_sub = isPub;
		cfg->ctx = ctx;
		cfg->port = MQTT_DEFAULT_PORT;
		cfg->max_inflight = 20;
		cfg->keepalive = 60;
		cfg->clean_session = true;
		cfg->qos = 1;
		cfg->eol = true;
		cfg->repeat_count = 1;
		cfg->repeat_delay.tv_sec = 0;
		cfg->repeat_delay.tv_usec = 0;
		cfg->random_filter = 10000;
		cfg->protocol_version = MQTT_PROTOCOL_V5;  /* we use V5 as the default protocal */
		cfg->session_expiry_interval = -1; /* -1 means unset here, the user can't set it to -1. */
		if (!isPub)
		{
			cfg->maxtopics = MQTT_MAX_TOPICS;
			cfg->topics = (char**)wt_palloc(ctx, ((cfg->maxtopics) * sizeof(char*))); // an array of char* pointer
			if (cfg->topics)
				r = MOSQ_ERR_SUCCESS;
		}
		else
			r = MOSQ_ERR_SUCCESS;
	}
	return r;
}

static void MQTTConfiugrationTerm(MQTTConf* cfg)
{
	assert(cfg);
	wt_mempool_destroy(cfg->ctx);
	memset(cfg, 0, sizeof(MQTTConf));
}

namespace MQTT
{
	int MQTT_Init()
	{
		int r0 = MQTTConfiugrationInit(&mqtt_cfg_pub, true);
		int r1 = MQTTConfiugrationInit(&mqtt_cfg_sub, false);
		int r2 = mosquitto_lib_init();
		int r = r0 + r1 + r2;
		return r;
	}

	int MQTT_Term()
	{
		MQTTConfiugrationTerm(&mqtt_cfg_pub);
		MQTTConfiugrationTerm(&mqtt_cfg_sub);
		return mosquitto_lib_cleanup();
	}

	int MQTT_AddSubTopic(char* topic, bool isPub)
	{
		int ret = 1; // xmqtt_cfg_add_topic(mempool, &mqtt_cfg_sub, type, topic, NULL);
		MQTTConf* cfg = isPub ? &mqtt_cfg_pub : &mqtt_cfg_sub;

		assert(cfg);
		assert(cfg->ctx);

		if (wt_IsAlphabetString((U8*)topic, strlen(topic)))
		{
			char* tp = wt_pstrdup(cfg->ctx, topic);
			if (tp)
			{
				if (cfg->topic_count >= MQTT_MAX_TOPICS)
				{
					// we reached the maximum topics
					wt_pfree(tp);
					return 1;
				}
				cfg->topic_count++;
				cfg->topics[cfg->topic_count - 1] = tp;
				ret = 0;
			}
		}
		return ret;
	}

	void MQTT_DestroyInstance(struct mosquitto* mq)
	{
		if (nullptr != mq)
		{
			mosquitto_destroy(mq);
		}
	}

	struct mosquitto* MQTT_CreateInstance(MemoryPoolContext ctxThread, void* hWnd, char* client_id, char* host, int port, MQTT_Methods* callback, bool isPub)
	{
		int ret;
		struct mosquitto* mosq = NULL;
		MQTTConf* cfg = isPub ? &mqtt_cfg_pub : &mqtt_cfg_sub;
		
		assert(cfg);
		assert(callback);

		cfg->ctxThread = ctxThread;
		cfg->hWnd = hWnd;
		cfg->id = client_id;
		cfg->host = host;
		cfg->port = port;

		mosq = mosquitto_new((const char*)(cfg->id), false, cfg);
		if (mosq)
		{
			//mosquitto_subscribe_callback_set(mosq, callback->subscribe_callback);
			mosquitto_connect_v5_callback_set(mosq, callback->connect_callback);
			mosquitto_message_v5_callback_set(mosq, callback->message_callback);
			if (cfg->debug)
			{
				mosquitto_log_callback_set(mosq, callback->log_callback);
			}
		}
		return mosq;
	}

#if 0
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

	int subscribe_callback(
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

