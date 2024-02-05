#ifndef __WOCHAT_WINDOWS1_H__
#define __WOCHAT_WINDOWS1_H__

#include "dui/dui_win.h"

wchar_t testxt[] = {
0x516c,0x94a5,0xff1a,0x0030,0x0032,0x0045,0x0041,0x0046,
0x0031,0x0041,0x0038,0x0041,0x0044,0x0039,0x0035,0x0036,
0x0037,0x0039,0x0036,0x0043,0x0036,0x0043,0x0041,0x0033,
0x0036,0x0030,0x0044,0x0044,0x0033,0x0043,0x0031,0x0044,
0x0035,0x0033,0x0043,0x0037,0x0035,0x0039,0x0041,0x0034,
0x0036,0x0042,0x0035,0x0044,0x0033,0x0037,0x0035,0x0031,
0x0036,0x0043,0x0046,0x0045,0x0045,0x0038,0x0034,0x0041,
0x0032,0x0043,0x0044,0x0034,0x0046,0x0043,0x0037,0x0039,
0x0041,0x0030,0x0033,0x0035,0x0045,0
};

enum Win1Mode
{
	WIN1_MODE_SHOWME = 0,
	WIN1_MODE_SHOWSEARCH
};

class XWindow1 : public XWindowT <XWindow1>
{
private:
	Win1Mode m_mode = WIN1_MODE_SHOWSEARCH;
#if 0
	IDWriteTextFormat* m_pTextFormat = nullptr;
	IDWriteTextLayout* m_pTextLayout = nullptr;
#endif
public:
	XWindow1()
	{
		m_backgroundColor = 0xFFEAECED;
		m_property |= (DUI_PROP_MOVEWIN |DUI_PROP_HANDLETIMER | DUI_PROP_HANDLEKEYBOARD);
		m_property = (DUI_PROP_MOVEWIN | DUI_PROP_HANDLEKEYBOARD);
		m_property = (DUI_PROP_MOVEWIN);
		m_message = WM_XWINDOWS01;
#if 0
		g_pDWriteFactory->CreateTextFormat(
			L"Microsoft Yahei",
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			16.0f,
			L"en-US",
			&m_pTextFormat
		);

		g_pDWriteFactory->CreateTextLayout(
			testxt,
			(UINT32)wcslen(testxt),
			m_pTextFormat,
			(FLOAT)600,
			(FLOAT)300,
			&m_pTextLayout
		);
#endif
	}

	~XWindow1()	{}

	Win1Mode GetMode() { return m_mode;  }
	void SetMode(Win1Mode mode) { m_mode = mode; }

	void InitBitmap()
	{
#if 0
		U8 id;
		XBitmap* bmp;

		int w = 27;
		int h = 27;
		id = XWIN1_BITMAP_SEARCHN; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachN; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_SEARCHH; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachH; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_SEARCHP; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachP; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_SEARCHA; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSerachH; bmp->w = w; bmp->h = h;
#endif
	}

	void InitControl()
	{
#if 0
		U32 objSize;
		U8 id, *mem;

		m_maxControl = 0;
		assert(nullptr != m_pool);

		InitBitmap(); // inital all bitmap resource

		objSize = sizeof(XLabel);
		id = XWIN1_LABEL_PUBLICKEY;
		mem = (U8*)palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			if (0 != lb->Init(dui_hCursorIBeam, g_pDWriteFactory, g_pTextFormat0))
			{
				pfree(mem);
			}
			else
			{
				lb->setId(id);
				lb->setText((U16*)testxt, wcslen(testxt));
				//lb->setStatus(XCONTROL_STATE_HIDDEN);
			}
			m_controlArray[id] = lb;
		}
		else return;

