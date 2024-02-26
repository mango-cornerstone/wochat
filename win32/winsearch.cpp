#include "pch.h"
#include "resource.h"
#include "wochat.h"
#include "wochatdef.h"
#include "winsearch.h"

static bool singleInstanceSearch = false;
static wchar_t txtSearchAll[] = { 0x641c,0x7d22,0x5168,0x90e8,0x804a,0x5929,0x8bb0,0x5f55,0 };

class XWinSearch : public ATL::CWindowImpl<XWinSearch>
{
public:
	DECLARE_XWND_CLASS(NULL, IDR_MAINFRAME, 0)

	BEGIN_MSG_MAP(XWinSearch)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_WIN_BRING_TO_FRONT, OnBringToFront)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL bHandled)
	{
		PostQuitMessage(0);
		return 0;
	}

	LRESULT OnBringToFront(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (IsIconic())
			ShowWindow(SW_RESTORE);

		::SetForegroundWindow(m_hWnd);

		return 0;
	}

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		PAINTSTRUCT ps;
		BeginPaint(&ps);
		EndPaint(&ps);
		return 0;
	}
};


DWORD WINAPI SearchAllUIThread(LPVOID lpData)
{
	DWORD dwRet = 0;
	RECT rw = { 0 };
	MSG msg = { 0 };

	InterlockedIncrement(&g_threadCount);

	XWinSearch winSearch;

	rw.left = 100; rw.top = 10; rw.right = rw.left + 600; rw.bottom = rw.top + 800;

	winSearch.Create(NULL, rw, txtSearchAll, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE);
	if (FALSE == winSearch.IsWindow())
		goto Exit_SearchAllUIThread;

	g_hWndSearchAll = winSearch.m_hWnd;
	winSearch.ShowWindow(SW_SHOW);

	// Main message loop:
	while ((0 == g_Quit) && GetMessageW(&msg, nullptr, 0, 0)) 
	{
		if (!TranslateAcceleratorW(msg.hwnd, 0, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	if (winSearch.IsWindow())
		winSearch.DestroyWindow();

Exit_SearchAllUIThread:
	InterlockedDecrement(&g_threadCount);
	singleInstanceSearch = false;
	g_hWndSearchAll = nullptr;
	return 0;
}

// pk is the Id of the other part that will share the screen
int CreateSearchAllThread(const U8* pk)
{
	DWORD dwThreadID = 0;
	HANDLE hThread = NULL;

	if (singleInstanceSearch)
	{
		if (::IsWindow(g_hWndChatHistory))
		{
			::PostMessage(g_hWndChatHistory, WM_WIN_BRING_TO_FRONT, 0, 0);
		}
		return 0;
	}

	singleInstanceSearch = true;

	hThread = ::CreateThread(NULL, 0, SearchAllUIThread, nullptr, 0, &dwThreadID);
	if (hThread == NULL)
	{
		::MessageBox(NULL, _T("ERROR: Cannot create chat history thread!!!"), _T("WoChat"), MB_OK);
	}
	return 0;
}

