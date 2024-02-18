#ifndef __WOCHAT_WINSCREEN_H__
#define __WOCHAT_WINSCREEN_H__

class XWinScreen : public ATL::CWindowImpl<XWinScreen>
{
public:
	DECLARE_XWND_CLASS(NULL, IDR_MAINFRAME, 0)

	XWinScreen()
	{
	}

	~XWinScreen()
	{
	}

	BEGIN_MSG_MAP(XWinScreen)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
		//MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return 0; // don't want flicker
	}

#if 0
	LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL bHandled)
	{
		return 0;
	}


	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL bHandled)
	{
		PostQuitMessage(0);
		return 0;
	}
#endif
};

#endif // __WOCHAT_WINSCREEN_H__