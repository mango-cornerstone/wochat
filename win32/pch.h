#ifndef __WT_PCH_H__
#define __WT_PCH_H__

#define WIN32_LEAN_AND_MEAN 
/*
 * The defined WIN32_NO_STATUS macro disables return code definitions in
 * windows.h, which avoids "macro redefinition" MSVC warnings in ntstatus.h.
 */
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

#include <stdio.h>
#include <stdint.h>
#include <io.h> 
#include <fcntl.h> 
#include <ntstatus.h>
#include <bcrypt.h>
#include <commdlg.h>

#include <atlbase.h>
#include <atlwin.h>
#include <d2d1.h>
#include <dwrite.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "bcrypt.lib")

#include <mutex>
#include <cmath>

#endif // __WT_PCH_H__
