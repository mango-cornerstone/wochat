/*
 * 
 *  This is the entry of WoChat application
 * 
 */

#include "pch.h"  // the precompiled header file for Visual C++

#ifndef __cplusplus
#error WoChat Application requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
#error WoChat Application requires atlbase.h to be included first
#endif

#ifndef __ATLWIN_H__
#error WoChat Application requires atlwin.h to be included first
#endif

#if !defined(_WIN64)
#error WoChat Application can only compiled in X64 mode
#endif

#include "resource.h"
#include "xbitmapdata.h"
#include "wochatdef.h"
#include "wochat.h"
#include "winlog.h"
#include "mosquitto.h"
#include "win0.h"
#include "win1.h"
#include "win2.h"
#include "win3.h"
#include "win4.h"
#include "win5.h"
#include "winscreen.h"
#include "winchathist.h"
#include "winaudiocall.h"

#if _DEBUG
static const U8* default_private_key = (const U8*)"E3589E51A04EAB9343E2AD073890A1A2C1B172167D486C38045D88C6FD20CBBC";
static const U8* default_public_key  = (const U8*)"02E3687B2D10B91B81730AA49DAB8BA42BCD25770D2B1C604897DDF2211ACBFE81";
#else
static const U8* default_private_key = (const U8*)"59AEA450B4C91512A5CD0465C50786911ED0BC671AF820EDFA163CF7CEA5345C";
static const U8* default_public_key  = (const U8*)"0288D216B2CEB02171FF4FD3392C2A1AC388F41FBD8392A719F690829F23BCA8E5";
#endif
////////////////////////////////////////////////////////////////
// global variables
////////////////////////////////////////////////////////////////
LONG 				g_threadCount = 0;
LONG				g_Quit = 0;
LONG                g_NetworkStatus = 0;
DWORD               g_dwMainThreadID  = 0;

U32  				g_messageSequence = 0;
HTAB*				g_messageHTAB = nullptr;
HTAB*               g_keyHTAB = nullptr;
MemoryPoolContext   g_messageMemPool = nullptr;
MemoryPoolContext   g_topMemPool = nullptr;

wchar_t*            g_myName = nullptr;
U8*                 g_myImage32 = nullptr;
U8*                 g_myImage128 = nullptr;

// private key and public key
U8*                 g_SK = nullptr;
U8*                 g_PK = nullptr;

U8*                 g_PKTo = nullptr;
U8*                 g_MQTTPubClientId = nullptr;
U8*                 g_MQTTSubClientId = nullptr;
HINSTANCE			g_hInstance = nullptr;
HANDLE				g_MQTTPubEvent;
HWND				g_hWndShareScreen = nullptr;
HWND				g_hWndChatHistory = nullptr;
HWND				g_hWndAudioCall = nullptr;


CRITICAL_SECTION    g_csMQTTSub;
CRITICAL_SECTION    g_csMQTTPub;

MQTTPubData			g_PubData = { 0 };
MQTTSubData			g_SubData = { 0 };

ID2D1Factory*		g_pD2DFactory = nullptr;
IDWriteFactory*		g_pDWriteFactory = nullptr;
IDWriteTextFormat*  g_pTextFormat[8] = { 0 };

D2D1_DRAW_TEXT_OPTIONS d2dDrawTextOptions = D2D1_DRAW_TEXT_OPTIONS_NONE;

HCURSOR				g_hCursorWE = nullptr;
HCURSOR				g_hCursorNS = nullptr;
HCURSOR				g_hCursorHand = nullptr;
HCURSOR				g_hCursorIBeam = nullptr;
wchar_t				g_AppPath[MAX_PATH + 1] = {0};
wchar_t				g_DBPath[MAX_PATH + 1] = { 0 };

CAtlWinModule _Module;

////////////////////////////////////////////////////////////////
// local static variables
////////////////////////////////////////////////////////////////
static HMODULE hDLLD2D{};
static HMODULE hDLLDWrite{};

IDWriteTextFormat* GetTextFormat(U8 idx)
{
	if (idx < 8)
		return g_pTextFormat[idx];

	return nullptr;
}

int GetSecretKey(U8* sk, U8* pk)
{
	return 0;
}

// three background threads
DWORD WINAPI LoadingDataThread(LPVOID lpData);
DWORD WINAPI MQTTSubThread(LPVOID lpData);
DWORD WINAPI MQTTPubThread(LPVOID lpData);


