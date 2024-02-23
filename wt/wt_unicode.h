#ifndef __WT_UNICODE_H__
#define __WT_UNICODE_H__
#ifdef __cplusplus
extern "C" {
#endif

/*
 * https://android.googlesource.com/platform/external/id3lib/+/master/unicode.org/ConvertUTF.h
 */

#include "wochatypes.h"

 /* ---------------------------------------------------------------------
	 The following 4 definitions are compiler-specific.
	 The C standard does not guarantee that wchar_t has at least
	 16 bits, so wchar_t is no less portable than unsigned short!
	 All should be unsigned values to avoid sign extension during
	 bit mask & shift operations.
 ------------------------------------------------------------------------ */
typedef unsigned long	UTF32;	/* at least 32 bits */
typedef unsigned short	UTF16;	/* at least 16 bits */
typedef unsigned char	UTF8;	/* typically 8 bits */
typedef unsigned char	Boolean; /* 0 or 1 */

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP (UTF32)0x0000FFFF
#define UNI_MAX_UTF16 (UTF32)0x0010FFFF
#define UNI_MAX_UTF32 (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32 (UTF32)0x0010FFFF

typedef enum 
{
	conversionOK, 		/* conversion successful */
	sourceExhausted,	/* partial character in source, but hit end */
	targetExhausted,	/* insuff. room in target for conversion */
	sourceIllegal		/* source sequence is illegal/malformed */
} ConversionResult;

typedef enum 
{
	strictConversion = 0,
	lenientConversion
} ConversionFlags;

#ifdef __cplusplus
}
#endif

#endif /* __WT_UNICODE_H__ */