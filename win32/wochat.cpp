#include "pch.h"
#include "mosquitto.h"
#include "wochatdef.h"
#include "wochat.h"
#include "wt_base64.h"
#include "wt_utils.h"
#include "wt_sha256.h"
#include "wt_chacha20.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"

#define WT_NETWORK_IS_BAD		0
#define WT_NETWORK_IS_GOOD		1

S64 GetCurrentUTCTime64()
{
	S64 tm;
	U16* p16;
	U8* p;
	SYSTEMTIME st;

	GetSystemTime(&st);

	p16 = (U16*)&tm; *p16 = st.wYear;
	p = (U8*)&tm; p[2] = st.wMonth; p[3] = st.wDay; p[4] = st.wHour; p[5] = st.wMinute; p[6] = st.wSecond; p[7] = 0;

	return tm;
}

void GetU64TimeStringW(S64 tm, wchar_t* tmstr, U16 len)
{
	if (tmstr)
	{
		U16* p16 = (U16*)&tm;
		U8* p = (U8*)&tm;
		U16 yy = *p16;
		swprintf(tmstr, len, L"%04d/%02d/%02d %02d:%02d:%02d", yy, p[2], p[3], p[4], p[5], p[6]);
	}
}


U32 ng_seqnumber = 0;

typedef struct PublishTask
{
	PublishTask* next;
	MessageTask* node;
	U8* message; 
	U32 msgLen;
} PublishTask;

static PublishTask* CloneInputMessageTasks(MemoryPoolContext mempool)
{
	PublishTask* pt = nullptr;
	
	EnterCriticalSection(&g_csMQTTPubUI);
	if (g_PubData.task)
	{
		U32 ret, len, len0;
		U32* p32;
		bool firstTime = true;
		MessageTask* p;
		PublishTask* item;
		PublishTask* curr = nullptr;

		p = g_PubData.task;
		while (p)
		{
			if (p->state != MESSAGE_TASK_STATE_COMPLETE)
			{
				item = (PublishTask*)wt_palloc0(mempool, sizeof(PublishTask));
				if (item)
				{
					item->node = p;
					if (p->type == 'T') // it is UTF16 encoding, we convert to UTF8 encoding
					{
						len0 = p->msgLen / sizeof(wchar_t);
						ret = wt_UTF16ToUTF8((U16*)p->message, len0, nullptr, &len);
						if (WT_OK != ret)
						{
							wt_pfree(item);
							continue;
						}
						item->message = (U8*)wt_palloc(mempool, len + 4);
						if (item->message == nullptr)
						{
							wt_pfree(item);
							continue;
						}
						
						p32 = (U32*)item->message;
						*p32 = ng_seqnumber;

						ret = wt_UTF16ToUTF8((U16*)p->message, len0, item->message + 4, nullptr);
						if (WT_OK != ret)
						{
							wt_pfree(item->message);
							wt_pfree(item);
							continue;
						}
						item->msgLen = len + 4;
					}

					if (firstTime)
					{
						firstTime = false;
						pt = item;
						curr = item;
					}
					else
					{
						assert(curr);
						curr->next = item;
						curr = item;
					}
				}
			}
			p = p->next;
		}
	}
	LeaveCriticalSection(&g_csMQTTPubUI);

	return pt;
}

static int PushTaskIntoReceiveMessageQueue(MessageTask* task)
{
	int ret = 0;
	MQTTSubData* pd = &g_SubData;
	MessageTask* p;

	EnterCriticalSection(&g_csMQTTSubUI);

	while (pd->task) // scan the link to find the message that has been processed
	{
		p = pd->task->next;
		if (MESSAGE_TASK_STATE_NULL == pd->task->state) // this task is not processed yet.
		{
			break;
		}
		wt_pfree(pd->task->message);
		wt_pfree(pd->task);
		pd->task = p;
	}

	if (nullptr == pd->task) // there is no task in the queue
	{
		pd->task = task;
		LeaveCriticalSection(&g_csMQTTSubUI);
		return 0;
	}

	p = pd->task;
	while (p->next)
		p = p->next;

	p->next = task;
	task->next = nullptr;

	LeaveCriticalSection(&g_csMQTTSubUI);

	return 0;
}

static U32 GetKeyFromSecretKeyAndPlubicKey(U8* sk, U8* pk, U8* key)
{
	U32 ret = WT_FAIL;
	secp256k1_context* ctx;

	assert(sk);
	assert(pk);
	assert(key);

	ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (ctx)
	{
		int rc;
		secp256k1_pubkey pubkey;
		U8 k[SECRET_KEY_SIZE] = { 0 };

		rc = secp256k1_ec_pubkey_parse(ctx, &pubkey, pk, PUBLIC_KEY_SIZE);
		if (1 != rc)
		{
			secp256k1_context_destroy(ctx);
			return WT_SECP256K1_CTX_ERROR;
		}
		rc = secp256k1_ecdh(ctx, k, &pubkey, sk, NULL, NULL);
		if (1 == rc)
		{
			for (int i = 0; i < SECRET_KEY_SIZE; i++)
			{
				key[i] = k[i];
				k[i] = 0;
			}
			ret = WT_OK;
		}
		secp256k1_context_destroy(ctx);
	}

	return ret;
}

static U32 InsertIntoGoodMsgTable(U8* txt, U32 len, U8* hash)
{
	U32 ret = WT_FAIL;
	int rc;
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;
	U8 sql[128];

	U8 hexHash[65];
	wt_Raw2HexString(hash, 32, hexHash, nullptr);

	rc = sqlite3_open16(g_DBPath, &db);
	if (SQLITE_OK != rc)
	{
		sqlite3_close(db);
		return WT_SQLITE_OPEN_ERR;
	}

	sprintf_s((char*)sql, 128, "SELECT count(1) FROM r WHERE h='%s'", hexHash);
	rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW)
		{
			int count = sqlite3_column_int(stmt, 0);
			if (count > 0)
			{
				ret = WT_OK;
				goto Exit_InsertIntoGoodMsgTable;
			}
		}
	}

	rc = sqlite3_prepare_v2(db, (const char*)"INSERT INTO r(h,t) VALUES((?),(?))", -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_bind_text(stmt, 1, (const char*)hexHash, 64, SQLITE_TRANSIENT);
		if (SQLITE_OK == rc)
		{
			rc = sqlite3_bind_text(stmt, 2, (const char*)txt, (int)len, SQLITE_TRANSIENT);
			if (SQLITE_OK == rc)
			{
				rc = sqlite3_step(stmt);
				if (SQLITE_DONE == rc)
					ret = WT_OK;
			}
		}
	}

