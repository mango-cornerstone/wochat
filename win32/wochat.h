#ifndef __WT_WOCHAT_H__
#define __WT_WOCHAT_H__

#include "wochatypes.h"

#include "dui.h"
#include "secp256k1.h"
#include "mbedtls/build_info.h"
#include "mbedtls/aes.h"
#include "sqlite/sqlite3.h"

extern LONG               g_threadCount;
extern LONG               g_Quit;
extern HANDLE             g_MQTTPubEvent;

extern CRITICAL_SECTION    g_csMQTTSub;
extern CRITICAL_SECTION    g_csMQTTPub;

extern U8 g_PKText[67];

extern ID2D1Factory* g_pD2DFactory;
extern IDWriteFactory* g_pDWriteFactory;

#define WT_TEXTFORMAT_MAINTEXT		0
#define WT_TEXTFORMAT_TITLE			1
#define WT_TEXTFORMAT_GROUPNAME		2
#define WT_TEXTFORMAT_TIMESTAMP		3
#define WT_TEXTFORMAT_GROUPMESSAGE	4
#define WT_TEXTFORMAT_INPUTMESSAGE	5
#define WT_TEXTFORMAT_USERNAME		6
#define WT_TEXTFORMAT_OTHER			7

IDWriteTextFormat* GetTextFormat(U8 idx);

extern U8  g_SK[32];
extern U8  g_PK[33];
extern U8  g_PKTo[33];

//int GetKeys(LPCTSTR path, U8* sk, U8* pk);

DWORD WINAPI MQTTSubThread(LPVOID lpData);
DWORD WINAPI MQTTPubThread(LPVOID lpData);

#define MQTT_DEFAULT_HOST	("www.boobooke.com")
#define MQTT_DEFAULT_PORT	1883

#define WM_MQTT_PUBMESSAGE		(WM_USER + 300)
#define WM_MQTT_SUBMESSAGE		(WM_USER + 301)

#define WIN4_GET_PUBLICKEY		1
#define WIN4_UPDATE_MESSAGE		2

typedef struct MQTTMessage
{
	MQTTMessage* next;
	U8    processed;
	char* topic;
	int   msglen;
	char* msgbody;
} MQTTMessage;

typedef struct MessageTask
{
	MessageTask* next;
	MessageTask* prev;
	LONG state;
	U8  pubkey[33];
	wchar_t* message;
	U32 msgLen;
} MessageTask;

typedef struct MQTTPubData
{
	char* host;
	int   port;
	char* default_topic;
	MessageTask* task;
} MQTTPubData;

typedef struct MQTTSubData
{
	char* host;
	int   port;
	char* topic;
	MessageTask* task;
}MQTTSubData;

extern MQTTPubData g_PubData;
extern MQTTSubData g_SubData;

typedef struct XChatMessage
{
	struct XChatMessage* next;
	struct XChatMessage* prev;
	U32  id;
	U32* icon;     // the bitmap data of this icon
	U8   w;        // the width in pixel of this icon
	U8   h;        // the height in pixel of this icon
	int  height;   // in pixel
	int  width;    // in pixel
	DWRITE_TEXT_METRICS tm;
	U16  state;
	U64  ts;		// the time stamp. 
	wchar_t* name;      // The name of this people      
	wchar_t* message;  // real message
	U16  msgLen;
	U8* obj;       // point to GIF/APNG/Video/Pic etc
} XChatMessage;

typedef struct XChatGroup
{
	XChatGroup* next;
	U8   pubkey[33];	// The public key of this chat group
	U32* icon;			// the bitmap data of this icon
	U8   w;			    // the width in pixel of this icon
	U8   h;			    // the height in pixel of this icon
	U16  width;		    // in pixel
	U16  height;		// in pixel
	U16  unread;		// how many unread messages? if more than 254, use ... 
	U16  member;		// how many members in this group?
	wchar_t* name;		// the group name
	U64  ts;			// the time stamp. 
	wchar_t* tsText;	// the group name
	wchar_t* lastmsg;	// the last message of this group
	XChatMessage* headMessage;
	XChatMessage* tailMessage;
	MemoryPoolContext mempool;
} XChatGroup;

int AESEncrypt(U8* input, int length, U8* output);
int InitWoChatDatabase(LPCWSTR lpszPath);

int PushSendMessageQueue(MessageTask* message_task);
int PushReceiveMessageQueue(MessageTask* message_task);

int GetPKFromSK(U8* sk, U8* pk);
int GetKeyFromSKAndPK(U8* sk, U8* pk, U8* key);

int Raw2HexString(U8* input, U8 len, U8* output, U8* outlen);
int HexString2Raw(U8* input, U8 len, U8* output, U8* outlen);
int Raw2HexStringW(U8* input, U8 len, wchar_t* output, U8* outlen);
//int HexString2RawW(wchar_t* input, U8 len, U8* output, U8* outlen);

int GetCurrentPublicKey(void* parent, U8* pk);


#endif // __WT_WOCHAT_H__