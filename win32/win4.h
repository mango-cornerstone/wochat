#ifndef __DUI_WINDOW4_H__
#define __DUI_WINDOW4_H__

#include "dui/dui_win.h"

#define XMESSAGE_FROM_SHE  0x0000
#define XMESSAGE_FROM_ME   0x0001

#define XWIN4_MARGIN	  32
#define XWIN4_OFFSET	  10

class XWindow4 : public XWindowT <XWindow4>
{
	enum 
	{
		WIN4_GAP_TOP4    = 40,
		WIN4_GAP_BOTTOM4 = 10,
		WIN4_GAP_LEFT4   = 10,
		WIN4_GAP_RIGHT4  = 10,
		WIN4_GAP_MESSAGE = 20
	};

	XChatGroup* m_chatGroup = nullptr;
public:
	XWindow4()
	{
		m_backgroundColor = 0xFFF5F5F5;
		m_property |= (DUI_PROP_HASVSCROLL | DUI_PROP_HANDLEWHEEL | DUI_PROP_HANDLETEXT);
		m_message = WM_XWINDOWS04;
	}
	~XWindow4() {}

public:
	int GetPublicKey(U8* pk) // public key is 33 bytes in raw
	{
		int r = 1;
		if (m_chatGroup && pk)
		{
			r = 0;
			for (int i = 0; i < 33; i++)
				pk[i] = m_chatGroup->pubkey[i];
		}
		return r;
	}

	int SetChatGroup(XChatGroup* cg)
	{
		int r = 0;
		assert(nullptr != cg);

		XChatGroup* prev = m_chatGroup;
		m_chatGroup = cg;
		if (prev != m_chatGroup)
		{
			m_status |= DUI_STATUS_NEEDRAW;
			InvalidateDUIWindow();
			r++;
		}
		return r;
	}

	int UpdateMyMessage(MessageTask* mt)
	{
		int i;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		XChatMessage* p = nullptr;
		XChatMessage* q = nullptr;

		if (nullptr == m_chatGroup)
			return 1;

		for (i = 0; i < 33; i++)
		{
			if (m_chatGroup->pubkey[i] != mt->pubkey[i])
				break;
		}

		if (i != 33)
			return 1;

		p = (XChatMessage*)palloc0(m_chatGroup->mempool, sizeof(XChatMessage));
		if (nullptr == p)
			return 1;

		p->message = (wchar_t*)palloc(m_chatGroup->mempool, sizeof(wchar_t)*(mt->msgLen));
		if (nullptr == p->message)
		{
			free(p);
			return 1;
		}
		p->msgLen = mt->msgLen;
		for (U16 i = 0; i < mt->msgLen; i++) // copy the text message
			p->message[i] = mt->message[i];
		
		p->state = XMESSAGE_FROM_ME;
		p->icon = (U32*)xbmpHeadMe;
		p->w = p->h = 34;

		p->next = p->prev = nullptr;

		if (nullptr == m_chatGroup->headMessage)
			m_chatGroup->headMessage = p;
		if (nullptr == m_chatGroup->tailMessage)
			m_chatGroup->tailMessage = p;
		else
		{
			m_chatGroup->tailMessage->next = p;
			p->prev = m_chatGroup->tailMessage;
			m_chatGroup->tailMessage = p;
		}

		{  // determine the height
			IDWriteTextLayout* pTextLayout = nullptr;
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			FLOAT Wf = static_cast<FLOAT>((w << 1) / 3);
			g_pDWriteFactory->CreateTextLayout(
				p->message,
				p->msgLen,
				pTextFormat,
				Wf,
				static_cast<FLOAT>(1),
				(IDWriteTextLayout**)(&pTextLayout));

			if (pTextLayout)
			{
				DWRITE_TEXT_METRICS tm;
				pTextLayout->GetMetrics(&tm);
				p->height = static_cast<int>(tm.height) + WIN4_GAP_MESSAGE;
				m_sizeAll.cy += p->height;
			}
			SafeRelease(&pTextLayout);
		}
		InterlockedIncrement(&(mt->state)); // ok, we have successfully processed this message task

		InvalidateScreen();
		return 0;
	}