Exit_InsertIntoGoodMsgTable:
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return ret;
}

static U32 InsertIntoAbadonMsgTable(U8* text, U32 len)
{
	U32 ret = WT_FAIL;
	int rc;
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;

	EnterCriticalSection(&g_csMQTTPubSub);

	rc = sqlite3_open16(g_DBPath, &db);
	if (SQLITE_OK == rc)
	{
		S64 tm = GetCurrentUTCTime64();
		rc = sqlite3_prepare_v2(db, (const char*)"INSERT INTO q(dt,tx) VALUES((?),(?))", -1, &stmt, NULL);
		if (SQLITE_OK == rc)
		{
			rc = sqlite3_bind_int64(stmt, 1, tm);
			if (SQLITE_OK == rc)
			{
				rc = sqlite3_bind_text(stmt, 2, (const char*)text, (int)len, SQLITE_TRANSIENT);
				if (SQLITE_OK == rc)
				{
					rc = sqlite3_step(stmt);
					if (SQLITE_DONE == rc)
						ret = WT_OK;
				}
			}
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
	LeaveCriticalSection(&g_csMQTTPubSub);

	return ret;
}

static U32 InsertIntoSendMsgTable(U8* txt, U32 len, U8* hash, bool successful = true)
{
#if 0
	U32 ret = WT_FAIL;
	int rc;
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;
	U8 hexHash[65];

	wt_Raw2HexString(hash, 32, hexHash, nullptr);

	rc = sqlite3_open16(g_DBPath, &db);
	if (SQLITE_OK != rc)
	{
		sqlite3_close(db);
		return WT_SQLITE_OPEN_ERR;
	}

	if(successful)
		rc = sqlite3_prepare_v2(db, (const char*)"INSERT INTO s(st,h,t) VALUES('Y',(?),(?))", -1, &stmt, NULL);
	else
		rc = sqlite3_prepare_v2(db, (const char*)"INSERT INTO s(st,h,t) VALUES('N',(?),(?))", -1, &stmt, NULL);

	if (SQLITE_OK == rc)
	{
		rc = sqlite3_bind_text(stmt, 1, (const char*)hexHash, 64, SQLITE_TRANSIENT);
		if (SQLITE_OK == rc)
		{
			rc = sqlite3_bind_text(stmt, 2, (const char*)txt, (int)len, SQLITE_TRANSIENT);
			if (SQLITE_OK == rc)
			{
				rc = sqlite3_step(stmt);
				if (SQLITE_DONE == rc)
					ret = WT_OK;
			}
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return ret;
#endif
	return WT_OK;
}

static int SendOutPushlishTask(struct mosquitto* mosq, MemoryPoolContext pool, PublishTask* task)
{
	assert(task);
	MessageTask* node = task->node;
	U8 Kp[SECRET_KEY_SIZE] = { 0 };
	U8 topic[67] = { 0 };
	U8* msssage_b64;
	U32 length_b64;

	if (!node)
		return 0;

	// get the primary key between the sender and the receiver
	if (WT_OK != GetKeyFromSecretKeyAndPlubicKey(g_SK, node->pubkey, Kp))
		return 0;

	if (node->type == 'T') // this is a text message
	{
		U8 offset = 0;
		U8* message_raw;
		U32 length_raw;

		assert(task->message);

		if (task->msgLen < 252) // we keep the minimal size to 252 bytes to make it harder for the attacker
		{
			length_raw = 252 + 72;
			offset = (U8)(252 - task->msgLen);
		}
		else
		{
			offset = 0;
			length_raw = task->msgLen + 72; // if the message is equal or more than 252 bytes, we do not make random data
		}

		message_raw = (U8*)wt_palloc(pool, length_raw);
		if (message_raw)
		{
			U8 idx;
			U32 i, j;
			wt_chacha20_context cxt = { 0 };
			AES256_ctx ctxAES0 = { 0 };
			AES256_ctx ctxAES1 = { 0 };
			U8 hash[32] = { 0 };
			U8 Ks[32] = { 0 };
			U8 mask[8] = { 0 };
			U8 nonce[12] = { 0 };
			U8* q;

			wt_sha256_hash(task->message, task->msgLen, hash); // generate the SHA256 hash of the original message
			wt_GenerateNewSecretKey(Ks); // genarate the session key, it is a randome number

			U8* p = message_raw;

			// the first 32 bytes contain the encrypted session key. Ks : key session
			wt_AES256_init(&ctxAES0, Kp);
			wt_AES256_encrypt(&ctxAES0, 2, p, Ks); 

			// the second 32 bytes contain the encrypted SHA256 of the original message.
			wt_AES256_init(&ctxAES1, Kp);
			wt_AES256_encrypt(&ctxAES1, 2, p+32, hash); 

			// generate the mask value of the encrypted SHA256
			for (i = 0; i < 4; i++)
			{
				q = p + 32 + i * 8;
				for (j = 0; j < 8; j++) mask[i] ^= q[i];
			}

			// handle the next 4 bytes
			p[64] = WT_ENCRYPTION_VERION; // the version byte
			p[65] = node->type; // the data type byte
			idx = wt_GenRandomU8(offset); // a random offset
			p[66] = idx;
			p[67] = 'X'; // reserved byte
			// save the original message length in the next 4 bytes
			U32* p32 = (U32*)(p + 68);
			*p32 = task->msgLen;

			// mask the 8 bytes to make it a little bit harder for the attacker :-)
			q = p + 64;
			for (i = 0; i < 8; i++) q[i] ^= mask[i];

			if (offset) // the original message is less than 252 bytes, we fill the random data in the 252 bytes
				wt_FillRandomData(p + 72, length_raw - 72);

			// save the original message
			memcpy(p + 72 + idx, task->message, task->msgLen);

			wt_chacha20_init(&cxt);
			wt_chacha20_setkey(&cxt, Ks); // use the session key as the key for Chacha20 encryption algorithm.
			for (i = 0; i < 12; i++) nonce[i] = i;
			wt_chacha20_starts(&cxt, nonce, 0);
			// encrypt the message except the first 32 bytes, because the first 32 bytes contain the encrypted session key
			wt_chacha20_update(&cxt, length_raw - 32, (const unsigned char*)(p+32), p + 32);  
			wt_chacha20_free(&cxt);

			// we complete the raw message, so conver the raw message to base64 encoding string
			length_b64 = wt_b64_enc_len(length_raw);
			msssage_b64 = (U8*)wt_palloc(pool, 89 + length_b64); 
			if (msssage_b64)
			{
				int rc;
				p = msssage_b64;
				// the first 44 bytes are the public key of the sender
				wt_b64_encode((const char*)g_PK, 33, (char*)p, 44);
				// the second 44 bytes are the public key of the receiver
				wt_b64_encode((const char*)node->pubkey, 33, (char*)(p+44), 44);
			
				p[88] = '|';  // this is the seperator character to indicate the message type

				// convert the raw message to base64 encoding string
				rc = wt_b64_encode((const char*)message_raw, length_raw, (char*)(p + 89), length_b64);
				if (rc == length_b64)
				{
					bool Successful = false;
					// the receiver's public key is also the topic of MQTT 
					wt_Raw2HexString(node->pubkey, 33, topic, nullptr); 

					// send out the message by using MQTT protocol
					if (MOSQ_ERR_SUCCESS == mosquitto_publish_v5(mosq, NULL, (const char*)topic, length_b64 + 89, msssage_b64, 1, false, NULL))
					{
						Successful = true;
						InterlockedIncrement(&(node->state)); // we have processed this task
					}
					InsertIntoSendMsgTable(msssage_b64, length_b64 + 89, hash,Successful); // log the message into SQLite database
				}
				wt_pfree(msssage_b64);
			}
			wt_pfree(message_raw);
		}
	}

	if (node->type == 'C') // confirmation message
	{
#if 0
		length_b64 = 44 + 44 + 1 + 44 + 1; // zero terminated
		msssage_b64 = (U8*)wt_palloc(pool, length_b64); 
		if (msssage_b64)
		{
			U8  Xor;
			U32 crcKp;
			AES256_ctx ctxAES = { 0 };
			U8 p[33];

			{
				INIT_CRC32C(crcKp);
				COMP_CRC32C(crcKp, Kp, 32);
				FIN_CRC32C(crcKp);

				U8* q = (U8*)&crcKp;
				for (int i = 0; i < 4; i++)
				{
					Xor = q[i];
					if (Xor)
						break;
				}
				if (0 == Xor)
					Xor = 0xFF;

				p[0] = 'C' ^ Xor;
			}

			wt_AES256_init(&ctxAES, Kp);
			wt_AES256_encrypt(&ctxAES, 2, p + 1, node->hash);

			// the first 44 bytes are the public key of the sender
			wt_b64_encode((const char*)g_PK, 33, (char*)msssage_b64, 44);
			// the second 44 bytes are the public key of the receiver
			wt_b64_encode((const char*)node->pubkey, 33, (char*)(msssage_b64 + 44), 44);

			msssage_b64[88] = '#';

			wt_b64_encode((const char*)p, 33, (char*)(msssage_b64 + 89), 44);

			msssage_b64[length_b64 - 1] = '\0';

			// the receiver's public key is also the topic of MQTT 
			wt_Raw2HexString(node->pubkey, 33, topic, nullptr);

			// send out the message by using MQTT protocol
			if (MOSQ_ERR_SUCCESS == mosquitto_publish_v5(mosq, NULL, (const char*)topic, length_b64, msssage_b64, 1, false, NULL))
			{
				InterlockedIncrement(&(node->state)); // we have processed this task
			}
			wt_pfree(msssage_b64);
		}
#endif 
	}

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
}

DWORD WINAPI MQTTPubThread(LPVOID lpData)
{
	int rc = 0;
	DWORD dwRet;
	HWND hWndUI;
	struct mosquitto* mosq = nullptr;
	MemoryPoolContext mempool;
	PublishTask* task;
	PublishTask* p;

	InterlockedIncrement(&g_threadCount);
	hWndUI = (HWND)(lpData);
	ATLASSERT(::IsWindow(hWndUI));

	mempool = wt_mempool_create("MQTT_PUB_POOL", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	if (nullptr == mempool)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 1, 0);
		goto QuitMQTTPubThread;
	}

	ng_seqnumber = wt_GenRandomU32(0x7FFFFFFF);

	mosq = MQTT::MQTT_CreateInstance(mempool, hWndUI, (char*)g_MQTTPubClientId, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, &mqtt_callback, true);

	if (nullptr == mosq)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 2, 0);
		goto QuitMQTTPubThread;
	}

	while (true)
	{
		dwRet = WaitForSingleObject(g_MQTTPubEvent, 1000); // wakeup every 1 seconds

		if (g_Quit) // we will quit the whole program
			break;

		if ((dwRet != WAIT_OBJECT_0) && (dwRet != WAIT_TIMEOUT))
			continue;

		task = CloneInputMessageTasks(mempool);
		if (task) // we have the tasks in the queue that need to send out.
		{
			rc = mosquitto_connect_bind_v5(mosq, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, 60, NULL, NULL);
			if (MOSQ_ERR_SUCCESS != rc)
			{
				InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_BAD); // if we cannot connect, the network is not good
				wt_mempool_reset(mempool); // clean up the memory pool
				continue;
			}

			InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_GOOD); // the network is good

			p = task;
			while (p)
			{
				SendOutPushlishTask(mosq, mempool, p);
				p = p->next;
			}
			mosquitto_disconnect_v5(mosq, 0, NULL);
			wt_mempool_reset(mempool); // clean up the memory pool
			PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 0, 0); // tell the UI thread that we send out some message
		}
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
			InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_GOOD);
			break;
		}
		else
		{
			InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_BAD); // if we cannot connect, the network is not good
			Sleep(1000); // wait for 1 second to try
		}
	}

	while (0 == g_Quit) // the main loop
	{
		for (;;)
		{
			if (g_Quit)
				break;
			ret = mosquitto_loop(mosq, -1, 1);
			if (ret == MOSQ_ERR_SUCCESS)
			{
				InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_GOOD);
				continue;
			}
			InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_BAD);
			break;
		}

		if (g_Quit)
			break;

		do 
		{
			InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_BAD);
			ret = mosquitto_reconnect(mosq);
		} while (0 == g_Quit && ret != MOSQ_ERR_SUCCESS);

		if (g_Quit)
			break;

		if (MOSQ_ERR_SUCCESS == ret)  // because we can connect successfully, the network is good
			InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_GOOD);
		else
			InterlockedExchange(&g_NetworkStatus, WT_NETWORK_IS_BAD);
	}

