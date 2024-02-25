#ifndef __DUI_WINDOW2_H__
#define __DUI_WINDOW2_H__

#include "dui/dui_win.h"

static const wchar_t AIGroupName[] = { 0x0041,0x0049,0x804a,0x5929,0x673a,0x5668,0x4eba,0 };

static const wchar_t Win2GeneralSetting[] = { 0x901a,0x7528,0x8bbe,0x7f6e,0 };
static const wchar_t Win2NetworkSetting[] = { 0x7f51,0x7edc,0x8bbe,0x7f6e,0 };
static const wchar_t Win2AboutSetting[] = { 0x5173,0x4e8e,0x672c,0x8f6f,0x4ef6,0 };

static const wchar_t Win2LastMSG[] = {
	0x66fe, 0x7ecf, 0x6709, 0x4e00, 0x4efd, 0x771f, 0x8bda, 0x7684,
	0x7231, 0x60c5, 0x6446, 0x5728, 0x6211, 0x7684, 0x9762, 0x524d, 0 };

enum Win2Mode
{
	WIN2_MODE_SHOWTALK = 0,
	WIN2_MODE_SHOWFRIEND,
	WIN2_MODE_SHOWSETTING
};

class XWindow2 : public XWindowT <XWindow2>
{
private:
	enum {
		SELECTED_COLOR = 0xFFC6C6C6,
		HOVERED_COLOR = 0xFFDADADA,
		DEFAULT_COLOR = 0xFFE5E5E5
	};

	enum {
		ITEM_HEIGHT = 64,
		ICON_HEIGHT = 40,
		ITEM_MARGIN = ((ITEM_HEIGHT - ICON_HEIGHT) >> 1)
	};

	Win2Mode m_mode = WIN2_MODE_SHOWTALK;

	int		m_lineHeight0 = 0;
	int		m_lineHeight1 = 0;

	XChatGroup* m_chatgroupRoot = nullptr;
	XChatGroup* m_chatgroupSelected = nullptr;
	XChatGroup* m_chatgroupSelectPrev = nullptr;
	XChatGroup* m_chatgroupHovered = nullptr;

	XSetting* m_settingRoot = nullptr;
	XSetting* m_settingSelected = nullptr;
	XSetting* m_settingSelectPrev = nullptr;
	XSetting* m_settingHovered = nullptr;

	void InitBitmap()
	{
	}

public:
	XWindow2()
	{
		m_backgroundColor = DEFAULT_COLOR;
		m_property |= (DUI_PROP_HASVSCROLL | DUI_PROP_HANDLEWHEEL | DUI_PROP_HANDLETEXT);
		m_message = WM_XWINDOWS02;
	}
	~XWindow2() {}

	Win2Mode GetMode() { return m_mode; }

	void SetMode(Win2Mode mode)
	{
		m_mode = mode;
		switch (m_mode)
		{
		case WIN2_MODE_SHOWTALK:
		case WIN2_MODE_SHOWFRIEND:
		case WIN2_MODE_SHOWSETTING:
			InvalidateScreen();
			break;
		default:
			break;
		}
	}

	XChatGroup* GetSelectedChatGroup()
	{
		return m_chatgroupSelected;
	}

	U32 InitSettings()
	{
		assert(nullptr == m_settingRoot);
		assert(nullptr != m_pool);

		m_settingRoot = (XSetting*)wt_palloc0(m_pool, sizeof(XSetting));
		if (m_settingRoot)
		{
			XSetting* p;
			XSetting* q;

			p = m_settingRoot;
			p->name = (wchar_t*)Win2GeneralSetting;
			p->nameLen = wcslen(p->name);

			q = (XSetting*)wt_palloc0(m_pool, sizeof(XSetting));
			assert(q);
			q->name = (wchar_t*)Win2NetworkSetting;
			q->nameLen = wcslen(p->name);
			p->next = q;
			p = q;

			q = (XSetting*)wt_palloc0(m_pool, sizeof(XSetting));
			assert(q);
			q->name = (wchar_t*)Win2AboutSetting;
			q->nameLen = wcslen(p->name);
			p->next = q;
		}

		return WT_OK;
	}

