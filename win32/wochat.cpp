#include "pch.h"
#include "mosquitto.h"
#include "wochat.h"
#include "wt_base64.h"
#include "wt_utils.h"
#include "wt_sha256.h"
#include "wt_chacha20.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"

extern "C"
{
	static void MQTT_Message_Callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message, const mosquitto_property* properties);
	static void MQTT_Connect_Callback(struct mosquitto* mosq, void* obj, int result, int flags, const mosquitto_property* properties);
	static void MQTT_Disconnect_Callback(struct mosquitto* mosq, void* obj, int result, const mosquitto_property* properties);
	static void MQTT_Publish_Callback(struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* properties);
	static void MQTT_Subscribe_Callback(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos);
	static void MQTT_Log_Callback(struct mosquitto* mosq, void* obj, int level, const char* str);

	static MQTT_Methods mqtt_callback =
	{
		MQTT_Message_Callback,
		MQTT_Connect_Callback,
		MQTT_Disconnect_Callback,
		MQTT_Subscribe_Callback,
		MQTT_Publish_Callback,
		MQTT_Log_Callback
	};
}

DWORD WINAPI MQTTPubThread(LPVOID lpData)
{
	int rc = 0;
	DWORD dwRet;
	HWND hWndUI;
	struct mosquitto* mosq = nullptr;
	MemoryPoolContext mempool;
	MessageTask* mt;
	MessageTask* p;

	InterlockedIncrement(&g_threadCount);
	hWndUI = (HWND)(lpData);
	ATLASSERT(::IsWindow(hWndUI));

	mempool = wt_mempool_create("MQTT_PUB_POOL", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	if (nullptr == mempool)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 2, 0);
		goto QuitMQTTPubThread;
	}

	mosq = MQTT::MQTT_CreateInstance(mempool, hWndUI, (char*)g_MQTTPubClientId, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, &mqtt_callback, true);

	if (nullptr == mosq)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 1, 0);
		goto QuitMQTTPubThread;
	}

	while (true)
	{
		dwRet = WaitForSingleObject(g_MQTTPubEvent, 10000); // wakeup every 10 seconds

		if (g_Quit) // we will quit the whole program
			break;

		if (WAIT_TIMEOUT == dwRet)
		{
			SendPingMessage(mosq); // send a ping message to check the network is good or not
			continue;
		}

		if (dwRet != WAIT_OBJECT_0)
			continue;

		rc = mosquitto_connect_bind_v5(mosq, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, 60, NULL, NULL);
		if (MOSQ_ERR_SUCCESS != rc)
		{
			InterlockedExchange(&g_NetworkStatus, 0); // if we cannot connect, the network is not good
			continue;
		}

		InterlockedExchange(&g_NetworkStatus, 1);

		rc = mosquitto_publish_v5(mosq, NULL, (const char*)"WOCHAT", 6, "HelloX", 1, false, NULL);

		mosquitto_disconnect_v5(mosq, 0, NULL);
#if 0
		mt = CloneInputMessageTask(mempool); // we clone the message queue so that we do not block the UI thread too long time
		p = mt;
		while (p)
		{
			msglen = p->msgLen * sizeof(wchar_t);
			output_len = wt_b64_enc_len(msglen + 4 + 32) + 1;
			msgbody = (U8*)wt_palloc(mempool, 67 + output_len);

			if (msgbody)
			{
				U8* MSG = (U8*)wt_palloc(mempool, msglen + 4 + 32);
				if (MSG)
				{
					wt_Raw2HexString(p->pubkey, 33, (U8*)topic, nullptr);
					wt_Raw2HexString(g_PK, 33, msgbody, nullptr);
					msgbody[66] = '|';

					U32* pLen = (U32*)(MSG);
					*pLen = (U32)msglen;

					{
						wt_sha256_ctx ctx = { 0 };
						wt_sha256_init(&ctx);
						wt_sha256_update(&ctx, (const unsigned char*)p->message, msglen);
						wt_sha256_final(&ctx, MSG + 4);
					}

					{
						int m;
						U8 Key[32] = { 0 };
						U8 nonce[12];
						GetKeyFromSKAndPK(g_SK, p->pubkey, Key);
						wt_chacha20_context cxt;
						wt_chacha20_init(&cxt);
						m = wt_chacha20_setkey(&cxt, Key);
						for (int i = 0; i < 12; i++)
							nonce[i] = i;
						m = wt_chacha20_starts(&cxt, nonce, 0);
						m = wt_chacha20_update(&cxt, 4 + 32, (const unsigned char*)MSG, MSG);
						m = wt_chacha20_update(&cxt, msglen, (const unsigned char*)p->message, MSG + 4 + 32);
						wt_chacha20_free(&cxt);
					}
					wt_b64_encode((const char*)MSG, (msglen + 4 + 32), (char*)msgbody + 67, output_len);
					msgbody[67 + output_len - 1] = 0; // zero-terminiated
					wt_pfree(MSG);
				}

				MQTT::MQTT_PubMessage(mq, topic, (char*)msgbody, 67 + output_len);
				InterlockedIncrement(&(p->state)); // we have processed this task.

				wt_pfree(msgbody);
				msgbody = nullptr;
			}
			p = p->next;
		}

		UpdateInputMessageTask(mt); // update the input message queue
		wt_mempool_reset(mempool); // release all memory in this memory pool
#endif
	}