QuitMQTTSubThread:
	MQTT::MQTT_DestroyInstance(mosq);
	mosq = nullptr;
	wt_mempool_destroy(mempool);
	InterlockedDecrement(&g_threadCount);

	return 0;
}

extern "C"
{
	static void MQTT_Message_Callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message, const mosquitto_property* properties)
	{
		int rc;
		HWND hWndUI;
		MemoryPoolContext pool;
		U8 Kp[32] = { 0 };
		U8 pkSender[33] = { 0 };
		U8 pkReceiver[33] = { 0 };
		U8* msssage_b64;
		U32 length_b64;
		MQTTConf* conf = (MQTTConf*)obj;
		assert(conf);
		pool = conf->ctxThread;
		assert(pool);

		UNUSED(properties);

		hWndUI = (HWND)conf->hWnd;
		assert(::IsWindow(hWndUI));

		msssage_b64 = (U8*)message->payload;
		length_b64 = (U32)message->payloadlen;

		// do some pre-check at first 
		if (length_b64 < 134) // this is the minimal size for the packet
			return;

		if (msssage_b64[88] != '|' && msssage_b64[88] != '#') // the 88th bytes is spearated character
			return;

		rc = wt_b64_decode((const char*)msssage_b64, 44, (char*)pkSender, 33);
		if (rc != 33)
			return;
		rc = wt_b64_decode((const char*)(msssage_b64 + 44), 44, (char*)pkReceiver, 33);
		if (rc != 33)
			return;
		if (memcmp(pkReceiver, g_PK, 33))
			return;

		if (WT_OK != GetKeyFromSecretKeyAndPlubicKey(g_SK, pkSender, Kp))
			return;

		if (msssage_b64[88] == '|') // this is a common message packet
		{
			int rc;
			U8* message_raw;
			U32 length_raw;

			length_raw = wt_b64_dec_len(length_b64 - 89);
			message_raw = (U8*)wt_palloc(pool, length_raw);
			if (message_raw)
			{
				U8 type, version, offset, reserved;
				U32 i, j, length;
				U8 Ks[32] = { 0 };
				U8 nonce[12] = { 0 };
				U8 hash[32] = { 0 };
				U8 hash_org[32] = { 0 };
				U8 mask[8] = { 0 };
				U8* q;

				wt_chacha20_context cxt = { 0 };
				AES256_ctx ctxAES0 = { 0 };
				AES256_ctx ctxAES1 = { 0 };

				U8* p = message_raw;
				rc = wt_b64_decode((const char*)(msssage_b64 + 89), length_b64 - 89, (char*)message_raw, length_raw);
				if (rc < 0)
				{
					wt_pfree(message_raw);
					InsertIntoAbadonMsgTable(msssage_b64, length_b64);
					return;
				}

				wt_AES256_init(&ctxAES0, Kp);
				wt_AES256_decrypt(&ctxAES0, 2, Ks, p); // get the session key at first

				wt_chacha20_init(&cxt);
				wt_chacha20_setkey(&cxt, Ks);
				for (i = 0; i < 12; i++) nonce[i] = i;
				wt_chacha20_starts(&cxt, nonce, 0);
				wt_chacha20_update(&cxt, length_raw - 32, (const unsigned char*)(p+32), p+32);  // decrypt the message!
				wt_chacha20_free(&cxt);

				for (i = 0; i < 4; i++)
				{
					q = p + 32 + i * 8;
					for (j = 0; j < 8; j++) mask[i] ^= q[i];
				}

				wt_AES256_init(&ctxAES1, Kp);
				wt_AES256_decrypt(&ctxAES1, 2, hash, p+32);

				q = p + 64;
				for (i = 0; i < 8; i++) q[i] ^= mask[i];

				version  = p[64];
				type     = p[65];
				offset   = p[66];
				reserved = p[67];
				length = *((U32*)(p + 68)); // the length of the original message

				q = p + 72 + offset; // now p is pointing to the real message
				wt_sha256_hash(q, length, hash_org);

				if (0 == memcmp(hash, hash_org, 32)) // check the hash value is the same
				{
					InsertIntoGoodMsgTable(msssage_b64, length_b64, hash);
					MessageTask* task = (MessageTask*)wt_palloc0(pool, sizeof(MessageTask));
					if (task)
					{
						U32 utf16len;
						memcpy(task->pubkey, pkSender, 33);
						memcpy(task->hash, hash, 32);
						task->type = type;
						// convert UTF8 to UTF 16
						if (wt_UTF8ToUTF16(q + 4, length - 4, nullptr, &utf16len) == WT_OK)
						{
							task->msgLen = utf16len * sizeof(wchar_t);
							task->message = (U8*)wt_palloc(pool, task->msgLen);
							if (task->message)
							{
								wt_UTF8ToUTF16(q + 4, length - 4, (U16*)task->message, nullptr);
								PushTaskIntoReceiveMessageQueue(task);
								::PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 0, 0); // tell the UI thread that a new message is received
							}
							else wt_pfree(task);
						}
						else wt_pfree(task);
					}
				}
				else InsertIntoAbadonMsgTable(msssage_b64, length_b64);

				wt_pfree(message_raw);
			}
			else InsertIntoAbadonMsgTable(msssage_b64, length_b64);
		}

		if (msssage_b64[88] == '#') // this is a common message packet
		{
#if 0
			U8  Xor;
			U32 crcKp;
			AES256_ctx ctxAES = { 0 };
			U8 p[33];
			U8 hash[32];

			rc = wt_b64_decode((const char*)(msssage_b64 + 89), 44, (char*)p, 33);
			if (rc == 33)
			{
				INIT_CRC32C(crcKp);
				COMP_CRC32C(crcKp, Kp, 32);
				FIN_CRC32C(crcKp);

				U8* q = (U8*)&crcKp;
				for (int i = 0; i < 4; i++)
				{
					Xor = q[i];
					if (Xor)
						break;
				}
				if (0 == Xor)
					Xor = 0xFF;

				p[0] ^= Xor;

				wt_AES256_init(&ctxAES, Kp);
				wt_AES256_decrypt(&ctxAES, 2, hash, p+1);

				if (p[0] == 'C')
				{
					MessageTask* task = (MessageTask*)wt_palloc0(pool, sizeof(MessageTask));
					if (task)
					{
						memcpy(task->pubkey, pkSender, 33);
						memcpy(task->hash, hash, 32);
						task->type = 'C';
						PushTaskIntoReceiveMessageQueue(task);
						::PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 0, 0); // tell the UI thread that a new message is received
					}
				}
			}