	int LoadChatGroupList()
	{
		int i, total = 0;
		assert(nullptr == m_chatgroupRoot);
		assert(nullptr != m_pool);

		m_sizeAll.cy = 0;
		m_sizeLine.cy = 0;

		m_chatgroupRoot = (XChatGroup*)wt_palloc0(m_pool, sizeof(XChatGroup));
		if (nullptr != m_chatgroupRoot)
		{
			XChatGroup* p;
			XChatGroup* q;
			p = m_chatgroupRoot;
			p->mempool = wt_mempool_create("CHATGROUP", 0, DUI_ALLOCSET_DEFAULT_INITSIZE, DUI_ALLOCSET_DEFAULT_MAXSIZE);
			if (nullptr != p->mempool)
			{
				m_chatgroupSelected = m_chatgroupRoot;
				for (int i = 0; i < 33; i++)
				{
					p->pubkey[i] = g_pkAI[i];
				}

				p->icon = (U32*)xbmpChatGPT;
				p->name = (wchar_t*)AIGroupName;
				p->nameLen = 7;
				p->lastmsg = (wchar_t*)Win2LastMSG;
				p->lastmsgLen = 16;

				p->w = ICON_HEIGHT;
				p->h = ICON_HEIGHT;
				p->height = ITEM_HEIGHT;
				p->width = 0;
				p->tsText = nullptr;
				p->next = nullptr;
			}
			else
			{
				wt_pfree(m_chatgroupRoot);
				m_chatgroupRoot = nullptr;
				return 1;
			}
		}

		m_sizeAll.cy = ITEM_HEIGHT;
		m_sizeLine.cy = ITEM_HEIGHT / 3;
		return 0;
	}

	int Do_DUI_CREATE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int ret = 0;
		ret = LoadChatGroupList();

		InitSettings();

		return ret;
	}

	int Do_DUI_DESTROY(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		XChatGroup* p = m_chatgroupRoot;

		while (nullptr != p)
		{
			assert(nullptr != p->mempool);
			wt_mempool_destroy(p->mempool);
			p->mempool = nullptr;
			p = p->next;
		}

		return 0;
	}


