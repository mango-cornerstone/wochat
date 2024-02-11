#include "pch.h"
#include "mosquitto.h"
#include "wochat.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"
#include "mbedtls/chacha20.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"


bool IsHexString(U8* str, U8 len)
{
	bool bRet = false;

	if (str)
	{
		U8 i, oneChar;
		for (i = 0; i < len; i++)
		{
			oneChar = str[i];
			if (oneChar >= '0' && oneChar <= '9')
				continue;
			if (oneChar >= 'A' && oneChar <= 'F')
				continue;
			break;
		}
		if (i == len)
			bRet = true;
	}
	return bRet;
}

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
		pfree(pd->task->message);
		pfree(pd->task);
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
			pfree(p->message);
			pfree(p);
		}
		p = q;
	}

	LeaveCriticalSection(&g_csMQTTPub);

	SetEvent(g_MQTTPubEvent); // tell the Pub thread to work

	return 0;
}

int Raw2HexString(U8* input, U8 len, U8* output, U8* outlen)
{
	U8 idx, i;
	const U8* hex_chars = (const U8*)"0123456789ABCDEF";

	for (i = 0; i < len; i++)
	{
		idx = ((input[i] >> 4) & 0x0F);
		output[(i << 1)] = hex_chars[idx];

		idx = (input[i] & 0x0F);
		output[(i << 1) + 1] = hex_chars[idx];
	}

	output[(i << 1)] = 0;
	if (outlen)
		*outlen = (i << 1);

	return 0;
}

int HexString2Raw(U8* input, U8 len, U8* output, U8* outlen)
{
	U8 oneChar, hiValue, lowValue, i;

	for (i = 0; i < len; i += 2)
	{
		oneChar = input[i];
		if (oneChar >= '0' && oneChar <= '9')
			hiValue = oneChar - '0';
		else if (oneChar >= 'A' && oneChar <= 'F')
			hiValue = (oneChar - 'A') + 0x0A;
		else return 1;

		oneChar = input[i + 1];
		if (oneChar >= '0' && oneChar <= '9')
			lowValue = oneChar - '0';
		else if (oneChar >= 'A' && oneChar <= 'F')
			lowValue = (oneChar - 'A') + 0x0A;
		else return 1;

		output[(i >> 1)] = (hiValue << 4 | lowValue);
	}

	if (outlen)
		*outlen = (len >> 1);

	return 0;
}

int Raw2HexStringW(U8* input, U8 len, wchar_t* output, U8* outlen)
{
	U8 idx, i;
	const wchar_t* hex_chars = (const wchar_t*)(L"0123456789ABCDEF");

	for (i = 0; i < len; i++)
	{
		idx = ((input[i] >> 4) & 0x0F);
		output[(i << 1)] = hex_chars[idx];

		idx = (input[i] & 0x0F);
		output[(i << 1) + 1] = hex_chars[idx];
	}

	output[(i << 1)] = 0;
	if (outlen)
		*outlen = (i << 1);

	return 0;
}

#if 0
int HexString2RawW(wchar_t* input, U8 len, U8* output, U8* outlen)
{
	U8 oneChar, hiValue, lowValue, i;

	for (i = 0; i < len; i += 2)
	{
		oneChar = input[i];
		if (oneChar >= '0' && oneChar <= '9')
			hiValue = oneChar - '0';
		else if (oneChar >= 'A' && oneChar <= 'F')
			hiValue = (oneChar - 'A') + 0x0A;
		else return 1;

		oneChar = input[i + 1];
		if (oneChar >= '0' && oneChar <= '9')
			lowValue = oneChar - '0';
		else if (oneChar >= 'A' && oneChar <= 'F')
			lowValue = (oneChar - 'A') + 0x0A;
		else return 1;

		output[(i >> 1)] = (hiValue << 4 | lowValue);
	}

	if (outlen)
		*outlen = (len >> 1);

	return 0;
}