#if 0
LRESULT CALLBACK WndProcEmoji(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
/************************************************************************************************
*  The layout of the Main Window
*
* +------+------------------------+-----------------------------------------------------+
* |      |         Win1           |                       Win3                          |
* |      +------------------------+-----------------------------------------------------+
* |      |                        |                                                     |
* |      |                        |                                                     |
* |      |                        |                       Win4                          |
* | Win0 |                        |                                                     |
* |      |         Win2           |                                                     |
* |      |                        +-----------------------------------------------------|
* |      |                        |                                                     |
* |      |                        |                       Win5                          |
* |      |                        |                                                     |
* +------+------------------------+-----------------------------------------------------+
*
*
* We have one vertical bar and three horizonal bars.
*
*************************************************************************************************
*/

// the main window class for the whole application
class XWindow : public ATL::CWindowImpl<XWindow>
{
public:
	DECLARE_XWND_CLASS(NULL, IDR_MAINFRAME, 0)

private:
	enum
	{
		STEPXY = 1,
		SPLITLINE_WIDTH = 1,
		MOVESTEP = 1
	};

	enum class DrapMode { dragModeNone, dragModeV, dragModeH };
	DrapMode m_dragMode = DrapMode::dragModeNone;

	enum class AppMode 
	{ 
		ModeMe,
		ModeTalk,
		ModeFriend,
		ModeQuan,
		ModeCoin,
		ModeFavorite,
		ModeFile,
		ModeSetting
	};

	AppMode m_mode = AppMode::ModeTalk;

	U32* m_screenBuff = nullptr;
	U32  m_screenSize = 0;

	RECT m_rectClient = { 0 };

	int m_splitterVPos = -1;               // splitter bar position
	int m_splitterVPosOld = -1;            // keep align value
	int m_splitterHPos = -1;
	int m_splitterHPosOld = -1;

	int m_cxyDragOffset = 0;		// internal

	int m_splitterVPosToLeft = 300;       // the minum pixel to the left of the client area.
	int m_splitterVPosToRight = 400;       // the minum pixel to the right of the client area.
	int m_splitterHPosToTop = (XWIN3_HEIGHT + XWIN4_HEIGHT);        // the minum pixel to the top of the client area.
	int m_splitterHPosToBottom = XWIN5_HEIGHT;       // the minum pixel to the right of the client area.

	int m_marginLeft = 64;
	int m_marginRight = 0;

	int m_splitterHPosfix0 = XWIN1_HEIGHT;
	int m_splitterHPosfix1 = XWIN3_HEIGHT;

	UINT m_nDPI = 96;
	float m_deviceScaleFactor = 1.f;
	U8 m_isInThisWindow = 0;

	U8 m_loadingPercent = 0;

	XWindow0 m_win0;
	XWindow1 m_win1;
	XWindow2 m_win2;
	XWindow3 m_win3;
	XWindow4 m_win4;
	XWindow5 m_win5;

	int m0 = 0;
	int m1 = 0;
	int m2 = 0;
	int m3 = 0;
	int m4 = 0;
	int m5 = 0;
	int mdrag = 0;

	ID2D1HwndRenderTarget* m_pD2DRenderTarget = nullptr;
	ID2D1Bitmap* m_pixelBitmap0               = nullptr;
	ID2D1Bitmap* m_pixelBitmap1               = nullptr;
	ID2D1Bitmap* m_pixelBitmap2               = nullptr;
	ID2D1SolidColorBrush* m_pTextBrushSel     = nullptr;
	ID2D1SolidColorBrush* m_pTextBrush0       = nullptr;
	ID2D1SolidColorBrush* m_pTextBrush1       = nullptr;
	ID2D1SolidColorBrush* m_pCaretBrush       = nullptr;
	ID2D1SolidColorBrush* m_pBkgBrush0        = nullptr;
	ID2D1SolidColorBrush* m_pBkgBrush1        = nullptr;

public:
	XWindow()
	{
		m_rectClient.left = m_rectClient.right = m_rectClient.top = m_rectClient.bottom = 0;
		m_win0.SetWindowId((const U8*)"DUIWin0", 7);
		m_win1.SetWindowId((const U8*)"DUIWin1", 7);
		m_win2.SetWindowId((const U8*)"DUIWin2", 7);
		m_win3.SetWindowId((const U8*)"DUIWin3", 7);
		m_win4.SetWindowId((const U8*)"DUIWin4", 7);
		m_win5.SetWindowId((const U8*)"DUIWin5", 7);
	}

	~XWindow()
	{
		if (nullptr != m_screenBuff)
			VirtualFree(m_screenBuff, 0, MEM_RELEASE);

		m_screenBuff = nullptr;
		m_screenSize = 0;
	}

	BEGIN_MSG_MAP(XWindow)
		MESSAGE_HANDLER_DUIWINDOW(DUI_ALLMESSAGE, OnDUIWindowMessage)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_MQTT_SUBMESSAGE, OnSubMessage)
		MESSAGE_HANDLER(WM_MQTT_PUBMESSAGE, OnPubMessage)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
		MESSAGE_HANDLER(WM_MOUSEHOVER, OnMouseHover)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDoubleClick)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		COMMAND_ID_HANDLER(IDM_EDITBOX_COPY, OnEditBoxCopy)
		COMMAND_ID_HANDLER(IDM_EDITBOX_CUT, OnEditBoxCut)
		COMMAND_ID_HANDLER(IDM_EDITBOX_PASTE, OnEditBoxPaste)
		MESSAGE_HANDLER(WM_XWINDOWS00, OnWin0Message)
		MESSAGE_HANDLER(WM_XWINDOWS01, OnWin1Message)
		MESSAGE_HANDLER(WM_XWINDOWS02, OnWin2Message)
		MESSAGE_HANDLER(WM_XWINDOWS03, OnWin3Message)
		MESSAGE_HANDLER(WM_XWINDOWS04, OnWin4Message)
		MESSAGE_HANDLER(WM_XWINDOWS05, OnWin5Message)
		MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
		MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_LOADPERCENTMESSAGE, OnLoading)
	END_MSG_MAP()

	// this is the routine to process all window messages for win0/1/2/3/4/5
	LRESULT OnDUIWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		WPARAM wp = wParam;
		LPARAM lp = lParam;

		ClearDUIWindowReDraw(); // reset the bit to indicate the redraw before we handle the messages

		switch (uMsg)
		{
		case WM_MOUSEWHEEL:
			{
				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				ScreenToClient(&pt);
				lp = MAKELONG(pt.x, pt.y);
			}
			break;
		case WM_CREATE:
			wp = (WPARAM)m_hWnd;
			break;
		default:
			break;
		}

		m_win0.HandleOSMessage((U32)uMsg, (U64)wp, (U64)lp, this);
		m_win1.HandleOSMessage((U32)uMsg, (U64)wp, (U64)lp, this);
		m_win2.HandleOSMessage((U32)uMsg, (U64)wp, (U64)lp, this);
		m_win3.HandleOSMessage((U32)uMsg, (U64)wp, (U64)lp, this);
		m_win4.HandleOSMessage((U32)uMsg, (U64)wp, (U64)lp, this);
		m_win5.HandleOSMessage((U32)uMsg, (U64)wp, (U64)lp, this);

		if (DUIWindowNeedReDraw()) // after all 5 virtual windows procced the window message, the whole window may need to fresh
		{
			// U64 s = DUIWindowNeedReDraw();
			Invalidate(); // send out the WM_PAINT messsage to the window to redraw
		}

		// to allow the host window to continue to handle the windows message
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL bHandled)
	{
		DWORD dwThreadID;
		HANDLE hThread = nullptr;
		XChatGroup* cg;
		m_loadingPercent = 0;

		hThread = ::CreateThread(NULL, 0, LoadingDataThread, m_hWnd, 0, &dwThreadID);
		if (nullptr == hThread)
		{
			MessageBox(TEXT("Loading thread creation is failed!"), TEXT("WoChat Error"), MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
		}

		cg = m_win2.GetSelectedChatGroup();
		if (cg)
		{
			//m_win3.UpdateTitle(cg->name);
			m_win4.SetChatGroup(cg);
		}

		m_nDPI = GetDpiForWindow(m_hWnd);
		UINT_PTR id = SetTimer(XWIN_CARET_TIMER, 666);

		return 0;
	}

	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return 0; // don't want flicker
	}

	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL bHandled)
	{
		KillTimer(XWIN_CARET_TIMER);
		
		// tell all threads to quit
		InterlockedIncrement(&g_Quit);

		SetEvent(g_MQTTPubEvent); // tell MQTT pub thread to quit gracefully
		
		ReleaseUnknown(m_pBkgBrush0);
		ReleaseUnknown(m_pBkgBrush1);
		ReleaseUnknown(m_pCaretBrush);
		ReleaseUnknown(m_pTextBrushSel);
		ReleaseUnknown(m_pTextBrush0);
		ReleaseUnknown(m_pTextBrush1);
		ReleaseUnknown(m_pixelBitmap0);
		ReleaseUnknown(m_pixelBitmap1);
		ReleaseUnknown(m_pixelBitmap2);
		ReleaseUnknown(m_pD2DRenderTarget);

		PostQuitMessage(0);

		return 0;
	}

	LRESULT OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;

		lpMMI->ptMinTrackSize.x = 1024;
		lpMMI->ptMinTrackSize.y = 768;

		return 0;
	}

	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT OnLoading(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_loadingPercent = (U8)wParam;
		Invalidate();

		if (m_loadingPercent > 100) // the loading is completed
		{
			DWORD dwThreadID0;
			DWORD dwThreadID1;
			HANDLE hThread0;
			HANDLE hThread1;
			// we create two threads. One thread for the MQTT subscription, and one for publication.
			hThread0 = ::CreateThread(NULL, 0, MQTTSubThread, m_hWnd, 0, &dwThreadID0);

			hThread1 = ::CreateThread(NULL, 0, MQTTPubThread, m_hWnd, 0, &dwThreadID1);

			if (nullptr == hThread0 || nullptr == hThread1)
			{
				MessageBox(TEXT("MQTT thread creation is failed!"), TEXT("WoChat Error"), MB_OK);
			}
		}

		return 0;
	}

	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (m_loadingPercent <= 100)
			return 0;

		if (::GetCapture() == m_hWnd)
		{
			bool bChanged = false;
			POINT pt = { 0 };
			int xyPos;

			switch (wParam)
			{
#if 0
			case VK_RETURN:
			case VK_ESCAPE:
				::ReleaseCapture();
				break;
#endif
			case VK_LEFT:
			case VK_RIGHT:
			case VK_UP:
			case VK_DOWN:
				if (DrapMode::dragModeV == m_dragMode)
				{
					::GetCursorPos(&pt);
					int xyPos = m_splitterVPos + ((wParam == VK_LEFT) ? -MOVESTEP : MOVESTEP);
					if (xyPos >= m_splitterVPosToLeft && xyPos < (m_rectClient.right - m_splitterVPosToRight))
					{
						pt.x += (xyPos - m_splitterVPos);
						::SetCursorPos(pt.x, pt.y);
						bChanged = true;
					}
				}
				if (DrapMode::dragModeH == m_dragMode)
				{
					::GetCursorPos(&pt);
					int xyPos = m_splitterHPos + ((wParam == VK_UP) ? -MOVESTEP : MOVESTEP);

					if (xyPos >= m_splitterHPosToTop && xyPos < (m_rectClient.bottom - m_splitterHPosToBottom))
					{
						pt.y += (xyPos - m_splitterHPos);
						::SetCursorPos(pt.x, pt.y);
						bChanged = true;
					}
				}

				if(bChanged)
					Invalidate();

				break;
			default:
				break;
			}
			
		}
		return 0;
	}

	LRESULT OnPubMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (!wParam)
		{
			PushTaskIntoSendMessageQueue(nullptr);
		}
		return 0;
	}

	LRESULT OnSubMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		int ret = 0;

		if (0 == wParam)
		{
			int rc;
			MQTTSubData* pd = &g_SubData;
			MessageTask* p;

			EnterCriticalSection(&g_csMQTTSub);
			p = pd->task;
			while (p) // scan the link to find the message that has been processed
			{
				rc = 0;
				if (MESSAGE_TASK_STATE_NULL == p->state) // this task is not processed yet.
				{
					switch (p->type)
					{
					case 'T':
						rc = m_win4.UpdateReceivedMessage(p);
						if (rc)
							SendConfirmationMessage(p->pubkey, p->hash); // send the confirmation to the sender that we got this message
						break;
					case 'C':
						rc = m_win4.UpdateMessageConfirmation(p->pubkey, p->hash);
						break;
					default:
						break;
					}
				}
				ret += rc;
				p = p->next;
			}
			LeaveCriticalSection(&g_csMQTTSub);

			if (ret)
				Invalidate();
		}
		return ret;
	}

	LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled = DUIWindowCursorIsSet() ? TRUE : FALSE;

		ClearDUIWindowCursor();

		if (((HWND)wParam == m_hWnd) && (LOWORD(lParam) == HTCLIENT) && m_loadingPercent > 100)
		{
			DWORD dwPos = ::GetMessagePos();

			POINT ptPos = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };

			ScreenToClient(&ptPos);

			DrapMode mode = IsOverSplitterBar(ptPos.x, ptPos.y);
			if (DrapMode::dragModeNone != mode)
				bHandled = TRUE;
		}

		return 0;
	}

	bool IsOverSplitterRect(int x, int y) const
	{
		// -1 == don't check
		return (((x == -1) || ((x >= m_rectClient.left) && (x < m_rectClient.right))) &&
			((y == -1) || ((y >= m_rectClient.top) && (y < m_rectClient.bottom))));
	}

	DrapMode IsOverSplitterBar(int x, int y) const
	{
		if (m_splitterVPos > 0)
		{
			int width = SPLITLINE_WIDTH << 1;
			if (!IsOverSplitterRect(x, y))
				return DrapMode::dragModeNone;

			if ((x >= m_splitterVPos) && (x < (m_splitterVPos + width)))
				return DrapMode::dragModeV;

			if (m_splitterHPos > 0)
			{
				if ((x >= m_splitterVPos + width) && (y >= m_splitterHPos) && (y < (m_splitterHPos + width)))
					return DrapMode::dragModeH;
			}
		}

		return DrapMode::dragModeNone;
	}

	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_isInThisWindow = 0;
		return 0;
	}

	LRESULT OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		if (m_loadingPercent <= 100)
			return 0;

		if (DUIWindowInDragMode())
		{
			mdrag = 1;
			ATLASSERT(DrapMode::dragModeNone == m_dragMode);

			if (::GetCapture() != m_hWnd)
				SetCapture();
		}
		else
		{
			m_dragMode = IsOverSplitterBar(xPos, yPos);
			switch (m_dragMode)
			{
			case DrapMode::dragModeV:
				m_cxyDragOffset = xPos - m_splitterVPos;
				break;
			case DrapMode::dragModeH:
				m_cxyDragOffset = yPos - m_splitterHPos;
				break;
			default:
				if (::GetCapture() == m_hWnd)
					::ReleaseCapture();
				return 0;
			}

			if (::GetCapture() != m_hWnd)
			{
				ClearDUIWindowLBtnDown();
				SetCapture();
			}
		}
		return 0;
	}

	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_loadingPercent <= 100)
			return 0;

		ClearDUIWindowLBtnDown();
		if (::GetCapture() == m_hWnd)
		{
			mdrag = 0;
			::ReleaseCapture();
		}
		return 0;
	}

	LRESULT OnLButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	// When the virtial or horizonal bar is changed, We need to change the position of window 1/2/3/4/5
	// window 0 is fixed
	void AdjustDUIWindowPosition()
	{
		if (nullptr != m_screenBuff)
		{
			U32  size;
			XRECT area;
			XRECT* r = &area;
			U32* dst = m_screenBuff;

			// windows 0
			r->left = m_rectClient.left;
			r->right = XWIN0_WIDTH;
			r->top = m_rectClient.top;
			r->bottom = m_rectClient.bottom;
			m_win0.UpdateSize(r, dst);
			size = (U32)((r->right - r->left) * (r->bottom - r->top));
			dst += size;
			// windows 1
			r->left = r->right; r->right = m_splitterVPos; r->top = m_rectClient.top; r->bottom = m_splitterHPosfix0;
			size = (U32)((r->right - r->left) * (r->bottom - r->top));
			m_win1.UpdateSize(r, dst);
			dst += size;
			// windows 2
			r->top = m_splitterHPosfix0 + SPLITLINE_WIDTH; r->bottom = m_rectClient.bottom;
			size = (U32)((r->right - r->left) * (r->bottom - r->top));
			m_win2.UpdateSize(r, dst);
			// windows 3
			dst += size;
			r->left = m_splitterVPos + SPLITLINE_WIDTH; r->right = m_rectClient.right; r->top = m_rectClient.top; r->bottom = m_splitterHPosfix1;
			size = (U32)((r->right - r->left) * (r->bottom - r->top));
			m_win3.UpdateSize(r, dst);
			// windows 4
			dst += size;
			r->top = m_splitterHPosfix1 + SPLITLINE_WIDTH; r->bottom = m_splitterHPos;
			size = (U32)((r->right - r->left) * (r->bottom - r->top));
			m_win4.UpdateSize(r, dst);
			// windows 5
			dst += size;
			r->top = m_splitterHPos + SPLITLINE_WIDTH; r->bottom = m_rectClient.bottom;
			size = (U32)((r->right - r->left) * (r->bottom - r->top));
			m_win5.UpdateSize(r, dst);
		}
	}

	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		if (!m_isInThisWindow)
		{
			m_isInThisWindow = 1;

			TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
			tme.dwFlags = TME_LEAVE | TME_HOVER;
			tme.hwndTrack = m_hWnd;
			tme.dwHoverTime = 3000; // in ms
			TrackMouseEvent(&tme);
		}

		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		if (!DUIWindowInDragMode() && !DUIWindowLButtonDown())
		{
			bool bChanged = false;
			if (::GetCapture() == m_hWnd)
			{
				int	newSplitPos;
				switch (m_dragMode)
				{
				case DrapMode::dragModeV:
					{
						newSplitPos = xPos - m_cxyDragOffset;
						if (m_splitterVPos != newSplitPos) // the position is changed
						{
							if (newSplitPos >= m_splitterVPosToLeft && newSplitPos < (m_rectClient.right - m_rectClient.left - m_splitterVPosToRight))
							{
								m_splitterVPos = newSplitPos;
								m_splitterVPosOld = m_splitterVPos - m_rectClient.left;
								bChanged = true;
							}
						}
					}
					break;
				case DrapMode::dragModeH:
					{
						newSplitPos = yPos - m_cxyDragOffset;
						if (m_splitterHPos != newSplitPos)
						{
							if (newSplitPos >= m_splitterHPosToTop && newSplitPos < (m_rectClient.bottom - m_rectClient.top - m_splitterHPosToBottom))
							{
								m_splitterHPos = newSplitPos;
								m_splitterHPosOld = m_rectClient.bottom - m_splitterHPos;
								bChanged = true;
							}
						}
					}
					break;
				default:
					ATLASSERT(0);
					break;
				}
				if (bChanged)
					AdjustDUIWindowPosition();
			}
			else
			{
				DrapMode mode = IsOverSplitterBar(xPos, yPos);
				switch (mode)
				{
				case DrapMode::dragModeV:
					ATLASSERT(nullptr != g_hCursorWE);
					::SetCursor(g_hCursorWE);
					break;
				case DrapMode::dragModeH:
					ATLASSERT(nullptr != g_hCursorNS);
					::SetCursor(g_hCursorNS);
					break;
				default:
					break;
				}
			}
			if (bChanged)
				Invalidate();
		}

		return 0;
	}

	LRESULT OnWin0Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bool bChanged = true;

		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		if (wParam == 0)
		{
			U8 ctlId = (U8)lParam;
			ClearDUIWindowLBtnDown();
			switch (ctlId)
			{
			case XWIN0_BUTTON_ME:
				m_mode = AppMode::ModeMe;
				m_splitterHPosfix0 = 0;
				m_splitterHPosfix1 = 0;
				m_splitterVPos = 0;
				m_splitterHPos = 0;
				DoMeModeLayOut();
				break;
			case XWIN0_BUTTON_TALK:
				m_mode = AppMode::ModeTalk;
				m_splitterHPosfix0 = XWIN1_HEIGHT;
				m_splitterHPosfix1 = XWIN3_HEIGHT;
				m_splitterVPos = m_splitterVPosOld;
				m_splitterHPos = (m_rectClient.bottom - m_rectClient.top) - m_splitterHPosOld;
				DoTalkModeLayOut();
				break;
			case XWIN0_BUTTON_FRIEND:
				m_mode = AppMode::ModeFriend;
				break;
			case XWIN0_BUTTON_QUAN:
			case XWIN0_BUTTON_COIN:
			case XWIN0_BUTTON_FAVORITE:
			case XWIN0_BUTTON_FILE:
			case XWIN0_BUTTON_SETTING:
				m_mode = AppMode::ModeCoin;
				m_splitterHPosfix0 = 0;
				m_splitterHPosfix1 = 0;
				m_splitterVPos = 0;
				m_splitterHPos = 0;
				DoTBDModeLayOut();
				break;
			default:
				bChanged = false;
				break;
			}
		}

		if(bChanged)
			Invalidate();

		return 0;
	}

	U32 Win1ChangeIcon()
	{

		return WT_OK;
	}

	LRESULT OnWin1Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		if (wParam == 0)
		{
			U8 ctlId = (U8)lParam;
			ClearDUIWindowLBtnDown();
			switch (ctlId)
			{
			case XWIN1_BUTTON_MYICON:
				{
					WCHAR path[MAX_PATH + 1] = { 0 };
					U32 rc = m_win1.SelectIconFile(path);
					if (WT_OK == rc)
					{
						rc++;
					}
				}
				break;
			default:
				break;
			}
		}
		return 0;
	}

	LRESULT OnWin2Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT OnWin3Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	int GetCurrentPublicKey(U8* pk)
	{
		return m_win4.GetPublicKey(pk);
	}

	LRESULT OnWin4Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		switch (wParam)
		{
		case WIN4_UPDATE_MESSAGE:
			{
				MessageTask* mt = (MessageTask*)lParam;
				if (mt)
				{
					if (m_win4.UpdateMyMessage(mt))
						Invalidate();
				}
			}
			break;
		default:
			break;
		}
		return 0;
	}

	int ShowEmojiPopWindow()
	{
#if 0
		const TCHAR szClassName[] = TEXT("MyWindowClass");
		WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProcEmoji, 0, 0,
						  g_hInstance, NULL, LoadCursor(NULL, IDC_ARROW),
						  (HBRUSH)(COLOR_WINDOW + 1), NULL, szClassName, NULL };
		if (!RegisterClassEx(&wc))
		{
			MessageBox(L"Window Registration Failed!", L"Error!", MB_OK);
			return 0;
		}

		HWND hwnd = CreateWindowEx(0, szClassName, TEXT("Popup Window"), WS_POPUP,
			300, 300, 400, 300, NULL, NULL, g_hInstance, NULL);
		if (hwnd == NULL)
		{
			MessageBox(L"Window CreateWindowEx Failed!", L"Error!", MB_OK);
			return 0;
		}

		::ShowWindow(hwnd, SW_SHOW);
		::UpdateWindow(hwnd);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return (int)msg.wParam;
