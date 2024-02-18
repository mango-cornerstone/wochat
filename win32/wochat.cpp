#include "pch.h"
#include "mosquitto.h"
#include "wochat.h"
#include "wt_base64.h"
#include "wt_utils.h"
#include "wt_sha256.h"
#include "wt_chacha20.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"

#define WT_NETWORK_IS_BAD		0
#define WT_NETWORK_IS_GOOD		1

typedef struct PublishTask
{
	PublishTask* next;
	MessageTask* node;
} PublishTask;

static PublishTask* CloneInputMessageTasks(MemoryPoolContext mempool)
{
	PublishTask* pt = nullptr;
	
	EnterCriticalSection(&g_csMQTTPub);
	if (g_PubData.task)
	{
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
	LeaveCriticalSection(&g_csMQTTPub);

	return pt;
}

static int PushTaskIntoReceiveMessageQueue(MessageTask* task)
{
	int ret = 0;
	MQTTSubData* pd = &g_SubData;
	MessageTask* p;

	EnterCriticalSection(&g_csMQTTSub);

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
		LeaveCriticalSection(&g_csMQTTSub);
		return 0;
	}

	p = pd->task;
	while (p->next)
		p = p->next;

	p->next = task;
	task->next = nullptr;

	LeaveCriticalSection(&g_csMQTTSub);

	return 0;
}

static int GetKeyFromSecretKeyAndPlubicKey(U8* sk, U8* pk, U8* key)
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

