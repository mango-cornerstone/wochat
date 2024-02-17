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
		WIN4_GAP_MESSAGE = 34
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
			for (int i = 0; i < 33; i++)
				pk[i] = m_chatGroup->pubkey[i];
			r = 0;
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

	int Do_DUI_CREATE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		// determine the line height
		IDWriteTextLayout* pTextLayout = nullptr;
		IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
		g_pDWriteFactory->CreateTextLayout(
			L"X",
			1,
			pTextFormat,
			1000.f,
			static_cast<FLOAT>(1),
			(IDWriteTextLayout**)(&pTextLayout));

		if (pTextLayout)
		{
			DWRITE_TEXT_METRICS tm;
			pTextLayout->GetMetrics(&tm);
			assert(tm.lineCount == 1);
			m_sizeLine.cy = static_cast<int>(tm.height) + 1;
		}
		else
		{
			m_sizeLine.cy = 16;
		}
		SafeRelease(&pTextLayout);

		return 0; 
	}

	int UpdateReceivedMessage(MessageTask* mt)
	{
		U32 i = 0;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		XChatMessage* p = nullptr;
		XChatMessage* q = nullptr;

		if (nullptr == m_chatGroup)
			return 0;

		for (i = 0; i < 33; i++) // check the public key is the same or not
		{
			if (m_chatGroup->pubkey[i] != mt->pubkey[i])
				break;
		}
		if (i != 33) // if the public key is not the same, something is wrong
			return 0;

		p = (XChatMessage*)wt_palloc0(m_chatGroup->mempool, sizeof(XChatMessage));
		if (nullptr == p)
			return 0;

		p->message = (wchar_t*)wt_palloc(m_chatGroup->mempool, sizeof(wchar_t) * (mt->msgLen));
		if (nullptr == p->message)
		{
			wt_pfree(p);
			return 0;
		}

		p->msgLen = mt->msgLen;
		for (i = 0; i < mt->msgLen; i++) // copy the text message
			p->message[i] = mt->message[i];

		{  // determine the height of the text layout
			IDWriteTextLayout* pTextLayout = nullptr;
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			FLOAT Wf = static_cast<FLOAT>(w * 2 / 3); // the text width is half of the window
			g_pDWriteFactory->CreateTextLayout(
				p->message,
				p->msgLen,
				pTextFormat,
				Wf,
				static_cast<FLOAT>(1),
				(IDWriteTextLayout**)(&pTextLayout));

			if (pTextLayout)
			{
				pTextLayout->GetMetrics(&(p->tm));
				p->height = static_cast<int>(p->tm.height) + 1 + WIN4_GAP_MESSAGE;
				if (p->tm.lineCount > 1) // more than 1 line
					p->width = (w * 2 / 3);
				else
					p->width = static_cast<int>(p->tm.width) + 1;
				p->width = static_cast<int>(p->tm.width) + 1;

			}
			else
			{
				wt_pfree(p->message);
				wt_pfree(p);
				return 0;
			}
			SafeRelease(&pTextLayout);
		}

		p->state = XMESSAGE_FROM_SHE;
#if _DEBUG
		p->icon = (U32*)xbmpHeadGirl;
#else
		p->icon = (U32*)xbmpHeadMe;
#endif
		p->w = p->h = 34;
		p->next = p->prev = nullptr;

		if (nullptr == m_chatGroup->headMessage)
			m_chatGroup->headMessage = p;
		if (nullptr == m_chatGroup->tailMessage)
			m_chatGroup->tailMessage = p;
		else  // put this message on the tail of this double link
		{
			m_chatGroup->tailMessage->next = p;
			p->prev = m_chatGroup->tailMessage;
			m_chatGroup->tailMessage = p;
		}

		if (p->height > 0)
		{
			m_sizeAll.cy += p->height;
			m_ptOffset.y = (m_sizeAll.cy > h) ? (m_sizeAll.cy - h) : 0; // show the last message on the bottom
		}

		InterlockedIncrement(&(mt->state)); // ok, we have successfully processed this message task

		InvalidateScreen();

		return 1;
	}

	int UpdateMyMessage(MessageTask* mt)
	{
		int r;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		XChatMessage* p = nullptr;
		XChatMessage* q = nullptr;

		if (nullptr == m_chatGroup || nullptr == mt)
			return 0;

		for (r = 0; r < 33; r++) // check the public key is the same or not
		{
			if (m_chatGroup->pubkey[r] != mt->pubkey[r])
				break;
		}
		if (r != 33) // if the public key is not the same, something is wrong
			return 0;

		r = 0;

		if ('T' == mt->type)
		{
#if 0
			p = (XChatMessage*)wt_palloc0(m_chatGroup->mempool, sizeof(XChatMessage));
			if (nullptr == p)
				return 0;

			p->message = (wchar_t*)wt_palloc(m_chatGroup->mempool, mt->msgLen);
			if (nullptr == p->message)
			{
				wt_pfree(p);
				return 0;
			}

			p->msgLen = mt->msgLen;
			for (U16 i = 0; i < mt->msgLen; i++) // copy the text message
				p->message[i] = mt->message[i];

			{  // determine the height of the text layout
				IDWriteTextLayout* pTextLayout = nullptr;
				IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
				FLOAT Wf = static_cast<FLOAT>(w * 2 / 3); // the text width is half of the window
				g_pDWriteFactory->CreateTextLayout(
					p->message,
					p->msgLen,
					pTextFormat,
					Wf,
					static_cast<FLOAT>(1),
					(IDWriteTextLayout**)(&pTextLayout));

				if (pTextLayout)
				{
					pTextLayout->GetMetrics(&(p->tm));
					p->height = static_cast<int>(p->tm.height) + 1 + WIN4_GAP_MESSAGE;
					p->width = static_cast<int>(p->tm.width) + 1;
				}
				else
				{
					wt_pfree(p->message);
					wt_pfree(p);
					return 0;
				}
				SafeRelease(&pTextLayout);
			}

			p->state = XMESSAGE_FROM_ME;
#if _DEBUG
			p->icon = (U32*)xbmpHeadMe;
#else
			p->icon = (U32*)xbmpHeadGirl;
#endif
			p->w = p->h = 34;
			p->next = p->prev = nullptr;

			if (nullptr == m_chatGroup->headMessage)
				m_chatGroup->headMessage = p;
			if (nullptr == m_chatGroup->tailMessage)
				m_chatGroup->tailMessage = p;
			else  // put this message on the tail of this double link
			{
				m_chatGroup->tailMessage->next = p;
				p->prev = m_chatGroup->tailMessage;
				m_chatGroup->tailMessage = p;
			}

			if (p->height > 0)
			{
				m_sizeAll.cy += p->height;
				m_ptOffset.y = (m_sizeAll.cy > h) ? (m_sizeAll.cy - h) : 0; // show the last message on the bottom
			}

			InterlockedIncrement(&(mt->state)); // ok, we have successfully processed this message task

			InvalidateScreen();
#endif
		}

		return r;
	}

	int Do_DUI_PAINT(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		// U32 color = 0xFF6AEA9E;
		int x, y, dx, dy, W, H;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;

		if (m_chatGroup)
		{
			int pos = XWIN4_OFFSET;
			XChatMessage* p = m_chatGroup->headMessage;
			while (nullptr != p)
			{
				H = p->height;
				if (pos + H > m_ptOffset.y && pos < m_ptOffset.y + h) // this node is in the visible area, we need to draw it
				{
					dy = pos - m_ptOffset.y;
					x = (XMESSAGE_FROM_ME & p->state)? (w - m_scrollWidth - p->w - XWIN4_OFFSET) : XWIN4_OFFSET;
					DUI_ScreenDrawRectRound(m_screen, w, h, (U32*)p->icon, p->w, p->h, x, dy, m_backgroundColor, m_backgroundColor);
				}
				pos += H;
				if (pos >= m_ptOffset.y + h) // out of the scope of the drawing area
					break;
				p = p->next;
			}
		}
		return 0;
	}

	int DoDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{ 
#if 0
		U32 color;
		bool isMe;
		int x, y, dx, dy, W, H, pos;
		XChatMessage* p;
		IDWriteTextLayout* pTextLayout = nullptr;
		IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
		ID2D1HwndRenderTarget* pD2DRenderTarget = static_cast<ID2D1HwndRenderTarget*>(surface);
		ID2D1SolidColorBrush* pTextBrush = static_cast<ID2D1SolidColorBrush*>(brushText);
		ID2D1SolidColorBrush* pBkgBrush0 = static_cast<ID2D1SolidColorBrush*>(brushBkg0);
		ID2D1SolidColorBrush* pBkgBrush1 = static_cast<ID2D1SolidColorBrush*>(brushBkg1);

		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		FLOAT Wf = static_cast<FLOAT>(w * 2 / 3);

		if (m_chatGroup)
		{
			D2D1_POINT_2F orgin;
			D2D1_RECT_F bkgarea;
			int pos = XWIN4_OFFSET;
			p = m_chatGroup->headMessage;
			while (nullptr != p)
			{
				isMe = (XMESSAGE_FROM_ME & p->state);
				H = p->height;
				if (pos + H > m_ptOffset.y && pos < m_ptOffset.y + h) // we are in the visible area
				{
					dy = pos - m_ptOffset.y;
					dx = (XWIN4_OFFSET<<2) + 34;

					if (isMe) // this message is from me
					{
						dx = w - p->width - 34 - m_scrollWidth - XWIN4_OFFSET - XWIN4_OFFSET;
					}

					bkgarea.left = static_cast<FLOAT>(m_area.left + dx) - XWIN4_OFFSET - XWIN4_OFFSET;
					bkgarea.top = static_cast<FLOAT>(m_area.top + dy);
					bkgarea.right = bkgarea.left + static_cast<FLOAT>(p->width) + XWIN4_OFFSET + XWIN4_OFFSET;
					bkgarea.bottom = bkgarea.top + static_cast<FLOAT>(p->height - WIN4_GAP_MESSAGE) + XWIN4_OFFSET + XWIN4_OFFSET;
					if(isMe)
						pD2DRenderTarget->FillRectangle(bkgarea, pBkgBrush0);
					else
						pD2DRenderTarget->FillRectangle(bkgarea, pBkgBrush1);

					g_pDWriteFactory->CreateTextLayout(
						p->message,
						p->msgLen,
						pTextFormat,
						Wf,
						static_cast<FLOAT>(1),
						(IDWriteTextLayout**)(&pTextLayout));

					if (pTextLayout)
					{
						orgin.x = static_cast<FLOAT>(dx + m_area.left - XWIN4_OFFSET);
						orgin.y = static_cast<FLOAT>(dy + m_area.top + XWIN4_OFFSET);
						pD2DRenderTarget->DrawTextLayout(orgin, pTextLayout, pTextBrush);
					}
					SafeRelease(&pTextLayout);
				}
				pos += H;
				if (pos >= m_ptOffset.y + h) // out of the scope of the drawing area
					break;
				p = p->next;
			}
		}
#endif
		return 0; 
	}
};

#endif  /* __DUI_WINDOW4_H__ */

