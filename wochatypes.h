/*
 *  common basic data types and constant symbols used in the whole project
 */

#ifndef __WT_WOCHATYPES_H__
#define __WT_WOCHATYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define S8      int8_t
#define S16     int16_t
#define S32     int32_t
#define S64     int64_t

#define U8      uint8_t
#define U16     uint16_t
#define U32     uint32_t
#define U64     uint64_t

typedef unsigned char uint8;	/* == 8 bits */
typedef unsigned short uint16;	/* == 16 bits */
typedef unsigned int uint32;	/* == 32 bits */

typedef signed char int8;		/* == 8 bits */
typedef signed short int16;		/* == 16 bits */
typedef signed int int32;		/* == 32 bits */

typedef long long int int64;

typedef unsigned long long int uint64;

/*
 * Size
 *		Size of any memory resident object, as returned by sizeof.
 */
typedef size_t Size;

typedef struct WTFriend
{
	struct WTFriend* next;
	U8 pubkey[33];
	U16* name;		// the name of this friend
	U8   nameLen;
	U32* iconSmall;
	U8   wSmall;	 // the width in pixel of this icon
	U8   hSmall;	 // the height in pixel of this icon
	U32* iconLarge;
	U8   wLarge;	 // the width in pixel of this icon
	U8   hLarge;	 // the height in pixel of this icon
} WTFriend;

typedef struct WTChatMessage
{
	struct WTChatMessage* next;
	struct WTChatMessage* prev;
	U32  id;
	U8   hash[32];
	U32* icon;     // the bitmap data of this icon
	U8   w;        // the width in pixel of this icon
	U8   h;        // the height in pixel of this icon
	int  height;   // in pixel
	int  width;    // in pixel
	U16  state;
	U64  ts;		// the time stamp. 
	wchar_t* name;      // The name of this people      
	wchar_t* message;  // real message
	U16  msgLen;
	U8* obj;       // point to GIF/APNG/Video/Pic etc
} WTChatMessage;

typedef struct WTChatGroup WTChatGroup;

typedef struct WTChatGroup
{
	WTChatGroup* next;
	WTFriend* people;
	U16  width;		    // in pixel, the width of window 4
	U16  height;		// in pixel

	U32  total;         // total messages in this group
	U32  unread;		// how many unread messages? if more than 254, use ... 
	U16  member;		// how many members in this group?
	U64  ts;			// the time stamp of last message. 
	U16* tsText;	    // the text of time stamp
	U16* lastmsg;	    // the last message of this group
	U16  lastMsgLen;
	U16  lastmsgLen;
	WTChatMessage* msgHead;
	WTChatMessage* msgTail;
} WTChatGroup;

#define SETTING_GENERAL		0
#define SETTING_NETWORK		1
#define SETTING_ABOUT		2

typedef struct WTSetting
{
	struct WTSetting* next;
	U8   id;
	U16* name;		// the group name
	U16  nameLen;
	U16  height;    // the height of this string in pixel
} WTSetting;


#define WT_OK					0x00000000		/* Successful result */
#define WT_FAIL					0x00000001

#define WT_SOURCEEXHAUSTED		0x00000002
#define WT_TARGETEXHAUSTED		0x00000003
#define WT_SOURCEILLEGAL		0x00000004
#define WT_SK_GENERATE_ERR		0x00000005
#define WT_PK_GENERATE_ERR		0x00000006
#define WT_UTF16ToUTF8_ERR		0x00000007
#define WT_MEMORY_ALLOC_ERR		0x00000008
#define WT_SQLITE_OPEN_ERR      0x00000009
#define WT_SQLITE_PREPARE_ERR   0x0000000A
#define WT_SQLITE_STEP_ERR      0x0000000B
#define WT_SQLITE_FINALIZE_ERR  0x0000000C
#define WT_MEMORY_POOL_ERROR	0x0000000D
#define WT_DYNAMIC_HASHTAB_ERR	0x0000000E
#define WT_SECP256K1_CTX_ERROR  0x0000000F
#define WT_RANDOM_NUMBER_ERROR  0x00000010


#define WT_PARAM_APPLICATION_NAME			0
#define WT_PARAM_SERVER_NAME				1
#define WT_PARAM_SERVER_PORT				2
#define WT_PARAM_NETWORK_PROTOCAL			3

/* the UI parameters for desktop(Windows,MacOS, Linux) application begin with WT_PARAM_D_ */
#define WT_PARAM_D_WIN0_WIDTH				1

/* the UI parameters for mobile(iOS, Android) application begin with WT_PARAM_M_ */
#define WT_PARAM_M_DUMMY					1


#ifdef __cplusplus
}
#endif

#endif /* __WT_WOCHATYPES_H__ */
