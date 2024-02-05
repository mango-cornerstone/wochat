/*
 *  common basic data types used in the whole project
 */

#ifndef _WT_WOCHATYPES_H_
#define _WT_WOCHATYPES_H_

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


#ifdef __cplusplus
}
#endif

#endif /* _WT_WOCHATYPES_H_ */