QuitMQTTPubThread:
	MQTT::MQTT_DestroyInstance(mosq);
	wt_mempool_destroy(mempool);
	InterlockedDecrement(&g_threadCount);

	return 0;
}

DWORD WINAPI MQTTSubThread(LPVOID lpData)
{
	int ret = 0;
	struct mosquitto* mosq = nullptr;
	HWND hWndUI;
	MemoryPoolContext mempool;
	U8 topic[67];

	InterlockedIncrement(&g_threadCount);

	hWndUI = (HWND)(lpData);
	ATLASSERT(::IsWindow(hWndUI));

	mempool = wt_mempool_create("MQTT_SUB_POOL", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	if (nullptr == mempool)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 2, 0);
		goto QuitMQTTSubThread;
	}

	mosq = MQTT::MQTT_CreateInstance(mempool, hWndUI, (char*)g_MQTTSubClientId, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, &mqtt_callback, false);

	if (nullptr == mosq) // if we cannot create a MQTT instance, quit this thread
	{
		PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 1, 0);
		goto QuitMQTTSubThread;
	}

	wt_Raw2HexString(g_PK, 33, topic, nullptr);
	ret = MQTT::MQTT_AddSubTopic((char*)topic);

	if(ret)
	{
		PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 2, 0);
		goto QuitMQTTSubThread;
	}

	while (0 == g_Quit)
	{
		ret = mosquitto_connect_bind_v5(mosq, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, 10, NULL, NULL);
		if (MOSQ_ERR_SUCCESS == ret)
		{
			InterlockedExchange(&g_NetworkStatus, 1);
			break;
		}
		else
			InterlockedExchange(&g_NetworkStatus, 0); // if we cannot connect, the network is not good
	}

	while (0 == g_Quit) // the main loop
	{
		for (;;)
		{
			if (g_Quit)
				break;
			ret = mosquitto_loop(mosq, -1, 1);
			if (ret != MOSQ_ERR_SUCCESS)
				break;
		}

		if (g_Quit)
			break;

		do 
		{
			ret = mosquitto_reconnect(mosq);
		} while (0 == g_Quit && ret != MOSQ_ERR_SUCCESS);

		if (g_Quit)
			break;

		if (MOSQ_ERR_SUCCESS == ret)  // because we can connect successfully, the network is good
			InterlockedExchange(&g_NetworkStatus, 1);
		else
			InterlockedExchange(&g_NetworkStatus, 0);
	}

QuitMQTTSubThread:
	MQTT::MQTT_DestroyInstance(mosq);
	mosq = nullptr;
	wt_mempool_destroy(mempool);
	InterlockedDecrement(&g_threadCount);

	return 0;
}

static int SendPingMessage(struct mosquitto* mq)
{
	int r = 1;
	return 0;
}

static void UpdateInputMessageTask(MessageTask* mt)
{

}

