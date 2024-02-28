#ifndef __WT_WOCHAT_H__
#define __WT_WOCHAT_H__

#include "wochatypes.h"

#include "dui.h"
#include "wt_mempool.h"
#include "wt_hash.h"
#include "wt_utils.h"
#include "wt_sha256.h"
#include "wt_aes256.h"
#include "wt_unicode.h"

#include "secp256k1.h"
#include "sqlite/sqlite3.h"

extern LONG  g_threadCount;
extern LONG  g_Quit;
extern LONG  g_NetworkStatus;
extern DWORD g_dwMainThreadID;

extern wchar_t*          g_myName;
extern U8*               g_myImage32;
extern U8*               g_myImage128;
extern U8*				 g_aiImage32;
extern U8*				 g_aiImage128;

extern HTAB*             g_messageHTAB;
extern HTAB*             g_keyHTAB;
extern MemoryPoolContext g_messageMemPool;
extern MemoryPoolContext g_topMemPool;

extern HINSTANCE g_hInstance;
extern wchar_t   g_AppPath[MAX_PATH + 1];
extern wchar_t   g_DBPath[MAX_PATH + 1];

extern HWND		 g_hWndShareScreen;
extern HWND		 g_hWndChatHistory;
extern HWND		 g_hWndAudioCall;
extern HWND		 g_hWndSearchAll;

extern U8* g_SK;
extern U8* g_PK;
extern U8* g_pkAI;
extern U8* g_MQTTPubClientId;
extern U8* g_MQTTSubClientId;

extern HANDLE             g_MQTTPubEvent;

extern CRITICAL_SECTION    g_csMQTTSubUI;
extern CRITICAL_SECTION    g_csMQTTPubUI;
extern CRITICAL_SECTION    g_csMQTTPubSub;

extern ID2D1Factory* g_pD2DFactory;
extern IDWriteFactory* g_pDWriteFactory;

#define WT_TEXTFORMAT_MAINTEXT		0
#define WT_TEXTFORMAT_TITLE			1
#define WT_TEXTFORMAT_GROUPNAME		2
#define WT_TEXTFORMAT_TIMESTAMP		3
#define WT_TEXTFORMAT_GROUPMESSAGE	4
#define WT_TEXTFORMAT_INPUTMESSAGE	5
#define WT_TEXTFORMAT_USERNAME		6
#define WT_TEXTFORMAT_OTHER0		7
#define WT_TEXTFORMAT_OTHER1		8
#define WT_TEXTFORMAT_OTHER2		9
#define WT_TEXTFORMAT_OTHER3		10
#define WT_TEXTFORMAT_OTHER4		11
#define WT_TEXTFORMAT_OTHER5		12
#define WT_TEXTFORMAT_OTHER6		13
#define WT_TEXTFORMAT_OTHER7		14
#define WT_TEXTFORMAT_OTHER8		15
#define WT_TEXTFORMAT_TOTAL			16

#define WM_MQTT_PUBMESSAGE		(WM_USER + 300)
#define WM_MQTT_SUBMESSAGE		(WM_USER + 301)
#define WM_LOADPERCENTMESSAGE	(WM_USER + 302)

#define WM_WIN_MAINUITHREAD		(WM_USER + 400)
#define WM_WIN_SCREENTHREAD		(WM_USER + 401)
#define WM_WIN_CHATHISTHREAD	(WM_USER + 402)
#define WM_WIN_AUDIOTHREAD		(WM_USER + 403)
#define WM_WIN_SEARCHALLTHREAD	(WM_USER + 404)

#define WM_WIN_BRING_TO_FRONT	(WM_USER + 500)

#define WIN4_GET_PUBLICKEY		1
#define WIN4_UPDATE_MESSAGE		2

#define MESSAGE_TASK_STATE_NULL		0
#define MESSAGE_TASK_STATE_ONE		1
#define MESSAGE_TASK_STATE_COMPLETE	2


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
	U8   hash[32];
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

typedef struct XMessage
{
	U8 hash[32];
	XChatMessage* pointer;
} XMessage;

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
	U16 nameLen;
	U64  ts;			// the time stamp. 
	wchar_t* tsText;	// the text of time stamp
	wchar_t* lastmsg;	// the last message of this group
	U16 lastmsgLen;
	XChatMessage* headMessage;
	XChatMessage* tailMessage;
	MemoryPoolContext mempool;
} XChatGroup;

typedef struct XSetting
{
	XSetting* next;
	wchar_t* name;		// the group name
	U16 nameLen;
} XSetting;

typedef struct XFriend
{
	XFriend* next;
	U8 pubkey[33];
	wchar_t* name;		// the group name
	U16 nameLen;
	U32* iconSmall;
	U8   wSmall;			    // the width in pixel of this icon
	U8   hSmall;			    // the height in pixel of this icon
	U32* iconLarge;
	U8   wLarge;			    // the width in pixel of this icon
	U8   hLarge;			    // the height in pixel of this icon
} XFriend;

U32 CheckWoChatDatabase(LPCWSTR lpszPath);

int PushTaskIntoSendMessageQueue(MessageTask* message_task);
U32 GenPublicKeyFromSecretKey(U8* sk, U8* pk);
int GetReceiverPublicKey(void* parent, U8* pk);

// send a confirmation to the sender
int SendConfirmationMessage(U8* pk, U8* hash);
int DoWoChatLoginOrRegistration(HINSTANCE hInstance);
U32 CreateNewAccount(wchar_t* name, U8 nlen, wchar_t* pwd, U8 plen, U32* skIdx);
U32 OpenAccount(U32 idx, U16* pwd, U32 len);

int GetSecretKey(U8* sk, U8* pk);

S64 GetCurrentUTCTime64();

void GetU64TimeStringW(S64 tm, wchar_t* tmstr, U16 len);

U32 GetSecretKeyNumber(U32* total);

IDWriteTextFormat* GetTextFormatAndHeight(U8 idx, U16* height = nullptr);

#endif // __WT_WOCHAT_H__