#endif
		}
	}

	static void MQTT_Connect_Callback(struct mosquitto* mosq, void* obj, int result, int flags, const mosquitto_property* properties)
	{
		MQTTConf* conf = (MQTTConf*)obj;
		assert(conf);

		UNUSED(flags);
		UNUSED(properties);

		if (!result)
		{
			if (!conf->pub_or_sub)
			{
				if (conf->topic_count > 0)
					mosquitto_subscribe_multiple(mosq, NULL, conf->topic_count, conf->topics, conf->qos, conf->sub_opts, conf->subscribe_props);
			}
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


// we have the 32-byte secret key, we want to generate the public key, return 0 if successful
U32 GenPublicKeyFromSecretKey(U8* sk, U8* pk)
{
	U32 ret = WT_FAIL;
	secp256k1_context* ctx;

	ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (ctx)
	{
		int return_val;
		size_t len = 33;
		U8 compressed_pubkey[33];
		U8 randomize[32];
		secp256k1_pubkey pubkey;

		if (WT_OK == wt_GenerateNewSecretKey(randomize))
		{
			return_val = secp256k1_ec_pubkey_create(ctx, &pubkey, sk);
			if (1 == return_val)
			{
				return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey, &len, &pubkey, SECP256K1_EC_COMPRESSED);
				if (len == 33 && return_val == 1 && pk)
				{
					for (int i = 0; i < 33; i++) pk[i] = compressed_pubkey[i];
					ret = WT_OK;
				}
			}
		}
		secp256k1_context_destroy(ctx);
	}
	return ret;
}

// put a message node into the send message queue
int PushTaskIntoSendMessageQueue(MessageTask* mt)
{
	MessageTask* p;
	MessageTask* q;

	bool hooked = false;

	EnterCriticalSection(&g_csMQTTPubUI);
	if (nullptr == g_PubData.task) // there is no task in the queue
	{
		g_PubData.task = mt;
		LeaveCriticalSection(&g_csMQTTPubUI);
		SetEvent(g_MQTTPubEvent); // tell the Pub thread to work
		return 0;
	}

	q = g_PubData.task; // the first node in this queue
	p = g_PubData.task;

	while (p) // try to find the first node that is not processed yet
	{
		if (p->state < MESSAGE_TASK_STATE_COMPLETE) // this task is not processed
			break;
		p = p->next;
	}

	g_PubData.task = p;  // here, g_PubData.task is pointing to a node not procesded or NULL
	if (nullptr == g_PubData.task) // there is no not-processed task in the queue
	{
		g_PubData.task = mt;
	}

	p = q; // p is pointing to the first node of the queue
	while (p)
	{
		q = p->next;
		if (nullptr == q && !hooked) // we reach the end of the queue, put the new node at the end of the queue
		{
			p->next = mt;
			hooked = true;
		}

		if (p->state == MESSAGE_TASK_STATE_COMPLETE) // this task has been processed, we can free the memory
		{
			wt_pfree(p->message);
			wt_pfree(p);
		}
		p = q;
	}
	LeaveCriticalSection(&g_csMQTTPubUI);

	if(mt)
		SetEvent(g_MQTTPubEvent); // tell the Pub thread to work

	return 0;
}

static const U8 txtUtf8Name[]   = { 0xE9,0xBB,0x84,0xE5,0xA4,0xA7,0xE4,0xBB,0x99,0 };
static const U8 txtUf8Motto[]  = { 0xE4,0xB8,0x80,0xE5,0x88,0x87,0xE7,0x9A,0x86,0xE6,0x9C,0x89,0xE5,0x8F,0xAF,0xE8,0x83,0xBD,0 };
static const U8 txtUtf8Area[]   = { 0xE5,0xAE,0x87,0xE5,0xAE,0x99,0xE8,0xB5,0xB7,0xE7,0x82,0xB9,0 };
static const U8 txtUtf8Source[] = { 0xE7,0xB3,0xBB,0xE7,0xBB,0x9F,0xE8,0x87,0xAA,0xE5,0xB8,0xA6,0 };

U32 CheckWoChatDatabase(LPCWSTR lpszPath)
{
	U32 ret = WT_FAIL;
	int rc;
	bool bAvailabe = false;
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;

	WIN32_FILE_ATTRIBUTE_DATA fileInfo;

	if (GetFileAttributesExW(lpszPath, GetFileExInfoStandard, &fileInfo) != 0)
	{
		if (!(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			bAvailabe = true;
	}

	rc = sqlite3_open16(lpszPath, &db);
	if (SQLITE_OK != rc)
	{
		sqlite3_close(db);
		return WT_SQLITE_OPEN_ERR;
	}

	if (!bAvailabe) // it is the first time to create the DB, so we create the table structure
	{
		char sql[SQL_STMT_MAX_LEN + 1] = { 0 };
		
		/* create the table at first */
		rc = sqlite3_prepare_v2(db,
			(const char*)"CREATE TABLE c(id INTEGER PRIMARY KEY,tp INTEGER NOT NULL,i0 INTEGER,i1 INTEGER,tx TEXT,bb BLOB)",
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		rc = sqlite3_prepare_v2(db,
			(const char*)"CREATE TABLE k(id INTEGER PRIMARY KEY AUTOINCREMENT,pt INTEGER NOT NULL,at INTEGER DEFAULT 1,dt INTEGER,sk CHAR(64) NOT NULL UNIQUE,pk CHAR(66) NOT NULL UNIQUE,nm TEXT NOT NULL UNIQUE,bt INTEGER DEFAULT 0,sx INTEGER DEFAULT 9,mt TEXT,fm TEXT,b0 BLOB,b1 BLOB)",
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		rc = sqlite3_prepare_v2(db,	
			(const char*)"CREATE TABLE p(id INTEGER PRIMARY KEY AUTOINCREMENT,dt INTEGER,at INTEGER DEFAULT 1,pk CHAR(66) NOT NULL UNIQUE,nm TEXT NOT NULL,bt INTEGER DEFAULT 0,sx INTEGER DEFAULT 9,mt TEXT,fm TEXT,sr TEXT,b0 BLOB,b1 BLOB,b2 BLOB)", 
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		rc = sqlite3_prepare_v2(db,
			(const char*)"CREATE TABLE m(id INTEGER PRIMARY KEY AUTOINCREMENT,dt INTEGER,st INTEGER,hs CHAR(64) NOT NULL UNIQUE,sd CHAR(64) NOT NULL,re CHAR(64) NOT NULL,tp CHAR(1),tx TEXT)", 
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		rc = sqlite3_prepare_v2(db,
			(const char*)"CREATE TABLE q(id INTEGER PRIMARY KEY AUTOINCREMENT,dt INTEGER,tx TEXT)", 
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		/* insert some initial data into the tables */
		sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "INSERT INTO c(id,tp,tx) VALUES(%d,1,'WoChat')", WT_PARAM_APPLICATION_NAME);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "INSERT INTO c(id,tp,tx) VALUES(%d,1,'www.boobooke.com')", WT_PARAM_SERVER_NAME);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "INSERT INTO c(id,tp,i0) VALUES(%d,0,1883)", WT_PARAM_SERVER_PORT);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "INSERT INTO c(id,tp,tx) VALUES(%d,1,'MQTT')", WT_PARAM_NETWORK_PROTOCAL);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);

		rc = sqlite3_prepare_v2(db,
			(const char*)"INSERT INTO p(pk,nm,mt,fm,sr,b0,b1) VALUES('03339A1C8FDB6AFF46845E49D110E0400021E16146341858585C2E25CA399C01CA',(?),(?),(?),(?),(?),(?))",
			-1, &stmt, NULL);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_bind_text(stmt, 1, (const char*)txtUtf8Name, 9, SQLITE_TRANSIENT);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_bind_text(stmt, 2, (const char*)txtUf8Motto, 18, SQLITE_TRANSIENT);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_bind_text(stmt, 3, (const char*)txtUtf8Area, 12, SQLITE_TRANSIENT);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_bind_text(stmt, 4, (const char*)txtUtf8Source, 12, SQLITE_TRANSIENT);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		assert(g_aiImage32);
		assert(g_aiImage128);
		rc = sqlite3_bind_blob(stmt, 5, g_aiImage32, (MY_ICON_SIZE32 * MY_ICON_SIZE32 * sizeof(U32)), SQLITE_TRANSIENT);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_bind_blob(stmt, 6, g_aiImage128, (MY_ICON_SIZE128 * MY_ICON_SIZE128 * sizeof(U32)), SQLITE_TRANSIENT);
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_DONE != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);
	}
	else
	{
		rc = sqlite3_prepare_v2(db, (const char*)"SELECT count(1) FROM k", -1, &stmt, NULL); 
		if (SQLITE_OK != rc) goto Exit_CheckWoChatDatabase;
		rc = sqlite3_step(stmt);
		if (SQLITE_ROW != rc) goto Exit_CheckWoChatDatabase;
		sqlite3_finalize(stmt);
	}

	ret = WT_OK;

Exit_CheckWoChatDatabase:
	sqlite3_close(db);
	return ret;
}

int SendConfirmationMessage(U8* pk, U8* hash)
{
	MessageTask* mt = (MessageTask*)wt_palloc0(g_messageMemPool, sizeof(MessageTask));
	if (mt)
	{
		mt->type = 'C'; // confirmation
		mt->state = MESSAGE_TASK_STATE_ONE;
		memcpy(mt->pubkey, pk, 33);
		memcpy(mt->hash, hash, 32);
		PushTaskIntoSendMessageQueue(mt);
	}
	return 0;
}

U32 GetSecretKeyNumber(U32* total)
{
	U32 ret = WT_FAIL;
	int count = 0;
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;
	int  rc;

	rc = sqlite3_open16(g_DBPath, &db);
	if (SQLITE_OK != rc)
	{
		sqlite3_close(db);
		return WT_SQLITE_OPEN_ERR;
	}

	rc = sqlite3_prepare_v2(db, (const char*)"SELECT count(1) FROM k WHERE pt=0 AND at<>0", -1, &stmt, NULL);
	if (SQLITE_OK != rc) goto Exit_GetSecretKeyNumber;
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW) goto Exit_GetSecretKeyNumber;
	count = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);

	if (total)
		*total = (U32)count;

	ret = WT_OK;

Exit_GetSecretKeyNumber:
	sqlite3_close(db);

	return ret;
}