static MessageTask* CloneInputMessageTask(MemoryPoolContext mempool)
{
	MessageTask* mt = nullptr;
	
	EnterCriticalSection(&g_csMQTTPub);
	if (g_PubData.task)
	{
		MessageTask* p;
		MessageTask* q;
		MessageTask* t;

		p = g_PubData.task;
		mt = (MessageTask*)wt_palloc0(mempool, sizeof(MessageTask));
		if (!mt)
		{
			LeaveCriticalSection(&g_csMQTTPub);
			return nullptr;
		}
		memcpy(mt, p, sizeof(MessageTask)); // clone a node
		mt->message = p->message;
		mt->next = nullptr;
		q = mt;

		p = g_PubData.task->next;
		while (p)
		{
			t = (MessageTask*)wt_palloc0(mempool, sizeof(MessageTask));
			if (!t)
				break;
			memcpy(t, p, sizeof(MessageTask)); // clone a node
			t->message = p->message;
			t->next = nullptr;
			q->next = t;
			q = t;
			p = p->next;
		}
	}
	
	LeaveCriticalSection(&g_csMQTTPub);

	return mt;
}


extern "C"
{
	static bool process_messages = true;
	static int msg_count = 0;
	static int last_mid = 0;
	static bool timed_out = false;
	static int connack_result = 0;
	static bool connack_received = false;

	static void FreeMessageTask(MessageTask* mt)
	{
		if (mt)
		{
			if(mt->message)
				wt_pfree(mt->message);

			wt_pfree(mt);
		}
	}
#if 0
	static MessageTask* ProcessIncomingMessage(MQTTPrivateData* pd, bool KeepAlive, U8* raw_message, U32 message_length)
	{
		MessageTask* mt = nullptr;
		MemoryPoolContext mempool = (MemoryPoolContext)pd->mempool;

		if (mempool && raw_message && (message_length > 130))
		{
			if (!wt_IsPublicKey(raw_message, 66))
				return nullptr;

			mt = (MessageTask*)wt_palloc0(mempool, sizeof(MessageTask));
			if (!mt)
				return nullptr;

			if (KeepAlive) // this is a keep alive message
			{
				return mt;
			}
#if 0
			{
				int output_len = wt_b64_dec_len(len - 67 - 1); // get the bytes after decode, the last byte is 0
				U8* p = (U8*)wt_palloc(mempool, output_len);
				if (p)
				{
					int m;
					U8 sha256hash[32];
					U8 nonce[12];
					U8 key[32] = { 0 };
					wt_chacha20_context cxt;
					wt_HexString2Raw(msg, 66, mt->pubkey, nullptr);
					GetKeyFromSKAndPK(g_SK, mt->pubkey, key);
					int realen = wt_b64_decode((const char*)msg + 67, len - 67 - 1, (char*)p, output_len);

					wt_chacha20_init(&cxt);
					m = wt_chacha20_setkey(&cxt, key);
					for (int i = 0; i < 12; i++) nonce[i] = i;
					m = wt_chacha20_starts(&cxt, nonce, 0);
					m = wt_chacha20_update(&cxt, realen, (const unsigned char*)p, p);  // decrypt the message
					wt_chacha20_free(&cxt);

					mt->msgLen = *((U32*)p); // get the length
					if (mt->msgLen != realen - 4 - 32)
					{
						wt_pfree(p);
						wt_pfree(mt);
						return 0;
					}

					wt_sha256_ctx ctx = { 0 };
					wt_sha256_init(&ctx);
					wt_sha256_update(&ctx, (const unsigned char*)p + 4 + 32, realen - 4 - 32);
					wt_sha256_final(&ctx, sha256hash);

					if (memcmp(sha256hash, p + 4, 32)) // compare the hash value
					{
						wt_pfree(p);
						wt_pfree(mt);
						return 0;
					}

					mt->message = (wchar_t*)wt_palloc(mempool, mt->msgLen);
					memcpy(mt->message, p + 36, mt->msgLen);
					mt->msgLen /= sizeof(wchar_t);
					wt_pfree(p);

#endif 
		}

		return mt;
	}

	static int PostMQTTMessage(MQTTPrivateData* pd, const struct mosquitto_message* message, const mosquitto_property* properties)
	{
		bool keepAlive = false;
		MessageTask* mt;
		HWND hWndUI;

		// do some check here
		if (nullptr == message || nullptr == pd)
			return 0;

		hWndUI = pd->hWnd;
		if (!(::IsWindow(hWndUI)))
			return 0;

		if (strlen(message->topic) < 5)
			return 0;

		if (0 == memcmp(message->topic, "WOCHAT", 5))
			keepAlive = true; // this is a keep alive message

		mt = ProcessIncomingMessage(pd, keepAlive, (U8*)message->payload, (U32)message->payloadlen);
		if (mt)
		{
			if (0 != PushReceiveMessageQueue(mt))
			{
				FreeMessageTask(mt);
				return 0;
			}
			::PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 0, 0); // tell the UI thread that a new message is received
		}
		return 0;
	}
#endif
	static void MQTT_Message_Callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message, const mosquitto_property* properties)
	{
		int i;
		HWND hWndUI;
		char* msgbody;
		int msglen;
		MQTTConf* conf = (MQTTConf*)obj;
		assert(conf);

		UNUSED(properties);

		hWndUI = (HWND)conf->hWnd;
		assert(::IsWindow(hWndUI));

		msgbody = (char*)message->payload;
		msglen = message->payloadlen;

#if 0
		if (conf->retained_only && !message->retain && process_messages)
		{
			process_messages = false;
			if (last_mid == 0)
			{
				mosquitto_disconnect_v5(mosq, 0, conf->disconnect_props);
			}
			return;
		}

		if (message->retain && conf->no_retain)
			return;

		if (conf->filter_outs)
		{
			for (i = 0; i < conf->filter_out_count; i++)
			{
				mosquitto_topic_matches_sub(conf->filter_outs[i], message->topic, &res);
				if (res) return;
			}
		}

		if (conf->remove_retained && message->retain)
		{
			mosquitto_publish(mosq, &last_mid, message->topic, 0, NULL, 1, true);
		}

		if (conf->msg_count > 0)
		{
			msg_count++;
			if (conf->msg_count == msg_count)
			{
				process_messages = false;
				if (last_mid == 0)
				{
					mosquitto_disconnect_v5(mosq, 0, conf->disconnect_props);
				}
			}
		}
#endif
	}

	static void MQTT_Connect_Callback(struct mosquitto* mosq, void* obj, int result, int flags, const mosquitto_property* properties)
	{
		int i;
		MQTTConf* conf = (MQTTConf*)obj;
		assert(conf);

		UNUSED(flags);
		UNUSED(properties);

		if (!result)
		{
			if (conf->topic_count > 0)
			{
				mosquitto_subscribe_multiple(mosq, NULL, conf->topic_count, conf->topics, conf->qos, conf->sub_opts, conf->subscribe_props);
			}
#if 0
			for (i = 0; i < conf->unsub_topic_count; i++)
			{
				mosquitto_unsubscribe_v5(mosq, NULL, conf->unsub_topics[i], conf->unsubscribe_props);
			}
#endif 
		}
		else
		{
			if (result)
			{
				if (conf->protocol_version == MQTT_PROTOCOL_V5)
				{
					if (result == MQTT_RC_UNSUPPORTED_PROTOCOL_VERSION)
					{
						//err_printf(&cfg, "Connection error: %s. Try connecting to an MQTT v5 broker, or use MQTT v3.x mode.\n", mosquitto_reason_string(result));
					}
					else
					{
						//err_printf(&cfg, "Connection error: %s\n", mosquitto_reason_string(result));
					}
				}
				else {
					//err_printf(&cfg, "Connection error: %s\n", mosquitto_connack_string(result));
				}
			}
			mosquitto_disconnect_v5(mosq, 0, conf->disconnect_props);
		}
	}

	static void MQTT_Disconnect_Callback(struct mosquitto* mosq, void* obj, int result, const mosquitto_property* properties)
	{

	}

	static void MQTT_Publish_Callback(struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* properties)
	{

	}

	static void MQTT_Subscribe_Callback(struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos)
	{
#if 0
		int i;
		struct mosq_config* conf;
		bool some_sub_allowed = (granted_qos[0] < 128);
		bool should_print;

		conf = (struct mosq_config*)obj;
		assert(nullptr != conf);
		should_print = conf->debug && !conf->quiet;

#if 0
		if (should_print)
			printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
#endif
		for (i = 1; i < qos_count; i++)
		{
			//if (should_print) printf(", %d", granted_qos[i]);
			some_sub_allowed |= (granted_qos[i] < 128);
		}
		//if (should_print) printf("\n");

		if (some_sub_allowed == false)
		{
			mosquitto_disconnect_v5(mosq, 0, conf->disconnect_props);
			//err_printf(&cfg, "All subscription requests were denied.\n");
		}

		if (conf->exit_after_sub)
		{
			mosquitto_disconnect_v5(mosq, 0, conf->disconnect_props);
		}
#endif
	}

	static void MQTT_Log_Callback(struct mosquitto* mosq, void* obj, int level, const char* str)
	{

	}
}


