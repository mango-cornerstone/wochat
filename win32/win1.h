#ifndef __WOCHAT_WINDOWS1_H__
#define __WOCHAT_WINDOWS1_H__

#include "dui/dui_win.h"

static const wchar_t txtWin1PK[] = { 0x516c,0x94a5,0x0020,0x003a,0x0020,0 };
static const wchar_t txtWin1DT[] = { 0x521b,0x5efa,0x65f6,0x95f4,0xff1a,0 };
static const wchar_t txtWin1TBD[] = 
{ 0x672c,0x529f,0x80fd,0x6b63,0x5728,0x5f00,0x53d1,0x4e2d,0xff0c,0x8bf7,0x8010,0x5fc3,0x7b49,0x5f85,0xff01,0x003a,0x002d,0x0029,0 };

enum Win1Mode
{
	WIN1_MODE_SHOWME = 0,
	WIN1_MODE_SHOWSEARCH,
	WIN1_MODE_SHOWTBD
};

class XWindow1 : public XWindowT <XWindow1>
{
private:
	Win1Mode m_mode = WIN1_MODE_SHOWSEARCH;
public:
	XWindow1()
	{
		m_backgroundColor = 0xFFEAECED;
		m_property |= (DUI_PROP_MOVEWIN | DUI_PROP_HANDLETIMER | DUI_PROP_HANDLEKEYBOARD | DUI_PROP_HANDLETEXT);;
		m_message = WM_XWINDOWS01;
	}

	~XWindow1()	{}