int AESEncrypt(U8* input, int length, U8* output)
{
	mbedtls_aes_context master;
	mbedtls_aes_init(&master);

	return 0;
}
#endif
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

#if 0
int ConvertBinToHex(U8* pk, U16 len, U8* pkHex)
{
	int ret = 0;

	if (len == 33)
	{
		U8 oneChar;
		for (U16 i = 0; i < len; i++)
		{
			oneChar = (pk[i] >> 4);
			if(oneChar < 0x0A)
				pkHex[(i << 1)] = oneChar + '0';
			else
				pkHex[(i << 1)] = (oneChar - 0x0A) + 'A';

			oneChar = (pk[i] & 0x0F);
			if (oneChar < 0x0A)
				pkHex[(i << 1)+1] = oneChar + '0';
			else
				pkHex[(i << 1)+1] = (oneChar - 0x0A) + 'A';
		}
		pkHex[66] = 0;
	}

	return ret;
}

int GetKeys(LPCTSTR path, U8* sk, U8* pk)
{
	int ret = 1;
	U8  hexSK[32];
	wchar_t keyFile[MAX_PATH + 1] = { 0 };
	
	swprintf((wchar_t*)keyFile, MAX_PATH, L"%s\\keys.txt", path);

	DWORD fileAttributes = GetFileAttributesW(keyFile);
	if (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		int  fd;
		if (0 == _tsopen_s(&fd, keyFile, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0))
		{
			U32  size, bytes;
			U8*  p;
			U8   hiChar, lowChar, hiValue, lowValue;
			U8   buf[256];

			size = (U32)_lseek(fd, 0, SEEK_END); /* get the file size */
			if (size < 131)
			{
				_close(fd);
				return 1;
			}
			_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */
			bytes = (U32)_read(fd, buf, 131);  /* try to detect PNG header */
			if(bytes != 131)
			{
				_close(fd);
				return 1;
			}
			_close(fd);

			if (buf[64] != '\n')
				return 1;

			p = buf;
			for (int i = 0; i < 32; i++)
			{
				hiChar = p[(i << 1)];
				if (hiChar >= '0' && hiChar <= '9')
					hiValue = hiChar - '0';
				else if (hiChar >= 'A' && hiChar <= 'F')
					hiValue = (hiChar - 'A') + 0x0A;
				else return 1;

				lowChar = p[(i << 1) + 1];
				if (lowChar >= '0' && lowChar <= '9')
					lowValue = lowChar - '0';
				else if (lowChar >= 'A' && lowChar <= 'F')
					lowValue = (lowChar - 'A') + 0x0A;
				else return 1;

				sk[i] = (hiValue << 4 | lowValue);
			}

			p = buf + 65;
			for (int i = 0; i < 33; i++)
			{
				hiChar = p[(i << 1)];
				if (hiChar >= '0' && hiChar <= '9')
					hiValue = hiChar - '0';
				else if (hiChar >= 'A' && hiChar <= 'F')
					hiValue = (hiChar - 'A') + 0x0A;
				else return 1;

				lowChar = p[(i << 1) + 1];
				if (lowChar >= '0' && lowChar <= '9')
					lowValue = lowChar - '0';
				else if (lowChar >= 'A' && lowChar <= 'F')
					lowValue = (lowChar - 'A') + 0x0A;
				else return 1;

				pk[i] = (hiValue << 4 | lowValue);
			}
			ret = 0;
		}
	}

	if (!bRet)
	{
		int fd;
		U8  len, oneChar;
		U8  hexString[65];
		U8 text[256 + 1] = { 0 };

		NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)hexSK, 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		if (STATUS_SUCCESS != status)
			return 1;

		if (0 != _tsopen_s(&fd, iniFile, _O_CREAT | _O_WRONLY | _O_TRUNC | _O_TEXT, _SH_DENYWR, 0))
			return 2;
		for (int i = 0; i < 32; i++)
		{
			sk[i] = hexSK[i];
			oneChar = (sk[i] >> 4) & 0x0F;
			if (oneChar < 0x0A)
				hexString[(i << 1)] = oneChar + '0';
			else
				hexString[(i << 1)] = (oneChar - 0x0A) + 'A';

			oneChar = sk[i] & 0x0F;
			if (oneChar < 0x0A)
				hexString[(i << 1) + 1] = oneChar + '0';
			else
				hexString[(i << 1) + 1] = (oneChar - 0x0A) + 'A';
		}
		hexString[64] = 0;

		sprintf_s((char*)text, 256, "[global]\nSK=%s\n", hexString);
		len = strlen((const char*)text);
		_write(fd, text, len);
		_close(fd);
	}
    return ret;
}
#endif

