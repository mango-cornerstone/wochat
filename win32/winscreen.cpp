#include "pch.h"
#include "resource.h"
#include "wochat.h"
#include "winscreen.h"

static U8 sreen_public_key[33] = { 0 };
static wchar_t* szWindowClass = L"WoChatShareScreen";
static wchar_t title[5] = { 0x5206, 0x4eab,0x5c4f, 0x5e55, 0 };
static bool singleInstance = false;
static DWORD WINAPI ShareScreenUIThread(LPVOID lpData);
static LRESULT CALLBACK WndProcShareScreen(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// pk is the Id of the other part that will share the screen
int CreateShareScreenThread(const U8* pk)
{
	DWORD dwThreadID = 0;
	HANDLE hThread = NULL;

	if (singleInstance)
	{
		if (::IsWindow(g_hWndShareScreen))
		{
			::PostMessage(g_hWndShareScreen, WM_WIN_BRING_TO_FRONT, 0, 0);
		}
		return 0;
	}

	singleInstance = true;
	if (wt_IsPublicKey((U8*)pk, 66))
	{
		wt_HexString2Raw((U8*)pk, 66, sreen_public_key, nullptr);
	}

	hThread = ::CreateThread(NULL, 0, ShareScreenUIThread, nullptr, 0, &dwThreadID);
	if (hThread == NULL)
	{
		::MessageBox(NULL, _T("ERROR: Cannot create share screen thread!!!"), _T("WoChat"), MB_OK);
	}
	return 0;
}


DWORD WINAPI ShareScreenUIThread(LPVOID lpData)
{
	HWND hWnd;
	MSG msg;
	WNDCLASSEXW wcex;

	InterlockedIncrement(&g_threadCount);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProcShareScreen;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = g_hInstance;
	wcex.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = NULL;

	ATOM rc = RegisterClassExW(&wcex);
	if (rc)
	{
		hWnd = ::CreateWindowW(szWindowClass, title, WS_OVERLAPPEDWINDOW,
			500, 50, 1024, 768, nullptr, nullptr, g_hInstance, nullptr);

		if (hWnd)
		{
			g_hWndShareScreen = hWnd;
			::ShowWindow(hWnd, SW_SHOW);
			::UpdateWindow(hWnd);

			// Main message loop:
			while ((0 == g_Quit) && GetMessage(&msg, NULL, 0, 0))
			{
				if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			if (::IsWindow(hWnd))
				::DestroyWindow(hWnd);
		}

		UnregisterClass(szWindowClass, g_hInstance);
	}

	InterlockedDecrement(&g_threadCount);
	singleInstance = false;
	g_hWndShareScreen = nullptr;
	return 0;
}

LRESULT CALLBACK WndProcShareScreen(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code that uses hdc here...
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_WIN_BRING_TO_FRONT:
		{
			if (IsIconic(hWnd))
				::ShowWindow(hWnd, SW_RESTORE);
			::SetForegroundWindow(hWnd);
		}
		break;
	case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