#if 0
AESEncrypt(NULL, 0, NULL);

{
	int return_val;
	size_t len;
	U8 randomize[32];
	U8 compressed_pubkey[33];
	secp256k1_context* ctx;
	secp256k1_pubkey pubkey;

	ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	for (int i = 0; i < 32; i++)
		randomize[i] = 137 - i;
	return_val = secp256k1_context_randomize(ctx, randomize);
	return_val = secp256k1_ec_pubkey_create(ctx, &pubkey, g_SK);
	len = sizeof(compressed_pubkey);
	return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey, &len, &pubkey, SECP256K1_EC_COMPRESSED);
	secp256k1_context_destroy(ctx);
}

{
	U32 crc;
	char* p = "abc1";
	INIT_CRC32C(crc);

	COMP_CRC32C(crc, p, 4);
	FIN_CRC32C(crc);

	int i = 1;
}

InitWoChatDatabase(g_AppPath);

{
	U32 hv;
	hv = hash_bytes((const unsigned char*)"03342EF59638622ABEE545AFABA85463932DA66967F2BA4EE6254F9988BEE5F0C2", 66);
	hv = hash_bytes((const unsigned char*)"B5C10E18A44C596BC0C213B9A3DD3E724C35B03BF2EBBADFB1B7C09AD2F9324C", 64);
	hv = 0;
}