static int SendOutPushlishTask(struct mosquitto* mosq, MemoryPoolContext pool, PublishTask* task)
{
	assert(task);
	MessageTask* node = task->node;

	if (node)
	{
		U8 offset = 0;
		U8* message_raw;
		U8* msssage_b64;
		U32 length_raw;
		U32 length_b64;

		if (node->msgLen < 256) // we keep the minimal size to 256 bytes to make it harder for the attacker
		{
			length_raw = 256 + 40;
			offset = (U8)(256 - node->msgLen);
		}
		else
			length_raw = node->msgLen + 40; // if the message is equal or more than 256 bytes, we do not make random data

		message_raw = (U8*)wt_palloc(pool, length_raw);
		if (message_raw)
		{
			U8 idx, salt0, salt1;
			U32 i, crc;
			wt_chacha20_context cxt = { 0 };
			U8 key[32] = { 0 };
			U8 nonce[12] = { 0 };
			U8 topic[67] = { 0 };

			wt_sha256_hash(node->message, node->msgLen, message_raw); // generate the SHA256 hash of the original message
			INIT_CRC32C(crc);
			COMP_CRC32C(crc, message_raw, 32);
			FIN_CRC32C(crc);  // generate the CRC value of the SHA256

			U32* p32 = (U32*)(message_raw + 32);
			*p32 = node->msgLen;
			U8* p = message_raw + 32 + 4;
			*p++ = node->type;

			idx = 0;
			if (offset) // the original message is less than 256 bytes
			{
				idx = wt_GenRandomIntLessThan(offset);  // idx range is 0 .. offset - 1
				wt_FillRandomData(message_raw + 40, length_raw - 40);
			}
			*p = idx;
			memcpy(message_raw + 40 + idx, node->message, node->msgLen);

			p = (U8*)&crc;
			for (i = 0; i < 6; i++)  // we use the CRC32 of SHA256 to mask the length information
				message_raw[32 + i] ^= p[i%4];

			// we use two bytes random data as salt to aovid the same plain texts have the same cipher texts
			wt_FillRandomData(message_raw + 38, 2); // generated two slat bytes
			salt0 = message_raw[38]; 
			salt1 = message_raw[39];

			p = message_raw;  // we use two salts to make the same text to be the different
			for (i = 0; i < 38; i++) p[i] ^= salt0;
			for (i = 40; i < length_raw; i++) p[i] ^= salt1;

			GetKeyFromSecretKeyAndPlubicKey(g_SK, node->pubkey, key);
			wt_chacha20_init(&cxt);
			wt_chacha20_setkey(&cxt, key);
			for (i = 0; i < 12; i++) nonce[i] = i;
			wt_chacha20_starts(&cxt, nonce, 0);
			wt_chacha20_update(&cxt, length_raw, (const unsigned char*)message_raw, message_raw);  // encrypt the message!
			wt_chacha20_free(&cxt);

			length_b64 = wt_b64_enc_len(length_raw);
			msssage_b64 = (U8*)wt_palloc(pool, 67 + length_b64 + 1); // zero terminated
			if (msssage_b64)
			{
				wt_Raw2HexString(g_PK, 33, msssage_b64, nullptr);
				msssage_b64[66] = '|';
				wt_b64_encode((const char*)message_raw, length_raw, (char*)(msssage_b64 + 67), length_b64);
				msssage_b64[length_b64 + 67] = '\0'; // make the last byte to be zero

				wt_Raw2HexString(node->pubkey, 33, topic, nullptr);
				if (MOSQ_ERR_SUCCESS == mosquitto_publish_v5(mosq, NULL, (const char*)topic, length_b64 + 67 + 1, msssage_b64, 1, false, NULL))
				{
					InterlockedIncrement(&(node->state)); // we have processed this task
				}
				wt_pfree(msssage_b64);
			}
			wt_pfree(message_raw);
		}
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
		HWND hWndUI;
		MemoryPoolContext pool;
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
		if (length_b64 < 132) // this is the minimal size for the packet
			return;
		if (msssage_b64[length_b64 - 1]) // the last byte is alway 0. 
			return; 
		if (msssage_b64[66] != '|' && msssage_b64[66] != '+' && msssage_b64[66] != '-') // the 67th bytes is spearated character
			return;
		if (!wt_IsPublicKey(msssage_b64, 66))
			return;

		if (msssage_b64[66] == '|') // this is a common message packet
		{
			U8* message_raw;
			U32 length_raw;
			length_raw = wt_b64_dec_len(length_b64 - 67 - 1);
			message_raw = (U8*)wt_palloc(pool, length_raw);
			if (message_raw)
			{
				U8 type, offset, salt0, salt1;
				U8* p;
				U32 i, crc, length;
				MessageTask* task;
				U8 pubkey[33] = { 0 };
				U8 key[32] = { 0 };
				U8 nonce[12] = { 0 };
				U8 hash[32] = { 0 };
				wt_chacha20_context cxt = { 0 };

				int rc = wt_b64_decode((const char*)msssage_b64 + 67, length_b64 - 67 - 1, (char*)message_raw, length_raw);

				wt_HexString2Raw(msssage_b64, 66, pubkey, nullptr);
				GetKeyFromSecretKeyAndPlubicKey(g_SK, pubkey, key);
				wt_chacha20_init(&cxt);
				wt_chacha20_setkey(&cxt, key);
				for (i = 0; i < 12; i++) nonce[i] = i;
				wt_chacha20_starts(&cxt, nonce, 0);
				wt_chacha20_update(&cxt, length_raw, (const unsigned char*)message_raw, message_raw);  // decrypt the message!
				wt_chacha20_free(&cxt);

				// remove the salt at first
				p = message_raw; 
				salt0 = p[38];
				salt1 = p[39];
				for (i = 0; i < 38; i++) p[i] ^= salt0;
				for (i = 40; i < length_raw; i++) p[i] ^= salt1;

				// the first 32 byte is the SHA256 of original message
				INIT_CRC32C(crc);
				COMP_CRC32C(crc, message_raw, 32);
				FIN_CRC32C(crc);
				p = (U8*)&crc;

				for (i = 0; i < 6; i++)	message_raw[32 + i] ^= p[i%4];

				p = message_raw;
				length = *((U32*)(p + 32)); // the length of the original message
				type = p[36];
				offset = p[37];

				p = message_raw + 40 + offset; // now p is pointing to the real message
				wt_sha256_hash(p, length, hash);

				if (0 == memcmp(hash, message_raw, 32)) // check the hash value is the same
				{
					task = (MessageTask*)wt_palloc0(pool, sizeof(MessageTask));
					if (task)
					{
						memcpy(task->pubkey, pubkey, 33);
						memcpy(task->hash, hash, 32);
						task->type = type;
						task->msgLen = length;
						task->message = (U8*)wt_palloc(pool, length);
						if (task->message)
						{
							memcpy(task->message, p, length);
							PushTaskIntoReceiveMessageQueue(task);
							::PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 0, 0); // tell the UI thread that a new message is received
						}
						else
							wt_pfree(task);
					}
				}
				wt_pfree(message_raw);
			}
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

// put a message node into the send message queue
int PushTaskIntoSendMessageQueue(MessageTask* mt)
{
	MessageTask* p;
	MessageTask* q;

	bool hooked = false;

	EnterCriticalSection(&g_csMQTTPub);
	if (nullptr == g_PubData.task) // there is no task in the queue
	{
		g_PubData.task = mt;
		LeaveCriticalSection(&g_csMQTTPub);
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
	LeaveCriticalSection(&g_csMQTTPub);

	if(mt)
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