public:
	int DoTalkDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{
		HRESULT hr;
		IDWriteTextFormat* pTextFormat0 = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
		IDWriteTextFormat* pTextFormat1 = GetTextFormat(WT_TEXTFORMAT_OTHER3);
		ID2D1HwndRenderTarget* pD2DRenderTarget = static_cast<ID2D1HwndRenderTarget*>(surface);
		ID2D1SolidColorBrush* pTextBrush = static_cast<ID2D1SolidColorBrush*>(brushText);
		ID2D1SolidColorBrush* pBkgBrush0 = static_cast<ID2D1SolidColorBrush*>(brushBkg0);
		ID2D1SolidColorBrush* pBkgBrush1 = static_cast<ID2D1SolidColorBrush*>(brushBkg1);
		IDWriteTextLayout* pTextLayout0 = nullptr;
		IDWriteTextLayout* pTextLayout1 = nullptr;
		IDWriteTextLayout* pTextLayout2 = nullptr;

		assert(pTextFormat0);
		assert(pTextFormat1);

		int dx, dy, pos;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		int W = DUI_ALIGN_DEFAULT32(w - ITEM_MARGIN - ITEM_MARGIN - ICON_HEIGHT - margin - 4);
		int H = ITEM_HEIGHT;
		D2D1_POINT_2F orgin;
		dx = ITEM_MARGIN;

		XChatGroup* p = m_chatgroupRoot;
		pos = 0;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				dx = ITEM_MARGIN + ICON_HEIGHT + ITEM_MARGIN;
				dy = pos - m_ptOffset.y + ITEM_MARGIN;
				hr = g_pDWriteFactory->CreateTextLayout(p->name, p->nameLen, pTextFormat0, static_cast<FLOAT>(W), static_cast<FLOAT>(1), &pTextLayout0);
				if (S_OK == hr)
				{
					orgin.x = static_cast<FLOAT>(dx + m_area.left);
					orgin.y = static_cast<FLOAT>(dy + m_area.top);
					pD2DRenderTarget->DrawTextLayout(orgin, pTextLayout0, pTextBrush);
				}
				SafeRelease(&pTextLayout0);
				hr = g_pDWriteFactory->CreateTextLayout(p->lastmsg, p->lastmsgLen, pTextFormat1, static_cast<FLOAT>(W), static_cast<FLOAT>(1), &pTextLayout1);
				if (S_OK == hr)
				{
					orgin.x = static_cast<FLOAT>(dx + m_area.left);
					orgin.y = static_cast<FLOAT>(dy + m_area.top + ITEM_MARGIN + ITEM_MARGIN);
					pD2DRenderTarget->DrawTextLayout(orgin, pTextLayout1, pTextBrush);
				}
				SafeRelease(&pTextLayout1);
			}
			p = p->next;
			pos += ITEM_HEIGHT;
			if (pos >= (m_ptOffset.y + h))
				break;
		}

		return 0;
	}

	int DoSettingDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{
		HRESULT hr;
		IDWriteTextFormat* pTextFormat0 = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
		IDWriteTextFormat* pTextFormat1 = GetTextFormat(WT_TEXTFORMAT_OTHER3);
		ID2D1HwndRenderTarget* pD2DRenderTarget = static_cast<ID2D1HwndRenderTarget*>(surface);
		ID2D1SolidColorBrush* pTextBrush = static_cast<ID2D1SolidColorBrush*>(brushText);
		ID2D1SolidColorBrush* pBkgBrush0 = static_cast<ID2D1SolidColorBrush*>(brushBkg0);
		ID2D1SolidColorBrush* pBkgBrush1 = static_cast<ID2D1SolidColorBrush*>(brushBkg1);
		IDWriteTextLayout* pTextLayout0 = nullptr;
		IDWriteTextLayout* pTextLayout1 = nullptr;
		IDWriteTextLayout* pTextLayout2 = nullptr;

		assert(pTextFormat0);
		assert(pTextFormat1);

		int dx, dy, pos;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		int W = DUI_ALIGN_DEFAULT32(w - margin - 4);
		int H = ITEM_HEIGHT;
		D2D1_POINT_2F orgin;
		dx = ITEM_MARGIN;

		XSetting* p = m_settingRoot;
		pos = 0;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				dy = pos - m_ptOffset.y + ITEM_MARGIN;
				hr = g_pDWriteFactory->CreateTextLayout(p->name, p->nameLen, pTextFormat0, static_cast<FLOAT>(W), static_cast<FLOAT>(1), &pTextLayout0);
				if (S_OK == hr)
				{
					orgin.x = static_cast<FLOAT>(dx + m_area.left);
					orgin.y = static_cast<FLOAT>(dy + m_area.top);
					pD2DRenderTarget->DrawTextLayout(orgin, pTextLayout0, pTextBrush);
				}
				SafeRelease(&pTextLayout0);
			}
			p = p->next;
			pos += ITEM_HEIGHT;
			if (pos >= (m_ptOffset.y + h))
				break;
		}
		return 0;
	}

	int DoFriendDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{
		return 0;
	}

	int DoDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{
		int r = 0;
		switch (m_mode)
		{
		case WIN2_MODE_SHOWTALK:
			r = DoTalkDrawText(surface, brushText, brushSelText, brushCaret, brushBkg0, brushBkg1);
			break;
		case WIN2_MODE_SHOWFRIEND:
			r = DoFriendDrawText(surface, brushText, brushSelText, brushCaret, brushBkg0, brushBkg1);
			break;
		case WIN2_MODE_SHOWSETTING:
			r = DoSettingDrawText(surface, brushText, brushSelText, brushCaret, brushBkg0, brushBkg1);
			break;
		default:
			assert(0);
			break;
		}
		return r;
	}

	int TalkPaint(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		U32 color;
		int dx, dy, pos;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		int W = DUI_ALIGN_DEFAULT32(w - ITEM_MARGIN - ITEM_MARGIN - ICON_HEIGHT - margin - 4);
		int H = ITEM_HEIGHT;

		dx = ITEM_MARGIN;
		XChatGroup* p = m_chatgroupRoot;
		pos = 0;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				if (p == m_chatgroupSelected)
				{
					color = SELECTED_COLOR;
				}
				else if (p == m_chatgroupHovered)
				{
					color = HOVERED_COLOR;
				}
				else
				{
					color = DEFAULT_COLOR;
				}
				dy = pos - m_ptOffset.y;
				DUI_ScreenFillRect(m_screen, w, h, color, w - margin, ITEM_HEIGHT, 0, dy);
				DUI_ScreenDrawRectRound(m_screen, w, h, p->icon, p->w, p->h, dx, dy + ITEM_MARGIN, color, color);
			}
			p = p->next;
			pos += ITEM_HEIGHT;
			if (pos >= (m_ptOffset.y + h))
				break;
		}
		return 0;
	}

	int FriendPaint(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = 0;

		return r;
	}

	int SettingPaint(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = 0;
		U32 color;
		int dx, dy, pos;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		int W = DUI_ALIGN_DEFAULT32(w - ITEM_MARGIN - ITEM_MARGIN - ICON_HEIGHT - margin - 4);
		int H = ITEM_HEIGHT;

		XSetting* p = m_settingRoot;
		m_sizeAll.cy = 0;
		while (p)
		{
			m_sizeAll.cy += ITEM_HEIGHT;
			p = p->next;
		}
		m_ptOffset.y = 0;
		pos = 0;
		p = m_settingRoot;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				if (p == m_settingSelected)
				{
					color = SELECTED_COLOR;
				}
				else if (p == m_settingHovered)
				{
					color = HOVERED_COLOR;
				}
				else
				{
					color = DEFAULT_COLOR;
				}
				dy = pos - m_ptOffset.y;
				DUI_ScreenFillRect(m_screen, w, h, color, w - margin, ITEM_HEIGHT, 0, dy);
			}
			p = p->next;
			pos += ITEM_HEIGHT;
			if (pos >= (m_ptOffset.y + h))
				break;
		}

		return r;
	}

	int Do_DUI_PAINT(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = 0;
		switch (m_mode)
		{
		case WIN2_MODE_SHOWTALK:
			r = TalkPaint(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_SHOWFRIEND:
			r = FriendPaint(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_SHOWSETTING:
			r = SettingPaint(uMsg, wParam, lParam, lpData);
			break;
		default:
			assert(0);
			break;
		}
		return r;
	}
};

#endif  /* __DUI_WINDOW2_H__ */