{
	bool found;
	HTAB* ht;
	HASHCTL ctl;
	ctl.keysize = 66;
	ctl.entrysize = ctl.keysize + sizeof(void*);
	ctl.hcxt = nullptr;

	ht = hash_create("HASHT", 100, &ctl, HASH_ELEM | HASH_BLOBS);

	void* ptr = hash_search(ht, g_PKText, HASH_ENTER, &found);

	hash_destroy(ht);

}
{
	int i = 321;
	DebugPrintf("================The value is %d\n", i);
}

{
	U8 sk1[32];
	U8 sk2[32];
	U8 pk1[33];
	U8 pk2[33];
	U8 K1[32] = { 0 };
	U8 K2[32] = { 0 };
	int ret;

	HexString2Raw((U8*)"9614310CE21149F83E6FA13E30FAA6B6233F6ED0344BC2F5716921EF1988400B", 64, sk1, nullptr);
	HexString2Raw((U8*)"2D65976AA60FDC65451A6244E4668AC9D757CEF2426B9FCECAC3BA0A52226EC4", 64, sk2, nullptr);
	HexString2Raw((U8*)"03DB0E39AFB91AD07B0088B9B0EDDF5903230BC1C2F9AA08104FE0F10D4BAA0E0D", 66, pk1, nullptr);
	HexString2Raw((U8*)"021790BF052D7940B861664ACA9729B48BD45498217A910CDF90D56C76045B9F7D", 66, pk2, nullptr);

	ret = GetKeyFromSKAndPK(sk1, pk2, K1);
	ret = GetKeyFromSKAndPK(sk2, pk1, K2);
	ret++;
}

{
	U8 sk[32] = { 0 };
	U8 k[32] = { 0 };
	U8 c[32] = { 0 };
	U8 p[32] = { 0 };

	wt_HexString2Raw((U8*)"9614310CE21149F83E6FA13E30FAA6B6233F6ED0344BC2F5716921EF1988400B", 64, sk, nullptr);
	wt_HexString2Raw((U8*)"2D65976AA60FDC65451A6244E4668AC9D757CEF2426B9FCECAC3BA0A52226EC4", 64, k, nullptr);

	AES256_ctx ctx = { 0 };
	wt_AES256_init(&ctx, k);
	wt_AES256_encrypt(&ctx, 2, c, sk);
	wt_AES256_decrypt(&ctx, 2, p, c);

	memset(&ctx, 0, sizeof(ctx));
}

{
	U8 sk[32] = { 0 };
	int r = wt_GenerateSecretKey(sk);
	if (r)
	{
		r++;
	}
}
#endif

int GetKeyFromSKAndPK(U8* sk, U8* pk, U8* key)
{
	int ret = 1;
	secp256k1_context* ctx;
	secp256k1_pubkey pubkey;
	U8 K[32] = { 0 };

	ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (ctx)
	{
		ret = secp256k1_ec_pubkey_parse(ctx, &pubkey, pk, 33);
		if (1 != ret)
		{
			secp256k1_context_destroy(ctx);
			ret = 1;
			return ret;
		}
		ret = secp256k1_ecdh(ctx, K, &pubkey, sk, NULL, NULL);
		if (ret && key)
		{
			for (int i = 0; i < 32; i++)
				key[i] = K[i];
		}

		secp256k1_context_destroy(ctx);
		ret = 0;
	}
	return ret;
}

