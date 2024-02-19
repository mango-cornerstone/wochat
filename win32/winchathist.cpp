#include "pch.h"
#include "resource.h"
#include "wochat.h"
#include "wochatdef.h"
#include "winchathist.h"

static bool singleInstanceChatHistory = false;
static wchar_t titleChatHistory[] = { 0x804a, 0x5929, 0x5386, 0x53f2, 0x8bb0, 0x5f55, 0 };

class XWinChatHist : public ATL::CWindowImpl<XWinChatHist>
{
public:
	DECLARE_XWND_CLASS(NULL, IDR_MAINFRAME, 0)

	BEGIN_MSG_MAP(XWinChatHist)
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


DWORD WINAPI ChatHistoryUIThread(LPVOID lpData)
{
	DWORD dwRet = 0;
	RECT rw = { 0 };
	MSG msg = { 0 };

	InterlockedIncrement(&g_threadCount);

	XWinChatHist winChatHist;

	rw.left = 100; rw.top = 10; rw.right = rw.left + 600; rw.bottom = rw.top + 800;

	winChatHist.Create(NULL, rw, titleChatHistory, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE);
	if (FALSE == winChatHist.IsWindow())
		goto ExitChatHistUIThread;

	g_hWndChatHistory = winChatHist.m_hWnd;
	winChatHist.ShowWindow(SW_SHOW);

	// Main message loop:
	while ((0 == g_Quit) && GetMessageW(&msg, nullptr, 0, 0)) 
	{
		if (!TranslateAcceleratorW(msg.hwnd, 0, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	if (winChatHist.IsWindow())
		winChatHist.DestroyWindow();

ExitChatHistUIThread:
	InterlockedDecrement(&g_threadCount);
	singleInstanceChatHistory = false;
	g_hWndChatHistory = nullptr;
	return 0;
}

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
		::MessageBox(NULL, _T("ERROR: Cannot create chat history thread!!!"), _T("WoChat"), MB_OK);
	}
	return 0;
}

