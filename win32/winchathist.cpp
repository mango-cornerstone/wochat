#include "pch.h"
#include "resource.h"
#include "wochat.h"
#include "winchathist.h"

static wchar_t* szWindowClassChatHistory = L"WoChatChatHistory";
static wchar_t titleChatHistory[] = { 0x804a, 0x5929, 0x5386, 0x53f2, 0x8bb0, 0x5f55, 0 };
static bool singleInstanceChatHistory = false;
static DWORD WINAPI ChatHistoryUIThread(LPVOID lpData);
static LRESULT CALLBACK WndProcChatHistory(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// pk is the Id of the other part that will share the screen
int CreateChatHistoryThread(const U8* pk)
{
	DWORD dwThreadID = 0;
	HANDLE hThread = NULL;

	if (singleInstanceChatHistory)
	{
		if (::IsWindow(g_hWndChatHistory))
		{
			::PostMessage(g_hWndChatHistory, WM_WIN_BRING_TO_FRONT, 0, 0);
		}
		return 0;
	}

	singleInstanceChatHistory = true;

	hThread = ::CreateThread(NULL, 0, ChatHistoryUIThread, nullptr, 0, &dwThreadID);
	if (hThread == NULL)
	{
		::MessageBox(NULL, _T("ERROR: Cannot create share screen thread!!!"), _T("WoChat"), MB_OK);
	}
	return 0;
}


DWORD WINAPI ChatHistoryUIThread(LPVOID lpData)
{
	HWND hWnd;
	MSG msg;
	WNDCLASSEXW wcex;

	InterlockedIncrement(&g_threadCount);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProcChatHistory;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = g_hInstance;
	wcex.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClassChatHistory;
	wcex.hIconSm = NULL;

	ATOM rc = RegisterClassExW(&wcex);
	if (rc)
	{
		hWnd = ::CreateWindowW(szWindowClassChatHistory, titleChatHistory, WS_OVERLAPPEDWINDOW,
			100, 50, 600, 800, nullptr, nullptr, g_hInstance, nullptr);

		if (hWnd)
		{
			g_hWndChatHistory = hWnd;
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

		UnregisterClass(szWindowClassChatHistory, g_hInstance);
	}

	InterlockedDecrement(&g_threadCount);
	singleInstanceChatHistory = false;
	g_hWndChatHistory = nullptr;
	return 0;
}

LRESULT CALLBACK WndProcChatHistory(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
