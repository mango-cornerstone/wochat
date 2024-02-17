/*
 * The defined WIN32_NO_STATUS macro disables return code definitions in
 * windows.h, which avoids "macro redefinition" MSVC warnings in ntstatus.h.
 */
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <bcrypt.h>

#include <nmmintrin.h>

#include "wt_utils.h"
#include "wt_sha256.h"

bool wt_IsAlphabetString(U8* str, U8 len)
{
	bool bRet = false;

	if (str && len)
	{
		U8 i, oneChar;
		for (i = 0; i < len; i++)
		{
			oneChar = str[i];
			if (oneChar >= '0' && oneChar <= '9')
				continue;
			if (oneChar >= 'A' && oneChar <= 'Z')
				continue;
			break;
		}
		if (i == len)
			bRet = true;
	}
	return bRet;

}
bool wt_IsHexString(U8* str, U8 len)
{
	bool bRet = false;

	if (str && len)
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

int wt_Raw2HexString(U8* input, U8 len, U8* output, U8* outlen)
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

int wt_HexString2Raw(U8* input, U8 len, U8* output, U8* outlen)
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

int wt_Raw2HexStringW(U8* input, U8 len, wchar_t* output, U8* outlen)
{
	U8 idx, i;
	const wchar_t* HexChars = (const wchar_t*)(L"0123456789ABCDEF");

	for (i = 0; i < len; i++)
	{
		idx = ((input[i] >> 4) & 0x0F);
		output[(i << 1)] = HexChars[idx];

		idx = (input[i] & 0x0F);
		output[(i << 1) + 1] = HexChars[idx];
	}

	output[(i << 1)] = 0;

	if (outlen)
		*outlen = (i << 1);

	return 0;
}

bool wt_IsPublicKey(U8* str, const U8 len)
{
	bool bRet = false;

	if (66 != len)
		return false;

	if (str)
	{
		U8 i, oneChar;

		if (str[0] != '0')
			return false;

		if ((str[1] == '2') || (str[1] == '3')) // the first two bytes is '02' or '03' for public key
		{
			for (i = 2; i < 66; i++)
			{
				oneChar = str[i];
				if (oneChar >= '0' && oneChar <= '9')
					continue;
				if (oneChar >= 'A' && oneChar <= 'F')
					continue;
#if 0
				if (oneChar >= 'a' && oneChar <= 'f')
					continue;
#endif
				break;
			}
			if (i == len)
				bRet = true;
		}
	}
	return bRet;
}

int wt_FillRandomData(U8* buf, U8 len)
{
	int r = 1;
	if (buf)
	{
		NTSTATUS status = BCryptGenRandom(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		if (STATUS_SUCCESS == status)
			r = 0;
	}
	return r;
}

int wt_GenerateSecretKey(U8* sk)
{
	int r = 1;
	if (sk)
	{
		U8 i, k;
		U8* p;
		U8* q;
		NTSTATUS status;
		U8 random_data[32] = { 0 };

		for (U8 k=0; k<255; k++)
		{
			status = BCryptGenRandom(NULL, random_data, 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
			if (STATUS_SUCCESS == status)
			{
				p = random_data;
				q = random_data + 16;
				// compare the first 16 bytes with the last 16 bytes to be sure they are not the same. This is rare
				for (i = 0; i < 16; i++) 
				{
					if (*p++ != *q++)
						break;
				}
				if (i < 16)
				{
					wt_sha256_hash(random_data, 32, sk);
					p = sk;
					q = sk + 16;
					for (i = 0; i < 16; i++)
					{
						if (*p++ != *q++)
							break;
					}
					if (i < 16)
					{
						r = 0;
						break;
					}
				}
			}
		}
	}
	return r;
}


wt_crc32c wt_comp_crc32c_sse42(wt_crc32c crc, const void* data, size_t len)
{
	const unsigned char* p = data;
	const unsigned char* pend = p + len;

	/*
	 * Process eight bytes of data at a time.
	 *
	 * NB: We do unaligned accesses here. The Intel architecture allows that,
	 * and performance testing didn't show any performance gain from aligning
	 * the begin address.
	 */
#ifdef _M_X64
	while (p + 8 <= pend)
	{
		crc = (U32)_mm_crc32_u64(crc, *((const U64*)p));
		p += 8;
	}

	/* Process remaining full four bytes if any */
	if (p + 4 <= pend)
	{
		crc = _mm_crc32_u32(crc, *((const unsigned int*)p));
		p += 4;
	}
#else
	 /*
	  * Process four bytes at a time. (The eight byte instruction is not
	  * available on the 32-bit x86 architecture).
	  */
	while (p + 4 <= pend)
	{
		crc = _mm_crc32_u32(crc, *((const unsigned int*)p));
		p += 4;
	}
#endif							/* _M_X64 */

	/* Process any remaining bytes one at a time. */
	while (p < pend)
	{
		crc = _mm_crc32_u8(crc, *p);
		p++;
	}

	return crc;
}
