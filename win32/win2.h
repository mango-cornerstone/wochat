#ifndef __DUI_WINDOW2_H__
#define __DUI_WINDOW2_H__

#include "dui/dui_win.h"
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

	int		m_lineHeight0 = 0;
	int		m_lineHeight1 = 0;

	XChatGroup* m_chatgroupRoot = nullptr;
	XChatGroup* m_chatgroupSelected = nullptr;
	XChatGroup* m_chatgroupSelectPrev = nullptr;
	XChatGroup* m_chatgroupHovered = nullptr;

	void InitBitmap()
	{
	}

public:
	XWindow2()
	{
		m_backgroundColor = DEFAULT_COLOR;
		m_property |= (DUI_PROP_HASVSCROLL | DUI_PROP_HANDLEWHEEL | DUI_PROP_LARGEMEMPOOL | DUI_PROP_HANDLETEXT);
		m_message = WM_XWINDOWS02;
	}
	~XWindow2() {}

	XChatGroup* GetSelectedChatGroup()
	{
		return m_chatgroupSelected;
	}

	int LoadChatGroupList()
	{
		int i, total = 0;
		assert(nullptr == m_chatgroupRoot);
		assert(nullptr != m_pool);

		m_sizeAll.cy = 0;
		m_sizeLine.cy = 0;

		m_chatgroupRoot = (XChatGroup*)palloc0(m_pool, sizeof(XChatGroup));
		if (nullptr != m_chatgroupRoot)
		{
			XChatGroup* p;
			XChatGroup* q;

			p = m_chatgroupRoot;
			p->mempool = mempool_create("CHATGRUP", 0, DUI_ALLOCSET_SMALL_INITSIZE, DUI_ALLOCSET_SMALL_MAXSIZE);
			if (nullptr != p->mempool)
			{
				m_chatgroupSelected = m_chatgroupRoot;
				for (int i = 0; i < 33; i++)
				{
					p->pubkey[i] = g_PK[i];
				}

				p->icon = (U32*)xbmpGroup;
				p->name = (wchar_t*)g_PKTextW;
				p->w = ICON_HEIGHT;
				p->h = ICON_HEIGHT;
				p->height = ITEM_HEIGHT;
				p->width = 0;
				p->lastmsg = nullptr;
				p->tsText = nullptr;
				p->next = nullptr;
			}
			else
			{
				pfree(m_chatgroupRoot);
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

		return ret;
	}

	int Do_DUI_DESTROY(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		XChatGroup* p = m_chatgroupRoot;

		while (nullptr != p)
		{
			assert(nullptr != p->mempool);
			mempool_destroy(p->mempool);
			p->mempool = nullptr;
			p = p->next;
		}

		return 0;
	}


public:
	int Do_DUI_PAINT(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
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
};

#endif  /* __DUI_WINDOW2_H__ */

