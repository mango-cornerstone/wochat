#ifndef __WT_WOCHAT_H__
#define __WT_WOCHAT_H__

#include "wochatypes.h"

#include "dui.h"
#include "wt_mempool.h"
#include "wt_utils.h"
#include "wt_sha256.h"
#include "wt_aes256.h"

#include "secp256k1.h"
#include "sqlite/sqlite3.h"

extern LONG  g_threadCount;
extern LONG  g_Quit;
extern LONG  g_NetworkStatus;
extern DWORD g_dwMainThreadID;
extern U32	 g_messageSequence;

extern U8* g_SK;
extern U8* g_PK;
extern U8* g_PKTo;
extern U8* g_MQTTPubClientId;
extern U8* g_MQTTSubClientId;

extern HANDLE             g_MQTTPubEvent;

extern CRITICAL_SECTION    g_csMQTTSub;
extern CRITICAL_SECTION    g_csMQTTPub;

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

DWORD WINAPI MQTTSubThread(LPVOID lpData);
DWORD WINAPI MQTTPubThread(LPVOID lpData);

#define WM_MQTT_PUBMESSAGE		(WM_USER + 300)
#define WM_MQTT_SUBMESSAGE		(WM_USER + 301)

#define WM_WIN_MAINUITHREAD		(WM_USER + 400)
#define WM_WIN_SCREENTHREAD		(WM_USER + 401)

#define WIN4_GET_PUBLICKEY		1
#define WIN4_UPDATE_MESSAGE		2

#define MESSAGE_TASK_STATE_NULL		0
#define MESSAGE_TASK_STATE_ONE		1
#define MESSAGE_TASK_STATE_COMPLETE	2

// a message node in the message queue
typedef struct MessageTask
{
	MessageTask* next;
	LONG state;
	U8   pubkey[33];
	U8   type;
	U8   hash[32]; // the SHA256 hash value of this node
	U8*  message;  // the first 4 bytes is a sequnce number to be sure that the same text has different SHA256 value
	U32  msgLen;
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
	U32  total;         // total messages in this group
	U32  unread;		// how many unread messages? if more than 254, use ... 
	U16  member;		// how many members in this group?
	wchar_t* name;		// the group name
	U64  ts;			// the time stamp. 
	wchar_t* tsText;	// the text of time stamp
	wchar_t* lastmsg;	// the last message of this group
	XChatMessage* headMessage;
	XChatMessage* tailMessage;
	MemoryPoolContext mempool;
} XChatGroup;

int InitWoChatDatabase(LPCWSTR lpszPath);

int PushTaskIntoSendMessageQueue(MessageTask* message_task);
//int PushTaskIntoReceiveMessageQueue(MessageTask* message_task);
int GenPublicKeyFromSecretKey(U8* sk, U8* pk);
int GetReceiverPublicKey(void* parent, U8* pk);

#endif // __WT_WOCHAT_H__