		m_maxControl = 1;
#endif
#if 0
		id = XWIN1_EDITBOX_SEARCH;
		m_startControl = id;
		objSize = sizeof(XEditBox);
		mem = (U8*)palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XEditBox* eb = new(mem)XEditBox;
			assert(nullptr != eb);
			if (0 != eb->Init(g_hCursorIBeam))
			{
				pfree(mem);
			}
			else
			{
				eb->setRoundColor(m_backgroundColor, m_backgroundColor);
				eb->setBkgFrontColor(0xFFFFFFFF, 0xFF555555);
				eb->setId(id);
				dui_controlArray[id] = eb;
				m_endControl = id;
			}
		}
		else return;

		id = XWIN1_BUTTON_SEARCH;
		objSize = sizeof(XButton);
		mem = (U8*)palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XBitmap* bmpN;
			XBitmap* bmpH;
			XBitmap* bmpP;
			XBitmap* bmpA;
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(g_hCursorHand);
			{
				bmpN = &m_bitmapArray[XWIN1_BITMAP_SEARCHN];
				bmpH = &m_bitmapArray[XWIN1_BITMAP_SEARCHH];
				bmpP = &m_bitmapArray[XWIN1_BITMAP_SEARCHP];
				bmpA = &m_bitmapArray[XWIN1_BITMAP_SEARCHA];
				button->setBitmap(bmpN, bmpH, bmpP, bmpA);
				button->setRoundColor(m_backgroundColor, m_backgroundColor);
				button->setId(id);
				dui_controlArray[id] = button;
				m_endControl = id;
			}
		}
#endif
	}


public:
	void UpdateControlPosition()
	{
#if 0
		XControl* xctl;
		int sw, sh, dx, dy, gap = 10; // pixel
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		xctl = m_controlArray[XWIN1_LABEL_PUBLICKEY];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dy = 20;
		dx = 10;
		xctl->setPosition(dx, dy);

		if (WIN1_MODE_SHOWME != m_mode)
		{
			//xctl->setStatus(XCONTROL_STATE_HIDDEN);
		}
#endif
	}

	int Do_DUI_CHAR(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{ 
		int r = 0;
#if 0
		U16 charCode = static_cast<U16>(wParam);

		XEditBox* eb = (XEditBox *)dui_controlArray[XWIN1_EDITBOX_SEARCH];
		assert(nullptr != eb);

		r = eb->OnKeyBoard(XKEYBOARD_NORMAL, charCode);
#endif
		return r; 
	}

	int Do_DUI_KEYDOWN(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = DUI_STATUS_NODRAW;
#if 0
		U32 keyCode = static_cast<U32>(wParam);

		bool heldShift = (GetKeyState(VK_SHIFT) & 0x80) != 0;
		bool heldControl = (GetKeyState(VK_CONTROL) & 0x80) != 0;

		XEditBox* eb = (XEditBox*)dui_controlArray[XWIN1_EDITBOX_SEARCH];
		assert(nullptr != eb);

		switch (keyCode)
		{
		case VK_RETURN:
		case VK_BACK:
		case VK_DELETE:
		case VK_TAB:
		case VK_LEFT:
			r = eb->MoveCursorLR(-1);
			break;
		case VK_RIGHT:
			r = eb->MoveCursorLR(1);
			break;
		case VK_UP:
		case VK_DOWN:
		case VK_HOME:
		case VK_END:
		case VK_INSERT:
		case 'C':
		case 'X':
		case 'A':
		case 'V':
		default:
			break;
		}
#endif
		return r;
	}

	int Do_DUI_PAINT(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
#if 0
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		DUI_ScreenFillRect(m_screen, w, h, 0xFF000000, w - 20, 1, 10, 153);
#endif
		return 0; 
	}

	int Do_DUI_MOUSEMOVE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		BOOL isTrailingHit = FALSE;
		BOOL isInside = FALSE;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		FLOAT x = (FLOAT)(xPos - (m_area.left + 20));
		FLOAT y = (FLOAT)(yPos - (m_area.top + 100));

#if 0
		DWRITE_HIT_TEST_METRICS htm = { 0 };

		if (nullptr != m_pTextLayout)
		{
			m_pTextLayout->HitTestPoint(x, y, &isTrailingHit, &isInside, &htm);
			if (isInside)
			{
				SetDUIWindowCursor();
				::SetCursor(dui_hCursorIBeam);
			}
		}
#endif
		return 0; 
	}
};

#endif  /* __WOCHAT_WINDOWS1_H__ */