U32 CreateNewAccount(wchar_t* name, U8 nlen, wchar_t* pwd, U8 plen, U32* skIdx)
{
	int rc;
	U8 tries;
	U32 ret = WT_FAIL;
	U32 len = 0;
	U8 randomize[32];
	U8 sk[32] = { 0 };
	U8 skEncrypted[32] = { 0 };
	U8 pk[33] = { 0 };
	U8 hash[32] = { 0 };
	U8 hexSK[65] = { 0 };
	U8 hexPK[67] = { 0 };
	U8* utf8Name;
	U8* utf8Password;
	AES256_ctx ctxAES = { 0 };

	secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (!ctx)
		return WT_SECP256K1_CTX_ERROR;

	/* Randomizing the context is recommended to protect against side-channel
	 * leakage See `secp256k1_context_randomize` in secp256k1.h for more
	 * information about it. This should never fail. */
	if (WT_OK == wt_GenerateNewSecretKey(randomize))
	{
		rc = secp256k1_context_randomize(ctx, randomize);
		assert(rc);
	}
	else
	{
		secp256k1_context_destroy(ctx);
		return WT_RANDOM_NUMBER_ERROR;
	}

	// we try to generate a new secret key
	tries = 255;
	while (tries)
	{
		ret = wt_GenerateNewSecretKey(sk);
		rc = secp256k1_ec_seckey_verify(ctx, sk);
		if (WT_OK == ret && 1 == rc)
			break;
		tries--;
	}

	if (0 == tries) // we cannot generate the 32 bytes secret key
	{
		secp256k1_context_destroy(ctx);
		return WT_SK_GENERATE_ERR;
	}

	ret = WT_FAIL;  // now we get the SK, we need to generate the PK

	{
		secp256k1_pubkey pubkey;
		size_t pklen = 33;
		rc = secp256k1_ec_pubkey_create(ctx, &pubkey, sk);
		if (1 == rc)
		{
			rc = secp256k1_ec_pubkey_serialize(ctx, pk, &pklen, &pubkey, SECP256K1_EC_COMPRESSED);
			if (pklen == 33 && rc == 1)
				ret = WT_OK;
		}
	}
	/* This will clear everything from the context and free the memory */
	secp256k1_context_destroy(ctx);

	if (WT_OK != ret)
		return WT_PK_GENERATE_ERR;

	ret = wt_UTF16ToUTF8((U16*)pwd, plen, nullptr, &len);
	if (WT_OK != ret)
		return WT_UTF16ToUTF8_ERR;

	utf8Password = (U8*)wt_palloc(g_messageMemPool, len);
	if (utf8Password)
	{
		ret = wt_UTF16ToUTF8((U16*)pwd, plen, utf8Password, nullptr);
		if (WT_OK != ret)
		{
			wt_pfree(utf8Password);
			return WT_UTF16ToUTF8_ERR;
		}
		wt_sha256_hash(utf8Password, len, hash); // we use the UTF8 password to hash the key to encrypt the secret key
		wt_pfree(utf8Password);
	}
	else
		return WT_MEMORY_ALLOC_ERR;

	ret = wt_UTF16ToUTF8((U16*)name, nlen, nullptr, &len);
	if (WT_OK != ret)
		return WT_UTF16ToUTF8_ERR;

	utf8Name = (U8*)wt_palloc(g_messageMemPool, len+1);
	if (utf8Name)
	{
		ret = wt_UTF16ToUTF8((U16*)name, nlen, utf8Name, nullptr);
		if (WT_OK != ret)
		{
			wt_pfree(utf8Name);
			return WT_UTF16ToUTF8_ERR;
		}
		utf8Name[len] = '\0';
	}
	else
		return WT_MEMORY_ALLOC_ERR;

	wt_AES256_init(&ctxAES, hash);
	wt_AES256_encrypt(&ctxAES, 2, skEncrypted, sk);
	wt_Raw2HexString(skEncrypted, 32, hexSK, nullptr);
	wt_Raw2HexString(pk, 33, hexPK, nullptr);

	ret = WT_FAIL;
	{
		sqlite3* db;
		sqlite3_stmt* stmt = NULL;
		U8 sql[SQL_STMT_MAX_LEN] = { 0 };

		rc = sqlite3_open16(g_DBPath, &db);
		if (SQLITE_OK != rc)
		{
			sqlite3_close(db);
			return WT_SQLITE_OPEN_ERR;
		}

		sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "INSERT INTO k(pt,dt,sk,pk,nm,b0,b1) VALUES(0,(?),'%s','%s',(?),(?),(?))", hexSK, hexPK);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK == rc)
		{
			S64 tm = GetCurrentUTCTime64();
			rc = sqlite3_bind_int64(stmt, 1, tm);
			if (SQLITE_OK == rc)
			{
				rc = sqlite3_bind_text(stmt, 2, (const char*)utf8Name, (int)len, SQLITE_TRANSIENT);
				if (SQLITE_OK == rc)
				{
					rc = sqlite3_bind_blob(stmt, 3, g_myImage32, (MY_ICON_SIZE32 * MY_ICON_SIZE32 * sizeof(U32)), SQLITE_TRANSIENT);
					if (SQLITE_OK == rc)
					{
						rc = sqlite3_bind_blob(stmt, 4, g_myImage128, (MY_ICON_SIZE128 * MY_ICON_SIZE128 * sizeof(U32)), SQLITE_TRANSIENT);
						if (SQLITE_OK == rc)
						{
							rc = sqlite3_step(stmt);
							if (SQLITE_DONE == rc)
							{
								sqlite3_finalize(stmt);
								sprintf_s((char*)sql, SQL_STMT_MAX_LEN, "SELECT id FROM k WHERE sk='%s' AND pk='%s'", hexSK, hexPK);
								rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
								if (SQLITE_OK == rc)
								{
									rc = sqlite3_step(stmt);
									if (SQLITE_ROW == rc)
									{
										U32 Idx = sqlite3_column_int(stmt, 0);
										if (skIdx)
											*skIdx = Idx;
										ret = WT_OK;
										sqlite3_finalize(stmt);
										sqlite3_close(db);
										goto Exit_NewAccount;
									}
								}
							}
						}
					}
				}
			}
		}
		sqlite3_close(db);
	}

