#include "wt_utils.h"

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
				if (oneChar >= 'a' && oneChar <= 'f')
					continue;
				if (oneChar >= 'A' && oneChar <= 'F')
					continue;
				break;
			}
			if (i == len)
				bRet = true;
		}
	}
	return bRet;
}
