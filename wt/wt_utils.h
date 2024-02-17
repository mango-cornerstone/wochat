#ifndef __WT_UTILS_H__
#define __WT_UTILS_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "wochatypes.h"

typedef U32	wt_crc32c;

wt_crc32c wt_comp_crc32c_sse42(wt_crc32c crc, const void* data, size_t len);

/* The INIT and EQ macros are the same for all implementations. */
#define INIT_CRC32C(crc) ((crc) = 0xFFFFFFFF)
#define EQ_CRC32C(c1, c2) ((c1) == (c2))

/* Use Intel SSE4.2 instructions. */
#define COMP_CRC32C(crc, data, len) \
	((crc) = wt_comp_crc32c_sse42((crc), (data), (len)))
#define FIN_CRC32C(crc) ((crc) ^= 0xFFFFFFFF)

bool wt_IsBigEndian();

int wt_Raw2HexString(U8* input, U8 len, U8* output, U8* outlen);
int wt_HexString2Raw(U8* input, U8 len, U8* output, U8* outlen);
int wt_Raw2HexStringW(U8* input, U8 len, wchar_t* output, U8* outlen);

bool wt_IsPublicKey(U8* str, const U8 len);

/*
 *  fill the buffer of len bytes with random data
 *  return 0 if successful.
 */
int wt_FillRandomData(U8* buf, U8 len);

// generate a 32-byte random data. sk must be 32 bytes long
int wt_GenerateSecretKey(U8* sk);

bool wt_IsHexString(U8* str, U8 len);

bool wt_IsAlphabetString(U8* str, U8 len);

U8 wt_GenRandomIntLessThan(U8 lessthan);

#ifdef __cplusplus
}
#endif

#endif /* __WT_UTILS_H__ */