#endif 
		return 0;
	}

	int DoScreenCapture()
	{
		return 0;
	}

	LRESULT OnWin5Message(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		if (wParam == 0)
		{
			U8 ctlId = (U8)lParam;
			switch (ctlId)
			{
			case XWIN5_BUTTON_EMOJI:
				ShowEmojiPopWindow();
				break;
			case XWIN5_BUTTON_UPLOAD:
				{
					WCHAR path[MAX_PATH + 1] = { 0 };
					int rc = m_win5.SelectUploadedFile(path);
					if (0 == rc)
					{
						// upload the file 
						rc++;
					}
				}
				break;
			case XWIN5_BUTTON_CAPTURE:
				DoScreenCapture();
				break;
			case XWIN5_BUTTON_CHATHISTORY:
				if (g_dwMainThreadID)
					::PostThreadMessage(g_dwMainThreadID, WM_WIN_CHATHISTHREAD, 0, 0L);
				break;
			case XWIN5_BUTTON_AUDIOCALL:
				if (g_dwMainThreadID)
					::PostThreadMessage(g_dwMainThreadID, WM_WIN_AUDIOTHREAD, 0, 0L);
				break;
			case XWIN5_BUTTON_VIDEOCALL:
				if(g_dwMainThreadID)
					::PostThreadMessage(g_dwMainThreadID, WM_WIN_SCREENTHREAD, 0, 0L);
				break;
			case XWIN5_BUTTON_SENDMESSAGE:
				break;
			default:
				break;
			}
		}
		return 0;
	}

	U32	DoMeModeLayOut()
	{
		XRECT area;
		XRECT* r;
		XRECT* xr = m_win0.GetWindowArea();
		area.left = xr->left;  area.top = xr->top; area.right = xr->right; area.bottom = xr->bottom;
		r = &area;

		U32* dst = m_screenBuff;
		U32 size = (U32)((r->right - r->left) * (r->bottom - r->top));
		dst += size;

		// windows 1
		r->left = r->right;
		r->right = m_rectClient.right;
		r->top = m_rectClient.top;
		r->bottom = m_rectClient.bottom;
		size = (U32)((r->right - r->left) * (r->bottom - r->top));
		m_win1.UpdateSize(r, dst);
		m_win1.SetMode(WIN1_MODE_SHOWME);
		m_win2.WindowHide();
		m_win3.WindowHide();
		m_win4.WindowHide();
		m_win5.WindowHide();

		return WT_OK;
	}

	U32	DoTalkModeLayOut()
	{
		m_win1.WindowShow();
		m_win2.WindowShow();
		m_win3.WindowShow();
		m_win4.WindowShow();
		m_win5.WindowShow();
		AdjustDUIWindowPosition();
		return WT_OK;
	}

	U32	DoFriendModeLayOut()
	{
		return WT_OK;
	}

	U32	DoTBDModeLayOut()
	{
		XRECT area;
		XRECT* r;
		XRECT* xr = m_win0.GetWindowArea();
		area.left = xr->left;  area.top = xr->top; area.right = xr->right; area.bottom = xr->bottom;
		r = &area;

		U32* dst = m_screenBuff;
		U32 size = (U32)((r->right - r->left) * (r->bottom - r->top));
		dst += size;

		// windows 1
		r->left = r->right;
		r->right = m_rectClient.right;
		r->top = m_rectClient.top;
		r->bottom = m_rectClient.bottom;
		size = (U32)((r->right - r->left) * (r->bottom - r->top));
		m_win1.UpdateSize(r, dst);
		m_win1.SetMode(WIN1_MODE_SHOWTBD);
		m_win2.WindowHide();
		m_win3.WindowHide();
		m_win4.WindowHide();
		m_win5.WindowHide();

		return WT_OK;
	}


	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (SIZE_MINIMIZED != wParam)
		{
			ReleaseUnknown(m_pD2DRenderTarget);

			GetClientRect(&m_rectClient);
			ATLASSERT(0 == m_rectClient.left);
			ATLASSERT(0 == m_rectClient.top);
			ATLASSERT(m_rectClient.right > 0);
			ATLASSERT(m_rectClient.bottom > 0);

			U32 w = (U32)(m_rectClient.right - m_rectClient.left);
			U32 h = (U32)(m_rectClient.bottom - m_rectClient.top);

			if (nullptr != m_screenBuff)
			{
				VirtualFree(m_screenBuff, 0, MEM_RELEASE);
				m_screenBuff = nullptr;
				m_screenSize = 0;
			}

			m_screenSize = DUI_ALIGN_PAGE(w * h * sizeof(U32));
			ATLASSERT(m_screenSize >= (w * h * sizeof(U32)));

			m_screenBuff = (U32*)VirtualAlloc(NULL, m_screenSize, MEM_COMMIT, PAGE_READWRITE);
			if (nullptr == m_screenBuff)
			{
				m_win0.UpdateSize(nullptr, nullptr);
				m_win1.UpdateSize(nullptr, nullptr);
				m_win2.UpdateSize(nullptr, nullptr);
				m_win3.UpdateSize(nullptr, nullptr);
				m_win4.UpdateSize(nullptr, nullptr);
				m_win5.UpdateSize(nullptr, nullptr);
				Invalidate();
				return 0;
			}

			if (m_splitterVPos < 0)
			{
				m_splitterVPos = (XWIN0_WIDTH + XWIN1_WIDTH);
				if (m_splitterVPos < m_splitterVPosToLeft)
					m_splitterVPos = m_splitterVPosToLeft;

				if (m_splitterVPos > (m_rectClient.right - m_rectClient.left - m_splitterVPosToRight))
				{
					m_splitterVPos = (m_rectClient.right - m_rectClient.left - m_splitterVPosToRight);
					ATLASSERT(m_splitterVPos > m_splitterVPosToLeft);
				}
				m_splitterVPosOld = m_splitterVPos;
			}

			if (m_splitterHPos < 0)
			{
				m_splitterHPos = m_rectClient.bottom - m_rectClient.top - m_splitterHPosToBottom;
				if (m_splitterHPos < m_splitterHPosToTop)
					m_splitterHPos = m_splitterHPosToTop;

				if (m_splitterHPos > (m_rectClient.bottom - m_rectClient.top - m_splitterHPosToBottom))
				{
					m_splitterHPos = m_rectClient.bottom - m_rectClient.top - m_splitterHPosToBottom;
					ATLASSERT(m_splitterHPos > m_splitterHPosToTop);
				}
				m_splitterHPosOld = (m_rectClient.bottom - m_rectClient.top) - m_splitterHPos;
			}

			switch (m_mode)
			{
			case AppMode::ModeTalk:
				m_splitterHPosfix0 = XWIN1_HEIGHT;
				m_splitterHPosfix1 = XWIN3_HEIGHT;
				m_splitterVPos = m_splitterVPosOld;
				m_splitterHPos = (m_rectClient.bottom - m_rectClient.top) - m_splitterHPosOld;
				ATLASSERT(m_splitterVPos > 0);
				ATLASSERT(m_splitterHPos > 0);
				DoTalkModeLayOut();
				break;
			case AppMode::ModeFriend:
				m_splitterHPosfix0 = XWIN1_HEIGHT;
				m_splitterHPosfix1 = XWIN3_HEIGHT;
				m_splitterVPos = m_splitterVPosOld;
				m_splitterHPos = (m_rectClient.bottom - m_rectClient.top) - m_splitterHPosOld;
				ATLASSERT(m_splitterVPos > 0);
				ATLASSERT(m_splitterHPos > 0);
				DoTalkModeLayOut();
				break;
			case AppMode::ModeQuan:
			case AppMode::ModeCoin:
			case AppMode::ModeFavorite:
			case AppMode::ModeFile:
			case AppMode::ModeSetting:
				DoTBDModeLayOut();
				break;
			case AppMode::ModeMe:
				m_splitterHPosfix0 = 0;
				m_splitterHPosfix1 = 0;
				m_splitterVPos = 0;
				m_splitterHPos = 0;
				DoMeModeLayOut();
				break;
			default:
				ATLASSERT(0);
				break;
			}
			Invalidate();
		}
		return 0;
	}

	void ShowLoadingProgress(int left, int top, int right, int bottom)
	{
		int w = right - left;
		int h = bottom - top;

		ATLASSERT(w > 0 && h > 0);

		if (w > 500 && h > 100)
		{
			int offset = m_loadingPercent * 5;
			int L = (w - 500) >> 1;
			int T = (h - 8) >> 1;
			int R = L + 500;
			int B = T + 8;

			D2D1_RECT_F r1 = D2D1::RectF(static_cast<FLOAT>(L),	static_cast<FLOAT>(T), static_cast<FLOAT>(R), static_cast<FLOAT>(T+1));
			m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap1, &r1);

			D2D1_RECT_F r2 = D2D1::RectF(static_cast<FLOAT>(L), static_cast<FLOAT>(B), static_cast<FLOAT>(R), static_cast<FLOAT>(B + 1));
			m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap1, &r2);

			D2D1_RECT_F r3 = D2D1::RectF(static_cast<FLOAT>(L), static_cast<FLOAT>(T), static_cast<FLOAT>(L+1), static_cast<FLOAT>(B));
			m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap1, &r3);
			
			D2D1_RECT_F r4 = D2D1::RectF(static_cast<FLOAT>(R), static_cast<FLOAT>(T), static_cast<FLOAT>(R+1), static_cast<FLOAT>(B));
			m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap1, &r4);

			D2D1_RECT_F r5 = D2D1::RectF(static_cast<FLOAT>(L), static_cast<FLOAT>(T), static_cast<FLOAT>(L + offset), static_cast<FLOAT>(B));
			m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap1, &r5);
		}
	}

	void DrawSeperatedLines()
	{
		if (nullptr != m_pixelBitmap0)
		{
			if (m_splitterVPos > 0)
			{
				D2D1_RECT_F rect = D2D1::RectF(
					static_cast<FLOAT>(m_splitterVPos),
					static_cast<FLOAT>(m_rectClient.top),
					static_cast<FLOAT>(m_splitterVPos + SPLITLINE_WIDTH), // m_cxySplitBar),
					static_cast<FLOAT>(m_rectClient.bottom)
				);
				m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap0, &rect);
			}
			if (m_splitterHPosfix0 > 0)
			{
				D2D1_RECT_F rect = D2D1::RectF(
					static_cast<FLOAT>(XWIN0_WIDTH),
					static_cast<FLOAT>(m_splitterHPosfix0),
					static_cast<FLOAT>(m_splitterVPos),
					static_cast<FLOAT>(m_splitterHPosfix0 + SPLITLINE_WIDTH)
				);
				m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap0, &rect);
			}
			if (m_splitterHPosfix1 > 0)
			{
				D2D1_RECT_F rect = D2D1::RectF(
					static_cast<FLOAT>(m_splitterVPos + 2),
					static_cast<FLOAT>(m_splitterHPosfix1),
					static_cast<FLOAT>(m_rectClient.right),
					static_cast<FLOAT>(m_splitterHPosfix1 + SPLITLINE_WIDTH)
				);
				m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap0, &rect);
			}
			if (m_splitterHPos > 0)
			{
				D2D1_RECT_F rect = D2D1::RectF(
					static_cast<FLOAT>(m_splitterVPos + SPLITLINE_WIDTH),
					static_cast<FLOAT>(m_splitterHPos),
					static_cast<FLOAT>(m_rectClient.right),
					static_cast<FLOAT>(m_splitterHPos + SPLITLINE_WIDTH)
				);
				m_pD2DRenderTarget->DrawBitmap(m_pixelBitmap0, &rect);
			}
		}
	}

	int GetFirstIntegralMultipleDeviceScaleFactor() const noexcept 
	{
		return static_cast<int>(std::ceil(m_deviceScaleFactor));
	}

	D2D1_SIZE_U GetSizeUFromRect(const RECT& rc, const int scaleFactor) noexcept 
	{
		const long width = rc.right - rc.left;
		const long height = rc.bottom - rc.top;
		const UINT32 scaledWidth = width * scaleFactor;
		const UINT32 scaledHeight = height * scaleFactor;
		return D2D1::SizeU(scaledWidth, scaledHeight);
	}

	HRESULT CreateDeviceDependantResource(int left, int top, int right, int bottom)
	{
		HRESULT hr = S_OK;
		if (nullptr == m_pD2DRenderTarget)  // Create a Direct2D render target.
		{
			RECT rc;
			const int integralDeviceScaleFactor = GetFirstIntegralMultipleDeviceScaleFactor();
			D2D1_RENDER_TARGET_PROPERTIES drtp{};
			drtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
			drtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
			drtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
			drtp.dpiX = 96.f * integralDeviceScaleFactor;
			drtp.dpiY = 96.f * integralDeviceScaleFactor;
			// drtp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN);
			drtp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);

			rc.left = left; rc.top = top; rc.right = right; rc.bottom = bottom;
			D2D1_HWND_RENDER_TARGET_PROPERTIES dhrtp{};
			dhrtp.hwnd = m_hWnd;
			dhrtp.pixelSize = GetSizeUFromRect(rc, integralDeviceScaleFactor);
			dhrtp.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

			ATLASSERT(nullptr != g_pD2DFactory);

			//hr = g_pD2DFactory->CreateHwndRenderTarget(renderTargetProperties, hwndRenderTragetproperties, &m_pD2DRenderTarget);
			hr = g_pD2DFactory->CreateHwndRenderTarget(drtp, dhrtp, &m_pD2DRenderTarget);

			if (S_OK == hr && nullptr != m_pD2DRenderTarget)
			{
				ReleaseUnknown(m_pBkgBrush0);
				ReleaseUnknown(m_pBkgBrush1);
				ReleaseUnknown(m_pCaretBrush);
				ReleaseUnknown(m_pTextBrushSel);
				ReleaseUnknown(m_pTextBrush0);
				ReleaseUnknown(m_pTextBrush1);
				ReleaseUnknown(m_pixelBitmap0);
				ReleaseUnknown(m_pixelBitmap1);
				//ReleaseUnknown(m_pixelBitmap2);

				U32 pixel[1] = { 0xFFEEEEEE };
				hr = m_pD2DRenderTarget->CreateBitmap(
					D2D1::SizeU(1, 1), pixel, 4, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
					&m_pixelBitmap0);
				if (S_OK == hr && nullptr != m_pixelBitmap0)
				{
					pixel[0] = 0xFF056608;
					hr = m_pD2DRenderTarget->CreateBitmap(
						D2D1::SizeU(1, 1), pixel, 4, D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
						&m_pixelBitmap1);
					if (S_OK == hr && nullptr != m_pixelBitmap1)
					{
						hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x000000), &m_pTextBrush0);
						if (S_OK == hr && nullptr != m_pTextBrush0)
						{
							hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x000000), &m_pTextBrush1);
							if (S_OK == hr && nullptr != m_pTextBrush1)
							{
								hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x87CEFA), &m_pTextBrushSel);
								if (S_OK == hr && nullptr != m_pTextBrushSel)
								{
									hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x000000), &m_pCaretBrush);
									if (S_OK == hr && nullptr != m_pCaretBrush)
									{
										hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x95EC6A), &m_pBkgBrush0);
										if (S_OK == hr && nullptr != m_pBkgBrush0)
											hr = m_pD2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF), &m_pBkgBrush1);
									}
								}
							}
						}
					}
				}
			}
		}
		return hr;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		static U32 track = 0;
		static wchar_t xtitle[256+1] = { 0 };
		PAINTSTRUCT ps;
		BeginPaint(&ps);

		RECT rc;
		GetClientRect(&rc);

		HRESULT hr = CreateDeviceDependantResource(rc.left, rc.top, rc.right, rc.bottom);

		if (S_OK == hr && nullptr != m_pD2DRenderTarget 
			&& nullptr != m_pixelBitmap0
			&& nullptr != m_pixelBitmap1
			&& nullptr != m_pTextBrush0 
			&& nullptr != m_pTextBrush1 
			&& nullptr != m_pTextBrushSel
			&& nullptr != m_pCaretBrush
			&& nullptr != m_pBkgBrush0
			&& nullptr != m_pBkgBrush1)
		{
			m_pD2DRenderTarget->BeginDraw();
			////////////////////////////////////////////////////////////////////////////////////////////////////
			//m_pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
			if (m_loadingPercent > 100)
			{
				DrawSeperatedLines(); // draw seperator lines

				// draw five windows
				m0 += m_win0.Draw(m_pD2DRenderTarget, m_pTextBrush0, m_pTextBrushSel, m_pCaretBrush, m_pBkgBrush0, m_pBkgBrush1);
				m1 += m_win1.Draw(m_pD2DRenderTarget, m_pTextBrush0, m_pTextBrushSel, m_pCaretBrush, m_pBkgBrush0, m_pBkgBrush1);
				m2 += m_win2.Draw(m_pD2DRenderTarget, m_pTextBrush0, m_pTextBrushSel, m_pCaretBrush, m_pBkgBrush0, m_pBkgBrush1);
				m3 += m_win3.Draw(m_pD2DRenderTarget, m_pTextBrush0, m_pTextBrushSel, m_pCaretBrush, m_pBkgBrush0, m_pBkgBrush1);
				m4 += m_win4.Draw(m_pD2DRenderTarget, m_pTextBrush0, m_pTextBrushSel, m_pCaretBrush, m_pBkgBrush0, m_pBkgBrush1);
				m5 += m_win5.Draw(m_pD2DRenderTarget, m_pTextBrush0, m_pTextBrushSel, m_pCaretBrush, m_pBkgBrush0, m_pBkgBrush1);
			}
			else
			{
				m_pD2DRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));
				ShowLoadingProgress(rc.left, rc.top, rc.right, rc.bottom);
			}

			hr = m_pD2DRenderTarget->EndDraw();
			////////////////////////////////////////////////////////////////////////////////////////////////////
			if (FAILED(hr) || D2DERR_RECREATE_TARGET == hr)
			{
				ReleaseUnknown(m_pD2DRenderTarget);
			}
		}

		swprintf((wchar_t*)xtitle, 256, L"WoChat - [%06d] thread: %d W0:%d - W1:%d - W2:%d - W3:%d - W4:%d - W5:%d -", ++track, (int)g_threadCount,
			m0,m1,m2,m3,m4,m5);
		::SetWindowTextW(m_hWnd, (LPCWSTR)xtitle);

		EndPaint(&ps);
		return 0;
	}

	LRESULT OnEditBoxCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		return 0;
	}

	LRESULT OnEditBoxCut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		return 0;
	}

	LRESULT OnEditBoxPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		int r;
		if (m_loadingPercent <= 100) // the loading phase is not completed yet
			return 0;

		r = m_win5.DoEditBoxPaste();
		if (r)
			Invalidate();
		return 0;
	}

};