Exit_NewAccount:
	wt_pfree(utf8Name);
	return ret;
}

U32 OpenAccount(U32 idx, U16* pwd, U32 len)
{
	U32 ret = WT_FAIL;
	U32 utf8len;
	U8* utf8Password;
	U8 hash[32];

	ret = wt_UTF16ToUTF8(pwd, len, nullptr, &utf8len);
	if (WT_OK != ret)
		return WT_UTF16ToUTF8_ERR;

	utf8Password = (U8*)wt_palloc(g_messageMemPool, utf8len);
	if (utf8Password)
	{
		ret = wt_UTF16ToUTF8(pwd, len, utf8Password, nullptr);
		if (WT_OK != ret)
		{
			wt_pfree(utf8Password);
			return WT_UTF16ToUTF8_ERR;
		}
		wt_sha256_hash(utf8Password, len, hash); // we use the UTF8 password to hash the key to encrypt the secret key
		wt_pfree(utf8Password);
	}
	else
		return WT_MEMORY_ALLOC_ERR;

	ret = WT_FAIL;
	{
		sqlite3* db;
		sqlite3_stmt* stmt = NULL;
		int  rc;
		U8 sql[64] = { 0 };

		rc = sqlite3_open16(g_DBPath, &db);
		if (SQLITE_OK != rc)
		{
			sqlite3_close(db);
			return WT_SQLITE_OPEN_ERR;
		}
		sprintf_s((char*)sql, 64, "SELECT sk,pk,nm,b0,b1,dt FROM k WHERE id=%d", idx);
		rc = sqlite3_prepare_v2(db, (const char*)sql, -1, &stmt, NULL);
		if (SQLITE_OK == rc)
		{
			rc = sqlite3_step(stmt);
			if (SQLITE_ROW == rc)
			{
				U8* hexSK = (U8*)sqlite3_column_text(stmt, 0);
				U8* hexPK = (U8*)sqlite3_column_text(stmt, 1);
				U8* utf8Name = (U8*)sqlite3_column_text(stmt, 2);
				U8* img32 = (U8*)sqlite3_column_blob(stmt, 3);
				int bytes32 = sqlite3_column_bytes(stmt, 3);
				U8* img128 = (U8*)sqlite3_column_blob(stmt, 4);
				int bytes128 = sqlite3_column_bytes(stmt, 4);
				S64 tm = sqlite3_column_int64(stmt, 5);
#if 0
				{
					wchar_t ts[32+1] = { 0 };
					GetU64TimeStringW(tm, ts, 32);
					tm++;
				}
#endif
				size_t skLen = strlen((const char*)hexSK);
				size_t pkLen = strlen((const char*)hexPK);
				if (skLen == 64 && pkLen == 66)
				{
					if (wt_IsHexString(hexSK, skLen) && wt_IsPublicKey(hexPK, pkLen))
					{
						U8 sk[32];
						U8 skEncrypted[32];
						U8 pk0[33];
						U8 pk1[33];

						AES256_ctx ctxAES = { 0 };

						wt_HexString2Raw(hexSK, 64, skEncrypted, nullptr);
						wt_HexString2Raw(hexPK, 66, pk0, nullptr);

						wt_AES256_init(&ctxAES, hash);
						wt_AES256_decrypt(&ctxAES, 2, sk, skEncrypted);

						if (WT_OK == GenPublicKeyFromSecretKey(sk, pk1))
						{
							if (0 == memcmp(pk0, pk1, 33)) // the public key is matched! so we are good
							{
								U8* icon;
								U32 utf16len;
								ret = WT_OK;

								assert(g_SK);
								assert(g_PK);

								memcpy(g_SK, sk, 32);
								memcpy(g_PK, pk0, 33);

								wt_sha256_hash(g_SK, 32, hash);
								wt_Raw2HexString(hash, 11, g_MQTTPubClientId, nullptr); // The MQTT client Id is 23 bytes long
								wt_Raw2HexString(hash + 16, 11, g_MQTTSubClientId, nullptr); // The MQTT client Id is 23 bytes long

								size_t utf8len = strlen((const char*)utf8Name);
								U32 status = wt_UTF8ToUTF16(utf8Name, (U32)utf8len, nullptr, &utf16len);
								if (WT_OK == status)
								{
									g_myName = (wchar_t*)wt_palloc(g_topMemPool, (utf16len + 1) * sizeof(wchar_t));
									if (g_myName)
									{
										wt_UTF8ToUTF16(utf8Name, (U32)utf8len, (U16*)g_myName, nullptr);
										g_myName[utf16len] = L'\0';
									}
								}
								if (bytes32 == MY_ICON_SIZE32 * MY_ICON_SIZE32 * sizeof(U32))
								{
									icon = (U8*)wt_palloc0(g_topMemPool, bytes32);
									if (icon)
									{
										memcpy(icon, img32, bytes32);
										g_myImage32 = icon;
									}
								}
								if (bytes128 == MY_ICON_SIZE128 * MY_ICON_SIZE128 * sizeof(U32))
								{
									icon = (U8*)wt_palloc0(g_topMemPool, bytes128);
									if (icon)
									{
										memcpy(icon, img128, bytes128);
										g_myImage128 = icon;
									}
								}
							}
						}
					}
				}
			}
			sqlite3_finalize(stmt);
		}
		sqlite3_close(db);
	}

	return ret;
}

DWORD WINAPI LoadingDataThread(LPVOID lpData)
{
	U8 percentage;
	HWND hWndUI = (HWND)(lpData);
	ATLASSERT(::IsWindow(hWndUI));

	InterlockedIncrement(&g_threadCount);

	percentage = 0;
	while (0 == g_Quit)
	{
		Sleep(100);

		PostMessage(hWndUI, WM_LOADPERCENTMESSAGE, (WPARAM)percentage, 0);

		percentage += 5;
		if (percentage >= 110)
			break;
	}

	InterlockedDecrement(&g_threadCount);

	return 0;
}
