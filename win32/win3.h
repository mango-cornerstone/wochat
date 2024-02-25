#ifndef __DUI_WINDOW3_H__
#define __DUI_WINDOW3_H__

#include "dui/dui_win.h"

enum Win3Mode
{
	WIN3_MODE_SHOWGROUP = 0,
	WIN3_MODE_SHOWFRIEND,
	WIN3_MODE_SHOWSETTING
};

class XWindow3 : public XWindowT <XWindow3>
{
	enum
	{
		GAP_TOP3 = 40,
		GAP_BOTTOM3 = 10,
		GAP_LEFT3 = 6,
		GAP_RIGHT3 = 10
	};
private:
	Win3Mode m_mode = WIN3_MODE_SHOWGROUP;

public:
	XWindow3()
	{
		m_backgroundColor = 0xFFF5F5F5;
		m_property |= (DUI_PROP_MOVEWIN | DUI_PROP_HANDLETEXT);
		m_message = WM_XWINDOWS03;
	}
	~XWindow3() {}

public:
	Win3Mode GetMode() { return m_mode; }

	void SetMode(Win3Mode mode)
	{
		XControl* xctl;
		m_mode = mode;
		switch (m_mode)
		{
		case WIN3_MODE_SHOWGROUP:
			m_backgroundColor = 0xFFF5F5F5;

			xctl = m_controlArray[XWIN3_BUTTON_DOT];
			assert(nullptr != xctl);
			xctl->HideMe(false);

			xctl = m_controlArray[XWIN3_LABEL_PUBLICKEY];
			assert(nullptr != xctl);
			xctl->HideMe(false);

			xctl = m_controlArray[XWIN3_LABEL_NAME];
			assert(nullptr != xctl);
			xctl->HideMe(false);
			InvalidateScreen();
			break;
		case WIN3_MODE_SHOWFRIEND:
		case WIN3_MODE_SHOWSETTING:
			m_backgroundColor = 0xFFFFFFFF;
			xctl = m_controlArray[XWIN3_BUTTON_DOT];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN3_LABEL_PUBLICKEY];
			assert(nullptr != xctl);
			xctl->HideMe();

			xctl = m_controlArray[XWIN3_LABEL_NAME];
			assert(nullptr != xctl);
			xctl->HideMe();
			InvalidateScreen();
			break;
		default:
			break;
		}
	}

	void UpdateControlPosition()
	{
		XControl* xctl;
		int dx, dy, sw, sh, gap = 10; // pixel
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		if (w > 0 && h > 0)
		{
			xctl = m_controlArray[XWIN3_BUTTON_DOT];
			assert(nullptr != xctl);
			sw = xctl->getWidth();
			sh = xctl->getHeight();

			if (w > (sw + GAP_RIGHT3) && h > (sh + GAP_BOTTOM3))
			{
				dx = w - sw - GAP_RIGHT3;
				dy = h - sh - GAP_BOTTOM3;
				xctl->setPosition(dx, dy);
			}

			xctl = m_controlArray[XWIN3_LABEL_PUBLICKEY];
			assert(nullptr != xctl);
			sh = xctl->getHeight();
			assert(h > sh);
			dx = GAP_LEFT3;
			dy = (h - sh) - 3;
			xctl->setPosition(dx, dy);

			xctl = m_controlArray[XWIN3_LABEL_NAME];
			assert(nullptr != xctl);
			sh = xctl->getHeight();
			assert(dy > sh);
			dx = GAP_LEFT3;
			dy = dy - sh - 3;
			xctl->setPosition(dx, dy);
		}
	}

private:
	void InitBitmap()
	{
		U8 id;
		XBitmap* bmp;

		int w = 19;
		int h = 7;
		id = XWIN3_BITMAP_DOTN; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpDotN; bmp->w = w; bmp->h = h;
		id = XWIN3_BITMAP_DOTH; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpDotH; bmp->w = w; bmp->h = h;
		id = XWIN3_BITMAP_DOTP; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpDotP; bmp->w = w; bmp->h = h;
		id = XWIN3_BITMAP_DOTA; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpDotH; bmp->w = w; bmp->h = h;
	}

public:
	void InitControl()
	{
		int lineheight;
		U8 id, * mem;
		U32 objSize;
		wchar_t name[] = { 0x0041,0x0049,0 };

		assert(nullptr != m_pool);

		InitBitmap(); // inital all bitmap resource

		id = XWIN3_BUTTON_DOT;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XBitmap* bmpN;
			XBitmap* bmpH;
			XBitmap* bmpP;
			XBitmap* bmpA;
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W3DOT");
			bmpN = &m_bitmapArray[XWIN3_BITMAP_DOTN];
			bmpH = &m_bitmapArray[XWIN3_BITMAP_DOTH];
			bmpP = &m_bitmapArray[XWIN3_BITMAP_DOTP];
			bmpA = &m_bitmapArray[XWIN3_BITMAP_DOTA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN3_LABEL_PUBLICKEY;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_TITLE);
			lb->Init(id, "W3TITLE", g_pDWriteFactory, pTextFormat);
			lb->setText((wchar_t*)name, 2);
			m_controlArray[id] = lb;
		}

		id = XWIN3_LABEL_NAME;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_GROUPNAME);
			assert(pTextFormat);
			lb->Init(id, "W3NAME", g_pDWriteFactory, pTextFormat);
			lb->setText((wchar_t*)name, 2);
			m_controlArray[id] = lb;
		}

		m_maxControl = 3;
	}

	U32 SetChatGroup(XChatGroup* cg)
	{
		XLabel* lb;
		wchar_t hexPK[67];

		assert(cg);

		lb = (XLabel*)m_controlArray[XWIN3_LABEL_NAME];
		lb->setText(cg->name, cg->nameLen);

		wt_Raw2HexStringW(cg->pubkey, 33, hexPK, nullptr);
		lb = (XLabel*)m_controlArray[XWIN3_LABEL_PUBLICKEY];
		lb->setText(hexPK, 66);
		UpdateControlPosition();
		InvalidateScreen();

		return WT_OK;
	}
};

#endif  /* __DUI_WINDOW3_H__ */