	Win1Mode GetMode() { return m_mode;  }
	void SetMode(Win1Mode mode) 
	{ 
		XControl* xctl;
		m_mode = mode;
		switch (m_mode)
		{
		case WIN1_MODE_SHOWSEARCH:
			m_backgroundColor = 0xFFEAECED;

			xctl = m_controlArray[XWIN1_BUTTON_SEARCH];
			assert(nullptr != xctl);
			xctl->HideMe(false);

			xctl = m_controlArray[XWIN1_EDITBOX_SEARCH];
			assert(nullptr != xctl);
			xctl->HideMe(false);

			xctl = m_controlArray[XWIN1_BUTTON_MYICON];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_LABEL_MYNAME];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY_VAL];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_CREATETIME];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_CREATETIME_VAL];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_TBD];
			assert(nullptr != xctl);
			xctl->HideMe();
			break;
		case WIN1_MODE_SHOWME:
			m_backgroundColor = 0xFFFFFFFF;

			xctl = m_controlArray[XWIN1_BUTTON_SEARCH];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_EDITBOX_SEARCH];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_BUTTON_MYICON];
			assert(nullptr != xctl);
			xctl->HideMe(false);

			xctl = m_controlArray[XWIN1_LABEL_MYNAME];
			assert(nullptr != xctl);
			xctl->HideMe(false);

			xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY];
			assert(nullptr != xctl);
			xctl->HideMe(false);
			xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY_VAL];
			assert(nullptr != xctl);
			xctl->HideMe(false);
			xctl = m_controlArray[XWIN1_LABEL_CREATETIME];
			assert(nullptr != xctl);
			xctl->HideMe(false);
			xctl = m_controlArray[XWIN1_LABEL_CREATETIME_VAL];
			assert(nullptr != xctl);
			xctl->HideMe(false);
			xctl = m_controlArray[XWIN1_LABEL_TBD];
			assert(nullptr != xctl);
			xctl->HideMe();

			break;
		case WIN1_MODE_SHOWTBD:
			m_backgroundColor = 0xFFFFFFFF;

			xctl = m_controlArray[XWIN1_BUTTON_SEARCH];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_EDITBOX_SEARCH];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_BUTTON_MYICON];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_LABEL_MYNAME];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY_VAL];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_CREATETIME];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_CREATETIME_VAL];
			assert(nullptr != xctl);
			xctl->HideMe();
			xctl = m_controlArray[XWIN1_LABEL_TBD];
			assert(nullptr != xctl);
			xctl->HideMe(false);

			break;
		default:
			break;
		}
	}

	void InitBitmap()
	{
		U8 id;
		XBitmap* bmp;

		int w = 27;
		int h = 27;
		id = XWIN1_BITMAP_SEARCHN; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachN; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_SEARCHH; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachH; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_SEARCHP; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachP; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_SEARCHA; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachH; bmp->w = w; bmp->h = h;

		w = 128;
		h = 128;
		id = XWIN1_BITMAP_MYICON; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)g_myImage128; bmp->w = w; bmp->h = h;
	}

	void InitControl()
	{
		U32 objSize;
		U8 id, *mem;

		m_maxControl = 0;
		assert(nullptr != m_pool);

		InitBitmap(); // inital all bitmap resource

		objSize = sizeof(XButton);
		id = XWIN1_BUTTON_SEARCH;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W1BTNSEARCH");
			XBitmap* bmpN = &m_bitmapArray[XWIN1_BITMAP_SEARCHN];
			XBitmap* bmpH = &m_bitmapArray[XWIN1_BITMAP_SEARCHH];
			XBitmap* bmpP = &m_bitmapArray[XWIN1_BITMAP_SEARCHP];
			XBitmap* bmpA = &m_bitmapArray[XWIN1_BITMAP_SEARCHA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
			m_activeControl = id; 
		}
		else return;

		id = XWIN1_EDITBOX_SEARCH;
		objSize = sizeof(XEditBoxLine);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XEditBoxLine* eb = new(mem)XEditBoxLine;
			assert(nullptr != eb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			eb->Init(id, "W1EDITSEARCH", g_pDWriteFactory, pTextFormat);
			eb->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = eb;
		}
		else return;

		id = XWIN1_BUTTON_MYICON;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W1BTNICON");
			XBitmap* bmp = &m_bitmapArray[XWIN1_BITMAP_MYICON];
			button->setBitmap(bmp, bmp, bmp, bmp);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
			button->HideMe();
		}

		id = XWIN1_LABEL_MYNAME;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_OTHER);
			assert(pTextFormat);

			lb->Init(id, "W1NAME", g_pDWriteFactory, pTextFormat);
			assert(g_myName);
			lb->setText(g_myName, wcslen(g_myName));
			m_controlArray[id] = lb;
			lb->HideMe();
		}

		id = XWIN1_LABEL_PUBLICKEY;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			assert(pTextFormat);
			lb->Init(id, "W1PK", g_pDWriteFactory, pTextFormat);
			lb->setText((wchar_t*)txtWin1PK, wcslen(txtWin1PK));
			m_controlArray[id] = lb;
			lb->HideMe();
		}

		id = XWIN1_LABEL_PUBLICKEY_VAL;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			wchar_t hexPK[67] = { 0 };
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			assert(pTextFormat);
			lb->Init(id, "W1PKValue", g_pDWriteFactory, pTextFormat);

			assert(g_PK);
			wt_Raw2HexStringW(g_PK, 33, hexPK, nullptr);
			lb->setText((wchar_t*)hexPK, wcslen(hexPK));
			m_controlArray[id] = lb;
			lb->HideMe();
		}

		id = XWIN1_LABEL_CREATETIME;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			assert(pTextFormat);
			lb->Init(id, "W1DT", g_pDWriteFactory, pTextFormat);
			lb->setText((wchar_t*)txtWin1DT, wcslen(txtWin1DT));
			m_controlArray[id] = lb;
			lb->HideMe();
		}

		id = XWIN1_LABEL_CREATETIME_VAL;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			wchar_t* dt = L"2024/02/27 18:56:32";
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			assert(pTextFormat);
			lb->Init(id, "W1DTValue", g_pDWriteFactory, pTextFormat);
			lb->setText(dt, wcslen(dt));
			m_controlArray[id] = lb;
			lb->HideMe();
		}

		id = XWIN1_LABEL_TBD;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			assert(pTextFormat);
			lb->Init(id, "W1PKValue", g_pDWriteFactory, pTextFormat);
			lb->setText((wchar_t*)txtWin1TBD, wcslen(txtWin1TBD));
			m_controlArray[id] = lb;
			lb->HideMe();
		}

		m_maxControl = XWIN1_LABEL_TBD + 1;
	}


