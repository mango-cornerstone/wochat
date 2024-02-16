#include "pch.h"
#include "mosquitto.h"
#include "wochat.h"
#include "wt_base64.h"
#include "wt_utils.h"
#include "wt_sha256.h"
#include "wt_chacha20.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"

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

int GetPKFromSK(U8* sk, U8* pk)
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

int PushSendMessageQueue(MessageTask* message_task)
{
	MQTTPubData* pd;
	MessageTask* p;
	MessageTask* q;

	EnterCriticalSection(&g_csMQTTPub);
	pd = &g_PubData;

	if (nullptr == pd->task) // there is no task in the queue
	{
		pd->task = message_task;
		LeaveCriticalSection(&g_csMQTTPub);
		SetEvent(g_MQTTPubEvent); // tell the Pub thread to work
		return 0;
	}
	
	q = pd->task;
	while (pd->task)
	{
		if (pd->task->state < 2) // this task has been processed, we can free the memory
		{
			break;
		}
		pd->task = pd->task->next;
	}

	// here, pd->task is pointing to a node not procesded or NULL
	if (nullptr == pd->task) // there is no task in the queue
	{
		pd->task = message_task;
	}

	p = q; // p is pointing to the first node
	while (p)
	{
		q = p->next;
		if (nullptr == q) // we reach the end of the queue
			p->next = message_task;
	
		if (p->state > 1) // this task has been processed, we can free the memory
		{
			wt_pfree(p->message);
			wt_pfree(p);
		}
		p = q;
	}

	LeaveCriticalSection(&g_csMQTTPub);

	SetEvent(g_MQTTPubEvent); // tell the Pub thread to work

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

	static XMQTTMessage mqtt_message = { 0 };
}

// client_id is 23 characters, and sk is 32 bytes
static int GenClientID(U8* client_id, U8* sk)
{
	int r = 1;
	U8 output[32] = { 0 };
	wt_sha256_ctx ctx = { 0 };

	wt_sha256_init(&ctx);
	wt_sha256_update(&ctx, (const unsigned char*)sk, 32);
	wt_sha256_final(&ctx, output);

	wt_Raw2HexString(output, 11, client_id, nullptr);
	r = 0;
	return r;
}

static MQTTPrivateData subdata;
static MQTTPrivateData pubdata;

DWORD WINAPI MQTTSubThread(LPVOID lpData)
{
	int ret = 0;
	Mosquitto mq = nullptr;
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

	subdata.hWnd = hWndUI;
	subdata.mempool = mempool;

	GenClientID(subdata.client_id, g_SK); // generate the client id from the private key
	mq = MQTT::MQTT_SubInit(&subdata, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, &mqtt_callback);

	if (nullptr == mq) // something is wrong in MQTT sub routine
	{
		PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 1, 0);
		goto QuitMQTTSubThread;
	}

	wt_Raw2HexString(g_PK, 33, topic, nullptr);
	ret = MQTT::MQTT_AddSubTopic(mempool, CLIENT_SUB, (char*)topic);

	if(ret)
	{
		PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 2, 0);
		goto QuitMQTTSubThread;
	}

	MQTT::MQTT_SubLoop(mq, &g_Quit);  // main loop go here.

QuitMQTTSubThread:

	MQTT::MQTT_SubTerm(mq);
	mq = nullptr;
	wt_mempool_destroy(mempool);
	InterlockedDecrement(&g_threadCount);
	return 0;
}

DWORD WINAPI MQTTPubThread(LPVOID lpData)
{
	int ret = 0;
	DWORD dwRet;
	HWND hWndUI;
	Mosquitto mq;
	MemoryPoolContext mempool;
	MessageTask* p;
	char topic[67] = { 0 };
	U8    sha256hash[32] = { 0 };
	U8*   msgbody = nullptr;
	U32   msglen = 0;
	size_t encoded_len;
	size_t output_len;

	InterlockedIncrement(&g_threadCount);
	hWndUI = (HWND)(lpData);
	ATLASSERT(::IsWindow(hWndUI));

	mempool = wt_mempool_create("MQTT_PUB_POOL", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	if (nullptr == mempool)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 2, 0);
		goto QuitMQTTPubThread;
	}

	pubdata.hWnd = hWndUI;
	pubdata.mempool = mempool;
	GenClientID(pubdata.client_id, g_SK); // generate the client id from the private key

	mq = MQTT::MQTT_PubInit(&pubdata, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, &mqtt_callback);

	if (nullptr == mq)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 1, 0);
		goto QuitMQTTPubThread;
	}

	while (true)
	{
		dwRet = WaitForSingleObject(g_MQTTPubEvent, INFINITE);

		if (dwRet != WAIT_OBJECT_0)
			continue;
		if (g_Quit) // we will quit the whole program
			break;

		EnterCriticalSection(&g_csMQTTPub);
		p = g_PubData.task;
		LeaveCriticalSection(&g_csMQTTPub);

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
	}

	MQTT::MQTT_PubTerm(mq);

QuitMQTTPubThread:
	wt_mempool_destroy(mempool);
	InterlockedDecrement(&g_threadCount);

	return 0;
}