class CWoChatThreadManager
{
public:
	// the main UI thread proc
	static DWORD WINAPI MainUIThread(LPVOID lpData)
	{
		DWORD dwRet = 0;
		RECT rw = { 0 };
		MSG msg = { 0 };

		InterlockedIncrement(&g_threadCount);

		XWindow winMain;

		rw.left = 100; rw.top = 100; rw.right = rw.left + 1024; rw.bottom = rw.top + 768;
		winMain.Create(NULL, rw, _T("WoChat"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE);
		if (FALSE == winMain.IsWindow())
			goto ExitMainUIThread;

		//winMain.CenterWindow();
		winMain.ShowWindow(SW_SHOW);

		// Main message loop:
		//while ((0 == g_Quit) && GetMessageW(&msg, nullptr, 0, 0)) 
		while (GetMessageW(&msg, nullptr, 0, 0))
		{
			if (!TranslateAcceleratorW(msg.hwnd, 0, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}

	ExitMainUIThread:
		InterlockedDecrement(&g_threadCount);
		return dwRet;
	}

	DWORD m_dwCount = 0;
	HANDLE m_arrThreadHandles[MAXIMUM_WAIT_OBJECTS - 1];
	DWORD  m_arrThreadIDs[MAXIMUM_WAIT_OBJECTS - 1];

	CWoChatThreadManager()
	{
		for (int i = 0; i < (MAXIMUM_WAIT_OBJECTS - 1); i++)
			m_arrThreadHandles[i] = nullptr;
	}

	// Operations
	DWORD AddThread(int uiType)
	{
		DWORD dwThreadID = 0;
		HANDLE hThread = NULL;

		if (m_dwCount == (MAXIMUM_WAIT_OBJECTS - 1))
		{
			::MessageBox(NULL, _T("ERROR: Cannot create ANY MORE threads!!!"), _T("WoChat"), MB_OK);
			return 0;
		}

		switch (uiType)
		{
		case WM_WIN_MAINUITHREAD:
			hThread = ::CreateThread(NULL, 0, MainUIThread, nullptr, 0, &dwThreadID);
			if (hThread == NULL)
			{
				::MessageBox(NULL, _T("ERROR: Cannot create thread!!!"), _T("WoChat"), MB_OK);
				return 0;
			}
			break;
#if 0
		case WM_WIN_SCREENTHREAD:
			hThread = ::CreateThread(NULL, 0, ShareScreenUIThread, nullptr, 0, &dwThreadID);
			if (hThread == NULL)
			{
				::MessageBox(NULL, _T("ERROR: Cannot create thread!!!"), _T("WoChat"), MB_OK);
				return 0;
			}
			break;
#endif
		default:
			break;
		}
		if (hThread)
		{
			m_arrThreadHandles[m_dwCount] = hThread;
			m_arrThreadIDs[m_dwCount] = dwThreadID;
			m_dwCount++;
		}
		return dwThreadID;
	}

	void RemoveThread(DWORD dwIndex)
	{
		::CloseHandle(m_arrThreadHandles[dwIndex]);
		if (dwIndex != (m_dwCount - 1))
		{
			m_arrThreadHandles[dwIndex] = m_arrThreadHandles[m_dwCount - 1];
			m_arrThreadIDs[dwIndex]     = m_arrThreadIDs[m_dwCount - 1];
		}
		m_dwCount--;
	}

	int Run(LPTSTR lpstrCmdLine, int nCmdShow)
	{
		int rc = 0;
		MSG msg;
		// force message queue to be created
		::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

		AddThread(WM_WIN_MAINUITHREAD);

		int nRet = m_dwCount;
		DWORD dwRet;
		while (m_dwCount > 0)
		{
			dwRet = ::MsgWaitForMultipleObjects(m_dwCount, m_arrThreadHandles, FALSE, INFINITE, QS_ALLINPUT);

			if (dwRet == 0xFFFFFFFF)
			{
				::MessageBox(NULL, _T("ERROR: Wait for multiple objects failed!!!"), _T("WoChat"), MB_OK);
			}
			else if (dwRet >= WAIT_OBJECT_0 && dwRet <= (WAIT_OBJECT_0 + m_dwCount - 1))
			{
#if 0
				if (dwRet == WAIT_OBJECT_0) /* the user close the main window, so we close all other UI thread */
				{
					InterlockedIncrement(&g_Quit); // tell all other threads to quit
					for (DWORD i = 1; i < m_dwCount; i++) /* tell all other the UI threads to quit. */
					{
						::PostThreadMessage(m_arrThreadIDs[i], WM_QUIT, 0, 0);
						Sleep(100);
						RemoveThread(i);
					}
					RemoveThread(dwRet - WAIT_OBJECT_0);
					break;
				}
				else
#endif 
				if (dwRet == WAIT_OBJECT_0)
					InterlockedIncrement(&g_Quit);
				RemoveThread(dwRet - WAIT_OBJECT_0);
			}
			else if (dwRet == (WAIT_OBJECT_0 + m_dwCount))
			{
				if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					switch (msg.message)
					{
					case WM_WIN_SCREENTHREAD:
						rc = CreateShareScreenThread(default_public_key);
						break;
					case WM_WIN_CHATHISTHREAD:
						rc = CreateChatHistoryThread(default_public_key);
						break;
					case WM_WIN_AUDIOTHREAD:
						rc = CreateAudioCallThread(default_public_key);
						break;
					default:
						break;
					}
				}
			}
			else
			{
				::MessageBeep((UINT)-1);
			}
		}

		return nRet;
	}
};

static void LoadD2DOnce()
{
	DWORD loadLibraryFlags = 0;
	HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");

	if (kernel32) 
	{
		if (::GetProcAddress(kernel32, "SetDefaultDllDirectories")) 
		{
			// Availability of SetDefaultDllDirectories implies Windows 8+ or
			// that KB2533623 has been installed so LoadLibraryEx can be called
			// with LOAD_LIBRARY_SEARCH_SYSTEM32.
			loadLibraryFlags = LOAD_LIBRARY_SEARCH_SYSTEM32;
		}
	}

	typedef HRESULT(WINAPI* D2D1CFSig)(D2D1_FACTORY_TYPE factoryType, REFIID riid, CONST D2D1_FACTORY_OPTIONS* pFactoryOptions, IUnknown** factory);
	typedef HRESULT(WINAPI* DWriteCFSig)(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown** factory);

	hDLLD2D = ::LoadLibraryEx(TEXT("D2D1.DLL"), 0, loadLibraryFlags);
	D2D1CFSig fnD2DCF = DLLFunction<D2D1CFSig>(hDLLD2D, "D2D1CreateFactory");

	if (fnD2DCF) 
	{
		// A single threaded factory as Scintilla always draw on the GUI thread
		fnD2DCF(D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory),
			nullptr,
			reinterpret_cast<IUnknown**>(&g_pD2DFactory));
	}

	hDLLDWrite = ::LoadLibraryEx(TEXT("DWRITE.DLL"), 0, loadLibraryFlags); 
	DWriteCFSig fnDWCF = DLLFunction<DWriteCFSig>(hDLLDWrite, "DWriteCreateFactory");
	if (fnDWCF) 
	{
		const GUID IID_IDWriteFactory2 = // 0439fc60-ca44-4994-8dee-3a9af7b732ec
		{ 0x0439fc60, 0xca44, 0x4994, { 0x8d, 0xee, 0x3a, 0x9a, 0xf7, 0xb7, 0x32, 0xec } };

		const HRESULT hr = fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
			IID_IDWriteFactory2,
			reinterpret_cast<IUnknown**>(&g_pDWriteFactory));
	
		if (SUCCEEDED(hr)) 
		{
			// D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
			d2dDrawTextOptions = static_cast<D2D1_DRAW_TEXT_OPTIONS>(0x00000004);
		}
		else 
		{
			fnDWCF(DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(&g_pDWriteFactory));
		}
	}
}