public:
	void UpdateControlPosition()
	{
		XControl* xctl;
		int gap = 10; // pixel
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int sw, sh, dx, dy;
		int L, R, T, B;

		xctl = m_controlArray[XWIN1_BUTTON_SEARCH];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dy = (h - sh) >> 1;
		dx = w - sw - gap;
		xctl->setPosition(dx, dy);

		xctl = m_controlArray[XWIN1_EDITBOX_SEARCH];
		assert(nullptr != xctl);
		L = gap;
		R = dx - (gap >> 1);
		T = dy;
		B = dy + sh;
		xctl->setPosition(L, T, R, B);

		xctl = m_controlArray[XWIN1_BUTTON_MYICON];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dx = gap;
		dy = gap;
		xctl->setPosition(dx, dy);
		T = dy + sh + gap + gap + 1;

		dy = dy + sh;
		xctl = m_controlArray[XWIN1_LABEL_MYNAME];
		assert(nullptr != xctl);
		dx = dx + sw + gap;
		dy -= xctl->getHeight();
		xctl->setPosition(dx, dy);

		xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY];
		assert(nullptr != xctl);
		dx = gap;
		dy = T;
		xctl->setPosition(dx, dy);
		B = xctl->getHeight();

		xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY_VAL];
		assert(nullptr != xctl);
		dx = gap << 2;
		dy = T + B + gap;
		xctl->setPosition(dx, dy);
		T = dy + xctl->getHeight() + gap;

		xctl = m_controlArray[XWIN1_LABEL_CREATETIME];
		assert(nullptr != xctl);
		dx = gap;
		dy = T;
		xctl->setPosition(dx, dy);
		B = xctl->getHeight();

		xctl = m_controlArray[XWIN1_LABEL_CREATETIME_VAL];
		assert(nullptr != xctl);
		dx = gap << 2;
		dy = T + B + gap;
		xctl->setPosition(dx, dy);

		xctl = m_controlArray[XWIN1_LABEL_TBD];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dx = (w - sw) >> 1;
		dy = (h - sh) >> 1;
		xctl->setPosition(dx, dy);
	}

	int Do_DUI_PAINT(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		if (WIN1_MODE_SHOWME == m_mode)
		{
			int sw, sh, gap = 10; // pixel
			int w = m_area.right - m_area.left;
			int h = m_area.bottom - m_area.top;
			XControl* xctl;
			xctl = m_controlArray[XWIN1_BUTTON_MYICON];
			sw = xctl->getWidth();
			sh = xctl->getHeight();

			DUI_ScreenFillRect(m_screen, w, h, 0xFF000000, w - gap - gap, 1, gap, gap + sw + gap);
		}

		return 0; 
	}

	int SelectIconFile(LPWSTR upfile)
	{
		OPENFILENAME ofn = { 0 };       // common dialog box structure
		WCHAR path[MAX_PATH + 1] = { 0 };

		// Initialize OPENFILENAME
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFile = path;
		//
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of file to initialize itself.
		//
		ofn.lpstrFile[0] = _T('\0');
		ofn.nMaxFile = MAX_PATH; //sizeof(path) / sizeof(TCHAR);
		ofn.lpstrFilter = _T("PNG files(*.png)\0*.png\0\0");
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		/* Display the Open dialog box. */
		if (GetOpenFileName(&ofn) != TRUE)
			return 1;

		for (int i = 0; i < MAX_PATH; i++)
			upfile[i] = path[i];

		return 0;
	}


};

#endif  /* __WOCHAT_WINDOWS1_H__ */