// we have the 32-byte secret key, we want to generate the public key, return 0 if successful
int GenPublicKeyFromSecretKey(U8* sk, U8* pk)
{
	int ret = 1;
	secp256k1_context* ctx;

	ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (ctx)
	{
		int return_val;
		size_t len = 33;
		U8 compressed_pubkey[33];
		secp256k1_pubkey pubkey;

		return_val = secp256k1_ec_pubkey_create(ctx, &pubkey, sk);
		return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey, &len, &pubkey, SECP256K1_EC_COMPRESSED);
		if (len == 33)
		{
			for (int i = 0; i < 33; i++)
				pk[i] = compressed_pubkey[i];
			ret = 0;
		}
		secp256k1_context_destroy(ctx);
	}

	return ret;
}

int PushReceiveMessageQueue(MessageTask* message_task)
{
	int ret = 0;
	MQTTSubData* pd = &g_SubData;
	MessageTask* p;

	EnterCriticalSection(&g_csMQTTSub);

	while (pd->task) // scan the link to find the message that has been processed
	{
		p = pd->task->next;
		if (0 == pd->task->state) // this task is not processed yet.
		{
			break;
		}
		wt_pfree(pd->task->message);
		wt_pfree(pd->task);
		pd->task = p;
	}

	if (nullptr == pd->task) // there is no task in the queue
	{
		pd->task = message_task;
		LeaveCriticalSection(&g_csMQTTSub);
		return 0;
	}
	assert(pd->task);
	p = pd->task;
	while (p->next)
		p = p->next;

	p->next = message_task;
	message_task->next = nullptr;

	LeaveCriticalSection(&g_csMQTTSub);

	return 0;
}

// put a message node into the send message queue
int PushTaskIntoSendMessageQueue(MessageTask* mt)
{
	MessageTask* p;
	MessageTask* q;

	if (mt)
	{
		EnterCriticalSection(&g_csMQTTPub);
		if (nullptr == g_PubData.task) // there is no task in the queue
		{
			g_PubData.task = mt;
			LeaveCriticalSection(&g_csMQTTPub);
			SetEvent(g_MQTTPubEvent); // tell the Pub thread to work
			return 0;
		}

		q = g_PubData.task;
		p = g_PubData.task;

		while (p) // try to find the first node that is not processed yet
		{
			if (p->state < 2) // this task is not processed
			{
				break;
			}
			p = p->next;
		}
		g_PubData.task = p;
		// here, g_PubData.task is pointing to a node not procesded or NULL
		if (nullptr == g_PubData.task) // there is no task in the queue
		{
			g_PubData.task = mt;
		}

		p = q; // p is pointing to the first node
		while (p)
		{
			q = p->next;
			if (nullptr == q) // we reach the end of the queue
				p->next = mt;

			if (p->state > 1) // this task has been processed, we can free the memory
			{
				wt_pfree(p->message);
				wt_pfree(p);
			}
			p = q;
		}
		LeaveCriticalSection(&g_csMQTTPub);

		SetEvent(g_MQTTPubEvent); // tell the Pub thread to work
	}
	return 0;
}


#define SQL_STMT_MAX_LEN		1024

int InitWoChatDatabase(LPCWSTR lpszPath)
{
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;
	int           rc;
	wchar_t keyFile[MAX_PATH + 1] = { 0 };
	wchar_t sql[SQL_STMT_MAX_LEN] = { 0 };

	swprintf((wchar_t*)keyFile, MAX_PATH, L"%s\\wt.db", lpszPath);

	rc = sqlite3_open16(keyFile, &db);

	if (rc)
	{
		sqlite3_close(db);
		return -1;
	}

	_stprintf_s(sql, SQL_STMT_MAX_LEN,
		L"CREATE TABLE config(id INTEGER PRIMARY KEY, idx INTEGER, intv INTEGER, charv VARCHAR(2048))");
	rc = sqlite3_prepare16_v2(db, sql, -1, &stmt, NULL);
	rc = sqlite3_step(stmt);
	rc = sqlite3_finalize(stmt);

	sqlite3_close(db);

	return 0;
}
