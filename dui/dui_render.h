#ifndef __WT_DUI_RENDER_H__
#define __WT_DUI_RENDER_H__

#include "wochatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef U32	pg_crc32c;

/* The INIT and EQ macros are the same for all implementations. */
#define INIT_CRC32C(crc) ((crc) = 0xFFFFFFFF)
#define EQ_CRC32C(c1, c2) ((c1) == (c2))

/* Use Intel SSE4.2 instructions. */
#define COMP_CRC32C(crc, data, len) \
	((crc) = pg_comp_crc32c_sse42((crc), (data), (len)))
#define FIN_CRC32C(crc) ((crc) ^= 0xFFFFFFFF)

extern pg_crc32c pg_comp_crc32c_sse42(pg_crc32c crc, const void* data, size_t len);


/* fill the whole screen with one color */
int DUI_ScreenClear(uint32_t* dst, uint32_t size, uint32_t color);

int DUI_ScreenDrawRect(uint32_t* dst, int w, int h, uint32_t* src, int sw, int sh, int dx, int dy);

int DUI_ScreenDrawRectRound(uint32_t* dst, int w, int h, uint32_t* src, int sw, int sh, int dx, int dy, uint32_t color0, uint32_t color1);

int DUI_ScreenFillRect(uint32_t* dst, int w, int h, uint32_t color, int sw, int sh, int dx, int dy);

int DUI_ScreenFillRectRound(uint32_t* dst, int w, int h, uint32_t color, int sw, int sh, int dx, int dy, uint32_t c1, uint32_t c2);

#ifdef __cplusplus
}
#endif

#endif // __WT_DUI_RENDER_H__