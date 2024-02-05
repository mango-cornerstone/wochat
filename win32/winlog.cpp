#include "pch.h"

#include "winlog.h"


static void DebugDisplay(const char* s) noexcept 
{
	::OutputDebugStringA(s);
}

void DebugPrintf(const char* format, ...) noexcept 
{
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsnprintf(buffer, std::size(buffer), format, pArguments);
	va_end(pArguments);
	DebugDisplay(buffer);
}