static MQTTSubPrivateData spd;

DWORD WINAPI MQTTSubThread(LPVOID lpData)
{
	int ret = 0;
	Mosquitto mq;
	HWND hWndUI;
	MemoryPoolContext mempool;
	InterlockedIncrement(&g_threadCount);

	hWndUI = (HWND)(lpData);
	ATLASSERT(::IsWindow(hWndUI));

	mempool = mempool_create("MQTT_SUB_POOL", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	if (nullptr == mempool)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 2, 0);
		goto QuitMQTTSubThread;
	}

	spd.hWnd = hWndUI;
	spd.mempool = mempool;

	mq = MQTT::MQTT_SubInit(&spd, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, &mqtt_callback);

	if (nullptr == mq) // something is wrong in MQTT sub routine
	{
		PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 1, 0);
		goto QuitMQTTSubThread;
	}

	{
		U8 topic[67];
		Raw2HexString(g_PK, 33, topic, nullptr);
		ret = MQTT::MQTT_AddSubTopic(CLIENT_SUB, (char*)topic);
	}

	if(ret)
	{
		PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 2, 0);
		goto QuitMQTTSubThread;
	}

	MQTT::MQTT_SubLoop(mq, &g_Quit);  // main loop go here.
	
QuitMQTTSubThread:
	mempool_destroy(mempool);
	MQTT::MQTT_SubTerm(mq);
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

	mq = MQTT::MQTT_PubInit(hWndUI, MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT, &mqtt_callback);

	if (nullptr == mq)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 1, 0);
		goto QuitMQTTPubThread;
	}

	mempool = mempool_create("MQTT_SUB_POOL", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	if (nullptr == mempool)
	{
		PostMessage(hWndUI, WM_MQTT_PUBMESSAGE, 2, 0);
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
			mbedtls_base64_encode(NULL, 0, &output_len, NULL, (msglen + 4 + 32)); // get the encoded length only
			msgbody = (U8*)palloc(mempool, 67 + output_len);
			
			if (msgbody)
			{
				U8* MSG = (U8*)palloc(mempool, msglen + 4 + 32);
				if (MSG)
				{

					Raw2HexString(p->pubkey, 33, (U8*)topic, nullptr);
					Raw2HexString(g_PK, 33, msgbody, nullptr);
					msgbody[66] = '|';

					U32* pLen = (U32*)(MSG);
					*pLen = (U32)msglen;

					{
						mbedtls_sha256_context sha256_context;
						mbedtls_sha256_init(&sha256_context);
						mbedtls_sha256_starts(&sha256_context, 0);
						mbedtls_sha256_update(&sha256_context, (const unsigned char*)p->message, msglen);
						mbedtls_sha256_finish(&sha256_context, MSG + 4);
					}

					{
						int m;
						U8 Key[32] = { 0 };
						U8 nonce[12];
						GetKeyFromSKAndPK(g_SK, p->pubkey, Key);
						mbedtls_chacha20_context cxt;
						mbedtls_chacha20_init(&cxt);
						m = mbedtls_chacha20_setkey(&cxt, Key);
						for (int i = 0; i < 12; i++)
							nonce[i] = i;
						m = mbedtls_chacha20_starts(&cxt, nonce, 0);
						m = mbedtls_chacha20_update(&cxt, 4 + 32, (const unsigned char*)MSG, MSG);
						m = mbedtls_chacha20_update(&cxt, msglen, (const unsigned char*)p->message, MSG + 4 + 32);
						mbedtls_chacha20_free(&cxt);
					}
					mbedtls_base64_encode(msgbody + 67, output_len, &encoded_len, (const unsigned char*)MSG, (msglen + 4 + 32));
					pfree(MSG);
				}

				MQTT::MQTT_PubMessage(mq, topic, (char*)msgbody, 67 + output_len);
				InterlockedIncrement(&(p->state)); // we have processed this task.

				pfree(msgbody);
				msgbody = nullptr;
			}

			p = p->next;
		}
	}

	mempool_destroy(mempool);