extern "C"
{
	static bool process_messages = true;
	static int msg_count = 0;
	static int last_mid = 0;
	static bool timed_out = false;
	static int connack_result = 0;
	static bool connack_received = false;

	static int PostMQTTMessage(MQTTPrivateData* pd, const struct mosquitto_message* message, const mosquitto_property* properties)
	{
		U8* msg;
		size_t len;
		HWND hWndUI;
		MemoryPoolContext mempool;
		MessageTask* mt;

		// do some check here
		if (nullptr == message)
			return 0;
		if (nullptr == pd)
			return 0;
		hWndUI = pd->hWnd;
		mempool = (MemoryPoolContext)pd->mempool;
		if (!(::IsWindow(hWndUI)))
			return 0;
		if (nullptr == mempool)
			return 0;

		msg = (U8*)message->payload;
		if (nullptr == msg)
			return 0;
		len = (size_t)message->payloadlen;
		if (len < 120) // 67 + base64(4 + 32 + 2) + '\0'
			return 0;

		if (msg[len]) // the last byte should be zero
			return 0;

		if (!wt_IsPublicKey(msg, 66)) // the first 66 bytes are the public key of the sender
			return 0;
		
		if (msg[66] != '|')
			return 0;

		mt = (MessageTask*)wt_palloc0(mempool, sizeof(MessageTask));
		if (mt)
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

				if(memcmp(sha256hash, p + 4, 32)) // compare the hash value
				{
					wt_pfree(p);
					wt_pfree(mt);
					return 0;
				}

				mt->message = (wchar_t*)wt_palloc(mempool, mt->msgLen);
				memcpy(mt->message, p + 36, mt->msgLen);
				mt->msgLen /= sizeof(wchar_t);
				wt_pfree(p);
				if (PushReceiveMessageQueue(mt))
				{
					wt_pfree(mt->message);
					wt_pfree(mt);
					return 0;
				}

				::PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 0, 0); // tell the UI thread that a new message is received
			}
			else
			{
				wt_pfree(mt);
			}
		}

		return 0;
	}

	static void MQTT_Message_Callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message, const mosquitto_property* properties)
	{
		int i;
		bool res;
		struct mosq_config* pMQTTConf;
		MQTTPrivateData* pd;

		UNUSED(properties);

		pMQTTConf = (struct mosq_config*)obj;
		assert(nullptr != pMQTTConf);

		pd = (MQTTPrivateData*)(pMQTTConf->userdata);
		assert(pd);
		assert(::IsWindow(pd->hWnd));

		if (process_messages == false) return;

		if (pMQTTConf->retained_only && !message->retain && process_messages)
		{
			process_messages = false;
			if (last_mid == 0)
			{
				mosquitto_disconnect_v5(mosq, 0, pMQTTConf->disconnect_props);
			}
			return;
		}

		if (message->retain && pMQTTConf->no_retain)
			return;

		if (pMQTTConf->filter_outs)
		{
			for (i = 0; i < pMQTTConf->filter_out_count; i++)
			{
				mosquitto_topic_matches_sub(pMQTTConf->filter_outs[i], message->topic, &res);
				if (res) return;
			}
		}

		if (pMQTTConf->remove_retained && message->retain)
		{
			mosquitto_publish(mosq, &last_mid, message->topic, 0, NULL, 1, true);
		}

		PostMQTTMessage(pd, message, properties);

		if (pMQTTConf->msg_count > 0)
		{
			msg_count++;
			if (pMQTTConf->msg_count == msg_count)
			{
				process_messages = false;
				if (last_mid == 0)
				{
					mosquitto_disconnect_v5(mosq, 0, pMQTTConf->disconnect_props);
				}
			}
		}
	}

	static void MQTT_Connect_Callback(struct mosquitto* mosq, void* obj, int result, int flags, const mosquitto_property* properties)
	{
		int i;
		struct mosq_config* pMQTTConf;

		UNUSED(flags);
		UNUSED(properties);

		pMQTTConf = (struct mosq_config*)obj;
		assert(nullptr != pMQTTConf);

		//connack_received = true;

		//connack_result = result;

		if (!result)
		{
			mosquitto_subscribe_multiple(mosq, NULL, pMQTTConf->topic_count, pMQTTConf->topics, pMQTTConf->qos, pMQTTConf->sub_opts, pMQTTConf->subscribe_props);

			for (i = 0; i < pMQTTConf->unsub_topic_count; i++)
			{
				mosquitto_unsubscribe_v5(mosq, NULL, pMQTTConf->unsub_topics[i], pMQTTConf->unsubscribe_props);
			}
		}
		else
		{
			if (result)
			{
				if (pMQTTConf->protocol_version == MQTT_PROTOCOL_V5)
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
			mosquitto_disconnect_v5(mosq, 0, pMQTTConf->disconnect_props);
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
		int i;
		struct mosq_config* pMQTTConf;
		bool some_sub_allowed = (granted_qos[0] < 128);
		bool should_print;

		pMQTTConf = (struct mosq_config*)obj;
		assert(nullptr != pMQTTConf);
		should_print = pMQTTConf->debug && !pMQTTConf->quiet;

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
			mosquitto_disconnect_v5(mosq, 0, pMQTTConf->disconnect_props);
			//err_printf(&cfg, "All subscription requests were denied.\n");
		}

		if (pMQTTConf->exit_after_sub)
		{
			mosquitto_disconnect_v5(mosq, 0, pMQTTConf->disconnect_props);
		}

	}

	static void MQTT_Log_Callback(struct mosquitto* mosq, void* obj, int level, const char* str)
	{

	}
}

