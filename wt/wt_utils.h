#ifndef __WT_UTILS_H__
#define __WT_UTILS_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "wochatypes.h"

bool wt_IsBigEndian();

int wt_Raw2HexString(U8* input, U8 len, U8* output, U8* outlen);
int wt_HexString2Raw(U8* input, U8 len, U8* output, U8* outlen);
int wt_Raw2HexStringW(U8* input, U8 len, wchar_t* output, U8* outlen);

bool wt_IsPublicKey(U8* str, const U8 len);

#ifdef __cplusplus
}
#endif

#endif /* __WT_UTILS_H__ */