QuitMQTTPubThread:

	MQTT::MQTT_PubTerm(mq);

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

	static int PostMQTTMessage(MQTTSubPrivateData* pd, const struct mosquitto_message* message, const mosquitto_property* properties)
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

		if (!IsHexString(msg, 66)) // the first 66 bytes are the public key of the sender
			return 0;
		
		if (msg[66] != '|')
			return 0;

		mt = (MessageTask*)palloc0(mempool, sizeof(MessageTask));
		if (mt)
		{
			size_t output_len = 0;
			mbedtls_base64_decode(nullptr, 0, &output_len, msg + 67, len - 67 - 1); // get the bytes after decode, the last byte is 0
			U8* p = (U8*)palloc(mempool, output_len);
			if (p)
			{
				int m;
				U8 sha256hash[32];
				U8 nonce[12];
				U8 key[32] = { 0 };
				mbedtls_chacha20_context cxt;
				mbedtls_sha256_context sha256_context;

				HexString2Raw(msg, 66, mt->pubkey, nullptr);
				GetKeyFromSKAndPK(g_SK, mt->pubkey, key);
				mbedtls_base64_decode(p, output_len, &output_len, msg + 67, len - 67 - 1);

				mbedtls_chacha20_init(&cxt);
				m = mbedtls_chacha20_setkey(&cxt, key);
				for (int i = 0; i < 12; i++) nonce[i] = i;
				m = mbedtls_chacha20_starts(&cxt, nonce, 0);
				m = mbedtls_chacha20_update(&cxt, output_len, (const unsigned char*)p, p);  // decrypt the message
				mbedtls_chacha20_free(&cxt);

				mt->msgLen = *((U32*)p); // get the length
				if (mt->msgLen != output_len - 4 - 32)
				{
					pfree(p);
					pfree(mt);
					return 0;
				}
					
				mbedtls_sha256_init(&sha256_context);
				mbedtls_sha256_starts(&sha256_context, 0);
				mbedtls_sha256_update(&sha256_context, (const unsigned char*)p + 4 + 32, output_len - 4 - 32);
				mbedtls_sha256_finish(&sha256_context, sha256hash);
				if(memcmp(sha256hash, p + 4, 32)) // compare the hash value
				{
					pfree(p);
					pfree(mt);
					return 0;
				}

				mt->message = (wchar_t*)palloc(mempool, mt->msgLen);
				memcpy(mt->message, p + 36, mt->msgLen);
				mt->msgLen /= sizeof(wchar_t);
				pfree(p);
				if (PushReceiveMessageQueue(mt))
				{
					pfree(mt->message);
					pfree(mt);
					return 0;
				}

				::PostMessage(hWndUI, WM_MQTT_SUBMESSAGE, 0, 0); // tell the UI thread that a new message is received
			}
			else
			{
				pfree(mt);
			}
		}

		return 0;
	}

	static void MQTT_Message_Callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* message, const mosquitto_property* properties)
	{
		int i;
		bool res;
		struct mosq_config* pMQTTConf;
		MQTTSubPrivateData* pd;

		UNUSED(properties);

		pMQTTConf = (struct mosq_config*)obj;
		assert(nullptr != pMQTTConf);

		pd = (MQTTSubPrivateData*)(pMQTTConf->userdata);
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