static bool LoadD2D() 
{
	static std::once_flag once;

	ReleaseUnknown(g_pD2DFactory);
	ReleaseUnknown(g_pDWriteFactory);

	std::call_once(once, LoadD2DOnce);

	return g_pDWriteFactory && g_pD2DFactory;
}


static int InitDirectWriteTextFormat(HINSTANCE hInstance)
{
	U8 idx;
	HRESULT hr = S_OK;
	
	ATLASSERT(g_pDWriteFactory);

	for (U8 i = 0; i < 8; i++)
	{
		ReleaseUnknown(g_pTextFormat[i]);
	}

	idx = WT_TEXTFORMAT_MAINTEXT;
	hr = g_pDWriteFactory->CreateTextFormat(
		L"Microsoft Yahei",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		13.5f,
		L"en-US",
		&(g_pTextFormat[idx])
	);

	if (S_OK != hr || nullptr == g_pTextFormat[idx])
	{
		return (20 + idx);
	}

	idx = WT_TEXTFORMAT_TITLE;
	hr = g_pDWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		11.5f,
		L"en-US",
		&(g_pTextFormat[idx])
	);

	if (S_OK != hr || nullptr == g_pTextFormat[idx])
	{
		return (20 + idx);
	}

	idx = WT_TEXTFORMAT_GROUPNAME;
	hr = g_pDWriteFactory->CreateTextFormat(
		L"Microsoft Yahei",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		17.f,
		L"en-US",
		&(g_pTextFormat[idx])
	);

	if (S_OK != hr || nullptr == g_pTextFormat[idx])
	{
		return (20 + idx);
	}

	idx = WT_TEXTFORMAT_OTHER;
	hr = g_pDWriteFactory->CreateTextFormat(
		L"Microsoft Yahei",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.f,
		L"en-US",
		&(g_pTextFormat[idx])
	);

	if (S_OK != hr || nullptr == g_pTextFormat[idx])
	{
		return (20 + idx);
	}

	return 0;
}