	int Do_DUI_PAINT(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		U32 color;
		int x, y, dx, dy, W, H, pos;
		XChatMessage* p;

		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;

		if (nullptr == m_chatGroup)
			return 0;

		pos = WIN4_GAP_MESSAGE;
		p = m_chatGroup->headMessage;
		while (nullptr != p)
		{
			H = p->height;
			if (pos + H > m_ptOffset.y && pos < m_ptOffset.y + h)
			{
				dy = pos - m_ptOffset.y;
				x = w - m_scrollWidth - p->w - XWIN4_OFFSET;
				if (XMESSAGE_FROM_ME & p->state) // me
				{
					color = 0xFF6AEA9E;
				}
				else
				{
					color = 0xFFFFFFFF;
					x = XWIN4_OFFSET;
				}
				DUI_ScreenDrawRectRound(m_screen, w, h, (U32*)p->icon, p->w, p->h, x, dy, m_backgroundColor, m_backgroundColor);
#if 0
				if (p->state % 2)
					ScreenDrawRect(m_screen, w, h, (U32*)littleArrowMe, 4, 8, x + p->w + 4, dy + 13);
#endif
				//ScreenFillRectRound(m_screen, w, h, color, tdi->right - tdi->left + 4, p->height - GAP_MESSAGE, tdi->left, dy, m_backgroundColor, m_backgroundColor);
			}
			pos += H;
			if (pos >= m_ptOffset.y + h) // out of the scope of the drawing area
				break;
			p = p->next;
		}

		return 0;
	}

#if 0
	int UpdateChatHistory(wchar_t* msgText, U16 len, U8 msgtype = 0)
	{
		int W, H, r;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		XChatMessage* p = nullptr;
		XChatMessage* q = nullptr;

		if (nullptr == m_chatGroup)
			return 1;

		p = (XChatMessage*)palloc0(m_chatGroup->mempool, sizeof(XChatMessage));
		if (nullptr == p)
			return 1;

		p->message = (wchar_t*)palloc(m_chatGroup->mempool, sizeof(wchar_t) * len);
		if (nullptr == p->message)
		{
			free(p);
			return 1;
		}

		p->msgLen = len;
		for (U16 i = 0; i < len; i++) // copy the text message
			p->message[i] = msgText[i];

		p->state = msgtype ? XMESSAGE_FROM_ME : XMESSAGE_FROM_SHE;
		if (XMESSAGE_FROM_ME & p->state)
			p->icon = (U32*)xbmpHeadMe;
		else
			p->icon = (U32*)xbmpHeadGirl;

		p->w = p->h = 34;
		p->id = msgtype;

		p->next = p->prev = nullptr;

		if (nullptr == m_chatGroup->headMessage)
			m_chatGroup->headMessage = p;
		if (nullptr == m_chatGroup->tailMessage)
			m_chatGroup->tailMessage = p;
		else
		{
			m_chatGroup->tailMessage->next = p;
			p->prev = m_chatGroup->tailMessage;
			m_chatGroup->tailMessage = p;
		}

		{  // determine the height
			IDWriteTextLayout* pTextLayout = nullptr;
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			FLOAT Wf = static_cast<FLOAT>((w << 1) / 3);
			g_pDWriteFactory->CreateTextLayout(
				p->message,
				p->msgLen,
				pTextFormat,
				Wf,
				static_cast<FLOAT>(1),
				(IDWriteTextLayout**)(&pTextLayout));

			if (pTextLayout)
			{
				DWRITE_TEXT_METRICS tm;
				pTextLayout->GetMetrics(&tm);
				p->height = static_cast<int>(tm.height) + WIN4_GAP_MESSAGE;
				m_sizeAll.cy += p->height;
			}
			SafeRelease(&pTextLayout);
		}

		InvalidateScreen();

		return 0;
	}
#endif
	int DoDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret) 
	{ 
		U32 color;
		int x, y, dx, dy, W, H, pos;
		XChatMessage* p;
		IDWriteTextLayout* pTextLayout = nullptr;
		IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
		ID2D1HwndRenderTarget* pD2DRenderTarget = static_cast<ID2D1HwndRenderTarget*>(surface);
		ID2D1SolidColorBrush* pTextBrush = static_cast<ID2D1SolidColorBrush*>(brushText);

		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		FLOAT Wf = static_cast<FLOAT>((w << 1) / 3);

		if (nullptr == m_chatGroup)
			return 0;

		pos = WIN4_GAP_MESSAGE;
		p = m_chatGroup->headMessage;
		while (nullptr != p)
		{
			H = p->height;
			if (pos + H > m_ptOffset.y && pos < m_ptOffset.y + h) // we are in the visible area
			{
				dy = pos - m_ptOffset.y;
				x = m_area.left + 100;
				if (XMESSAGE_FROM_SHE & p->state) // me
				{
					x = XWIN4_OFFSET;
				}
				g_pDWriteFactory->CreateTextLayout(
					p->message,
					p->msgLen,
					pTextFormat,
					Wf,
					static_cast<FLOAT>(1),
					(IDWriteTextLayout**)(&pTextLayout));

				if (pTextLayout)
				{
					DWRITE_TEXT_METRICS tm;
					pTextLayout->GetMetrics(&tm);
					D2D1_POINT_2F orgin;
					orgin.x = static_cast<FLOAT>(x);
					orgin.y = static_cast<FLOAT>(dy + m_area.top);
					pD2DRenderTarget->DrawTextLayout(orgin, pTextLayout, pTextBrush);
				}
				SafeRelease(&pTextLayout);
			}
			pos += H;
			if (pos >= m_ptOffset.y + h) // out of the scope of the drawing area
				break;
			p = p->next;
		}

		return 0; 
	}
};

#endif  /* __DUI_WINDOW4_H__ */

