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
public:
	XWindow1()
	{
		m_backgroundColor = 0xFFEAECED;
		m_property |= (DUI_PROP_MOVEWIN | DUI_PROP_HANDLETIMER | DUI_PROP_HANDLEKEYBOARD | DUI_PROP_HANDLETEXT);;
		m_message = WM_XWINDOWS01;
	}

	~XWindow1()	{}

	Win1Mode GetMode() { return m_mode;  }
	void SetMode(Win1Mode mode) { m_mode = mode; }

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
		mem = (U8*)palloc(m_pool, objSize);
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
		mem = (U8*)palloc(m_pool, objSize);
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

		m_maxControl = 2;
	}


public:
	void UpdateControlPosition()
	{
		XControl* xctl;
		int gap = 10; // pixel
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		xctl = m_controlArray[XWIN1_BUTTON_SEARCH];
		assert(nullptr != xctl);
		int sw = xctl->getWidth();
		int sh = xctl->getHeight();
		int dy = (h - sh) >> 1;
		int dx = w - sw - gap;
		xctl->setPosition(dx, dy);

		xctl = m_controlArray[XWIN1_EDITBOX_SEARCH];
		assert(nullptr != xctl);
		int L = gap;
		int R = dx - (gap >> 1);
		int T = dy;
		int B = dy + sh;
		xctl->setPosition(L, T, R, B);
	}
};

#endif  /* __WOCHAT_WINDOWS1_H__ */