static int GetApplicationPath(HINSTANCE hInstance)
{
	int iRet = 0;
	DWORD len, i = 0;

	len = GetModuleFileNameW(hInstance, g_AppPath, MAX_PATH);
	if (len > 0)
	{
		for (i = len - 1; i > 0; i--)
		{
			if (g_AppPath[i] == L'\\')
				break;
		}
		g_AppPath[i] = L'\0';
	}
	else
		g_AppPath[0] = L'\0';

	if (i == 0 || i > (MAX_PATH - 10))
		return 30;

	for (DWORD k = 0; k < i; k++)
		g_DBPath[k] = g_AppPath[k];

	g_DBPath[i] = L'\\';
	g_DBPath[i+1] = L'w'; g_DBPath[i + 2] = L't'; g_DBPath[i + 3] = L'.'; g_DBPath[i + 4] = L'd'; g_DBPath[i + 5] = L'b';
	g_DBPath[i+6] = L'\0';

	if (WT_OK != CheckWoChatDatabase(g_DBPath))
		iRet = 31;

	return iRet;
}

/*
 * MemoryContextInit
 *		Start up the memory-context subsystem.
 *
 * This must be called before creating contexts or allocating memory in
 * contexts.  
 */
static U32 MemoryContextInit()
{
	U32 ret = WT_FAIL;

	ATLASSERT(g_topMemPool == nullptr);
	ATLASSERT(g_SK == nullptr);
	ATLASSERT(g_PK == nullptr);
	ATLASSERT(g_PKTo == nullptr);
	ATLASSERT(g_MQTTPubClientId == nullptr);
	ATLASSERT(g_MQTTSubClientId == nullptr);

	g_topMemPool = wt_mempool_create("TopMemoryContext", 0, DUI_ALLOCSET_SMALL_INITSIZE, DUI_ALLOCSET_SMALL_MAXSIZE);
	if (g_topMemPool)
	{
		g_SK   = (U8*)wt_palloc0(g_topMemPool, 32);
		g_PK   = (U8*)wt_palloc0(g_topMemPool, 33);
		g_PKTo = (U8*)wt_palloc0(g_topMemPool, 33);
		g_MQTTPubClientId = (U8*)wt_palloc0(g_topMemPool, 23);
		g_MQTTSubClientId = (U8*)wt_palloc0(g_topMemPool, 23);

		if (g_SK && g_PK && g_PKTo && g_MQTTPubClientId && g_MQTTSubClientId)
		{
			U8 hash[32] = { 0 };
			wt_HexString2Raw((U8*)default_private_key, 64, g_SK, nullptr);
			wt_HexString2Raw((U8*)default_public_key, 66, g_PKTo, nullptr);
			ret = GenPublicKeyFromSecretKey(g_SK, g_PK);

			wt_sha256_hash(g_SK, 32, hash);
			wt_Raw2HexString(hash, 11, g_MQTTPubClientId, nullptr); // The MQTT client Id is 23 bytes long
			wt_Raw2HexString(hash+16, 11, g_MQTTSubClientId, nullptr); // The MQTT client Id is 23 bytes long
		}
	}

	g_messageMemPool = wt_mempool_create("MessagePool", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
	if (!g_messageMemPool)
		ret = WT_MEMORY_POOL_ERROR;

	{
		HASHCTL hctl = { 0 };
		hctl.keysize = 32; // the SHA256 is 32 bytes
		hctl.entrysize = hctl.keysize + sizeof(void*);
		g_messageHTAB = hash_create("MainMessageHTAB", 256, &hctl, HASH_ELEM | HASH_BLOBS);
		if (!g_messageHTAB)
			ret = WT_DYNAMIC_HASHTAB_ERR;
	}

#if 0
	{
		HASHCTL hctl = { 0 };
		hctl.keysize = 33; // the SHA256 is 32 bytes
		hctl.entrysize = hctl.keysize + sizeof(void*);
		g_messageHTAB = hash_create("MainMessageHTAB", 256, &hctl, HASH_ELEM | HASH_BLOBS);
		if (!g_messageHTAB)
			r = 1;
	}

	{
		U8 utf8[] = { 0xE7,0x8B,0xA1,0xE5,0x85,0x94,0xE4,0xB8,0x89,0xE7,0xAA,0x9F };
		U32 len;
		U32 rc = wt_UTF8ToUTF16(utf8, 12, nullptr, &len);
		if (len > 0)
		{
			U16* output = (U16*)wt_palloc(g_messageMemPool, (len + 1)*sizeof(U16));
			if (output)
			{
				len = 0;
				rc = wt_UTF8ToUTF16(utf8, 12, output, &len);
				output[len] = 0;
				wchar_t* p = (wchar_t*)output;
				wt_pfree(output);
			}
		}
		rc++;
	}
	{
		U16 utf16[] = { 0x72e1,0x5154,0x4e09,0x7a9f };
		U32 len;
		U32 rc = wt_UTF16ToUTF8(utf16, 4, nullptr, &len);
		if (len > 0)
		{
			U8 utf8[12] = { 0 };
			rc = wt_UTF16ToUTF8(utf16, 4, utf8, nullptr);
			rc++;
#if 0
			U8* output = (U8*)wt_palloc(g_messageMemPool, len + 1);
			if (output)
			{

				len = 0;
				rc = wt_UTF8ToUTF16(utf8, 12, output, &len);
				output[len] = 0;
				wchar_t* p = (wchar_t*)output;
				wt_pfree(output);
#endif
		}
	}
#endif
	return ret;
}

static void MemoryContextTerm()
{
	hash_destroy(g_messageHTAB);
	g_messageHTAB = nullptr;
	wt_mempool_destroy(g_messageMemPool);
	g_messageMemPool = nullptr;
	wt_mempool_destroy(g_topMemPool);
	g_topMemPool = nullptr;
}

static int InitInstance(HINSTANCE hInstance)
{
	int iRet = 0;
	DWORD length = 0;
	HRESULT hr = S_OK;

	g_myImage32 = (U8*)xbmpIcon32;
	g_myImage128 = (U8*)xbmpIcon128;

	INITCOMMONCONTROLSEX iccex;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES;

	BOOL bRet = InitCommonControlsEx(&iccex);
	if (!bRet)
		return 1;

	iRet = MemoryContextInit();
	if (iRet)
		return 1;

	g_MQTTPubEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (NULL == g_MQTTPubEvent)
	{
		return 2;
	}

	InitializeCriticalSection(&g_csMQTTSub);
	InitializeCriticalSection(&g_csMQTTPub);

	g_hCursorWE = ::LoadCursor(NULL, IDC_SIZEWE);
	g_hCursorNS = ::LoadCursor(NULL, IDC_SIZENS);
	g_hCursorHand = ::LoadCursor(nullptr, IDC_HAND);
	g_hCursorIBeam = ::LoadCursor(NULL, IDC_IBEAM);
	if (NULL == g_hCursorWE || NULL == g_hCursorNS || NULL == g_hCursorHand || NULL == g_hCursorIBeam)
	{
		return 3;
	}

	if (!LoadD2D())
	{
		return 4;
	}

	iRet = InitDirectWriteTextFormat(hInstance);
	if (iRet)
		return 5;

	iRet = GetApplicationPath(hInstance);
	if (iRet)
		return 6;

	iRet = MQTT::MQTT_Init();

	if (MOSQ_ERR_SUCCESS != iRet)
		return 7;

	iRet = DUI_Init();

	g_PubData.host = MQTT_DEFAULT_HOST;
	g_PubData.port = MQTT_DEFAULT_PORT;

	g_SubData.host = MQTT_DEFAULT_HOST;
	g_SubData.port = MQTT_DEFAULT_PORT;

	g_messageSequence = wt_GenRandomU32(0x7FFFFFFF);

	return 0;
}

static void ExitInstance(HINSTANCE hInstance)
{
	UINT i, tries;

	// tell all threads to quit
	InterlockedIncrement(&g_Quit);

	// wait for all threads to quit gracefully
	tries = 20;
	while (g_threadCount && tries > 0)
	{
		Sleep(1000);
		tries--;
	}

	DUI_Term();

	ReleaseUnknown(g_pDWriteFactory);
	ReleaseUnknown(g_pD2DFactory);
	
	if (hDLLDWrite) 
	{
		FreeLibrary(hDLLDWrite);
		hDLLDWrite = {};
	}
	if (hDLLD2D) 
	{
		FreeLibrary(hDLLD2D);
		hDLLD2D = {};
	}

	CloseHandle(g_MQTTPubEvent);
	DeleteCriticalSection(&g_csMQTTSub);
	DeleteCriticalSection(&g_csMQTTPub);

	MQTT::MQTT_Term();
	MemoryContextTerm();
}

// The main entry of WoChat
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	int iRet = 0;
	HRESULT hr;

	UNREFERENCED_PARAMETER(hPrevInstance);

	g_Quit = 0;
	g_threadCount = 0;
	g_hInstance = hInstance;  // save the instance
	g_dwMainThreadID = GetCurrentThreadId();

	// The Microsoft Security Development Lifecycle recommends that all
	// applications include the following call to ensure that heap corruptions
	// do not go unnoticed and therefore do not introduce opportunities
	// for security exploits.
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (S_OK != hr)
	{
		MessageBoxW(NULL, L"The calling of CoInitializeEx() is failed", L"WoChat Error", MB_OK);
		return 0;
	}

	iRet = InitInstance(hInstance);

	if (iRet)
	{
		wchar_t msg[MAX_PATH + 1] = { 0 };
		swprintf((wchar_t*)msg, MAX_PATH, L"InitInstance() is failed. Return code:[%d]", iRet);

		MessageBoxW(NULL, (LPCWSTR)msg, L"WoChat Error", MB_OK);

		goto ExitThisApplication;
	}
	else  // BLOCK: Run application
	{
		iRet = DoWoChatLoginOrRegistration(hInstance);
		if (0 == iRet)
		{
			CWoChatThreadManager mgr;
			iRet = mgr.Run(lpCmdLine, nShowCmd);
		}
	}

ExitThisApplication:
	ExitInstance(hInstance);
	CoUninitialize();

	return 0;
}

// get the public key of the current receiver
int GetReceiverPublicKey(void* parent, U8* pk)
{
	int r = 1;
	XWindow* pThis = static_cast<XWindow*>(parent);

	if (pThis && pk)
	{
		r = pThis->GetCurrentPublicKey(pk);
	}
	return r;
}

#if 0
LRESULT CALLBACK WndProcEmoji(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;

}
#endif

