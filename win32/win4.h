#ifndef __DUI_WINDOW4_H__
#define __DUI_WINDOW4_H__

#include "dui/dui_win.h"

#define XMESSAGE_FROM_SHE		0x0000
#define XMESSAGE_FROM_ME		0x0001
#define XMESSAGE_CONFIRMATION	0x0002

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

	WTChatGroup* m_chatGroup = nullptr;

	IDWriteTextLayout* m_cacheTL[1024];
	FLOAT m_offsetX[1024];
	FLOAT m_offsetY[1024];
	U16  m_idxTL = 0;

public:
	XWindow4()
	{
		m_backgroundColor = 0xFFF5F5F5;
		m_property |= (DUI_PROP_HASVSCROLL | DUI_PROP_HANDLEWHEEL | DUI_PROP_HANDLETEXT | DUI_PROP_LARGEMEMPOOL);
		m_message = WM_XWINDOWS04;
	}

	~XWindow4() 
	{
		for (U16 i = 0; i < m_idxTL; i++)
		{
			assert(m_cacheTL[i]);
			SafeRelease(&m_cacheTL[i]);
		}
		m_idxTL = 0;
	}

public:
	int GetPublicKey(U8* pk) // public key is 33 bytes in raw
	{
		int r = 1;
		if (m_chatGroup && pk)
		{
			WTFriend* p = m_chatGroup->people;
			assert(p);
			for (int i = 0; i < PUBLIC_KEY_SIZE; i++) pk[i] = p->pubkey[i];
			r = 0;
		}
		return r;
	}

	int ReLayoutText(int width, int height)
	{
		int r = 0;
#if 0
		if (width && m_chatGroup)
		{
			m_sizeAll.cy = 0;
			WTChatGroup* p = m_chatGroup->headMessage;
			while (nullptr != p)
			{
				{  // determine the height of the text layout
					IDWriteTextLayout* pTextLayout = nullptr;
					IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
					FLOAT Wf = static_cast<FLOAT>(width * 2 / 3); // the text width is half of the window
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
						
						m_sizeAll.cy += p->height;
					}
					SafeRelease(&pTextLayout);
				}
				p = p->next;
			}
			m_ptOffset.y = (m_sizeAll.cy > height) ? (m_sizeAll.cy - height) : 0;

			r++;
		}
#endif
		return r;
	}

	int SetChatGroup(WTChatGroup* cg)
	{
		int r = 0;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		assert(nullptr != cg);

		WTChatGroup* prev = m_chatGroup;
		m_chatGroup = cg;

		if (0 == m_chatGroup->width)
		{
			m_chatGroup->width = w;
			ReLayoutText(w, h);
		}
		else if(m_chatGroup->width != w)
		{
			m_chatGroup->width = w;
			ReLayoutText(w, h);
		}

		if (prev != m_chatGroup)
		{
			m_status |= DUI_STATUS_NEEDRAW;
			InvalidateDUIWindow();
			r++;
		}

		return r;
	}

	int UpdateMessageConfirmation(U8* pk, U8* hash)
	{
		int i = 0, r = 0;
#if 0
		XMessage* xm;

		if (nullptr == m_chatGroup)
			return 0;

		for (i = 0; i < 33; i++) // check the public key is the same or not
		{
			if (m_chatGroup->pubkey[i] != pk[i])
				break;
		}
		if (i != 33) // if the public key is not the same, something is wrong
			return 0;

		xm = (XMessage*)hash_search(g_messageHTAB, hash, HASH_FIND, NULL);
		if (xm)
		{
			WTChatGroup* p = xm->pointer;
			if (p)
			{
				p->state |= XMESSAGE_CONFIRMATION;
				r++;
			}
		}
		if (r)
			InvalidateScreen();
#endif 
		return r;
	}

	void UpdateControlPosition() // the user has changed the width of this window
	{
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		if (0 == m_chatGroup->width)
		{
			m_chatGroup->width = w;
			ReLayoutText(w, h);
		}
		else if (m_chatGroup->width != w)
		{
			m_chatGroup->width = w;
			ReLayoutText(w, h);
		}
	}

	int Do_DUI_CREATE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		U16 h = 18;
		
		GetTextFormatAndHeight(WT_TEXTFORMAT_MAINTEXT, &h);

		m_sizeLine.cy = h;

		return 0; 
	}

	int UpdateReceivedMessage(MessageTask* mt)
	{
		U32 i = 0;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		WTChatMessage* p = nullptr;
		WTChatMessage* q = nullptr;
		WTFriend* people;

		if (nullptr == m_chatGroup || nullptr == mt)
			return 0;

		people = m_chatGroup->people;
		assert(people);

		if (memcmp(people->pubkey, mt->pubkey, PUBLIC_KEY_SIZE))
			return 0;

		if ('T' == mt->type)
		{
			p = (WTChatMessage*)wt_palloc0(m_pool, sizeof(WTChatMessage));

			if (p)
			{
				p->message = (wchar_t*)wt_palloc(m_pool, mt->msgLen);
				if (p->message)
				{
					HRESULT hr = S_OK;
					IDWriteTextLayout* pTextLayout = nullptr;
					IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_MAINTEXT);

					assert(pTextFormat);

					wchar_t* text = (wchar_t*)mt->message;

					p->msgLen = mt->msgLen / sizeof(wchar_t);
					for (U16 i = 0; i < p->msgLen; i++) // copy the text message
						p->message[i] = text[i];

					// determine the height of the text layout
					FLOAT Wf = static_cast<FLOAT>(w * 2 / 3); // the text width is 2/3 of the window
					hr = g_pDWriteFactory->CreateTextLayout(p->message, p->msgLen, pTextFormat, Wf, static_cast<FLOAT>(1), &pTextLayout);

					if (S_OK == hr && pTextLayout)
					{
						DWRITE_TEXT_METRICS tm;
						pTextLayout->GetMetrics(&tm);
						p->height = static_cast<int>(tm.height) + 1 + WIN4_GAP_MESSAGE;
						p->width = static_cast<int>(tm.width) + 1;
					}
					else
					{
						wt_pfree(p->message);
						wt_pfree(p);
						return 0;
					}
					SafeRelease(&pTextLayout);
				}
				else
				{
					wt_pfree(p);
					return 0;
				}
			}

			p->state = XMESSAGE_FROM_SHE;
#if _DEBUG
			p->icon = (U32*)xbmpHeadGirl;
#else
			p->icon = (U32*)xbmpHeadMe;
#endif
			p->w = p->h = 34;
			p->next = p->prev = nullptr;

			if (nullptr == m_chatGroup->msgHead)
				m_chatGroup->msgHead = p;
			if (nullptr == m_chatGroup->msgTail)
				m_chatGroup->msgTail = p;
			else  // put this message on the tail of this double link
			{
				m_chatGroup->msgTail->next = p;
				p->prev = m_chatGroup->msgTail;
				m_chatGroup->msgTail = p;
			}
			if (p->height > 0)
			{
				m_sizeAll.cy += p->height;
				m_ptOffset.y = (m_sizeAll.cy > h) ? (m_sizeAll.cy - h) : 0; // show the last message on the bottom
			}
			InterlockedIncrement(&(mt->state)); // ok, we have successfully processed this message task
			InvalidateScreen();
		}

		return 1;
	}

	int UpdateMyMessage(MessageTask* mt)
	{
		int r;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		WTChatMessage* p = nullptr;
		WTChatMessage* q = nullptr;
		WTFriend* people;

		if (nullptr == m_chatGroup || nullptr == mt)
			return 0;

		people = m_chatGroup->people;
		assert(people);

		if (memcmp(people->pubkey, mt->pubkey, PUBLIC_KEY_SIZE))
			return 0;

		r = 0;
		if ('T' == mt->type)
		{
			p = (WTChatMessage*)wt_palloc0(m_pool, sizeof(WTChatMessage));
			if (p)
			{
				p->message = (wchar_t*)wt_palloc(m_pool, mt->msgLen);
				if (p->message)
				{
					HRESULT hr = S_OK;
					IDWriteTextLayout* pTextLayout = nullptr;
					IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_MAINTEXT);

					assert(pTextFormat);

					wchar_t* text = (wchar_t*)mt->message;

					p->msgLen = mt->msgLen / sizeof(wchar_t);
					for (U16 i = 0; i < p->msgLen; i++) // copy the text message
						p->message[i] = text[i];
					
					// determine the height of the text layout
					FLOAT Wf = static_cast<FLOAT>(w * 2 / 3); // the text width is 2/3 of the window
					hr = g_pDWriteFactory->CreateTextLayout(p->message, p->msgLen, pTextFormat, Wf, static_cast<FLOAT>(1), &pTextLayout);

					if (S_OK == hr && pTextLayout)
					{
						DWRITE_TEXT_METRICS tm;
						pTextLayout->GetMetrics(&tm);
						p->height = static_cast<int>(tm.height) + 1 + WIN4_GAP_MESSAGE;
						p->width = static_cast<int>(tm.width) + 1;
					}
					else
					{
						wt_pfree(p->message);
						wt_pfree(p);
						return 0;
					}
					SafeRelease(&pTextLayout);
				}
				else
				{
					wt_pfree(p);
					return 0;
				}
			}

			p->state = XMESSAGE_FROM_ME;
#if _DEBUG
			p->icon = (U32*)xbmpHeadMe;
#else
			p->icon = (U32*)xbmpHeadGirl;
#endif
			p->w = p->h = 34;
			p->next = p->prev = nullptr;

			if (nullptr == m_chatGroup->msgHead)
				m_chatGroup->msgHead = p;
			if (nullptr == m_chatGroup->msgTail)
				m_chatGroup->msgTail = p;
			else  // put this message on the tail of this double link
			{
				m_chatGroup->msgTail->next = p;
				p->prev = m_chatGroup->msgTail;
				m_chatGroup->msgTail = p;
			}

			if (p->height > 0)
			{
				m_sizeAll.cy += p->height;
				m_ptOffset.y = (m_sizeAll.cy > h) ? (m_sizeAll.cy - h) : 0; // show the last message on the bottom
			}
			InterlockedIncrement(&(mt->state)); // ok, we have successfully processed this message task
			InvalidateScreen();
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
			WTChatMessage* p = m_chatGroup->msgHead;
			while (nullptr != p)
			{
				H = p->height;
				if (pos + H > m_ptOffset.y && pos < m_ptOffset.y + h) // this node is in the visible area, we need to draw it
				{
					dy = pos - m_ptOffset.y;
					x = (XMESSAGE_FROM_ME & p->state)? (w - m_scrollWidth - p->w - XWIN4_OFFSET) : XWIN4_OFFSET;
					DUI_ScreenDrawRectRound(m_screen, w, h, (U32*)p->icon, p->w, p->h, x, dy, m_backgroundColor, m_backgroundColor);
					if (XMESSAGE_CONFIRMATION & p->state)
					{
						DUI_ScreenDrawRect(m_screen, w, h, (U32*)xbmpOK, 10, 10, x-12, dy + 2);
					}
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
		HRESULT hr;
		U32 color;
		bool isMe;
		int x, y, dx, dy, W, H, pos;
		WTChatMessage* p;
		IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_MAINTEXT);
		ID2D1HwndRenderTarget* pD2DRenderTarget = static_cast<ID2D1HwndRenderTarget*>(surface);
		ID2D1SolidColorBrush* pTextBrush = static_cast<ID2D1SolidColorBrush*>(brushText);
		ID2D1SolidColorBrush* pBkgBrush0 = static_cast<ID2D1SolidColorBrush*>(brushBkg0);
		ID2D1SolidColorBrush* pBkgBrush1 = static_cast<ID2D1SolidColorBrush*>(brushBkg1);

		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		FLOAT Wf = static_cast<FLOAT>(w * 2 / 3);

		for (U16 i = 0; i < m_idxTL; i++)
		{
			assert(m_cacheTL[i]);
			SafeRelease(&m_cacheTL[i]);
		}
		m_idxTL = 0;

		if (m_chatGroup)
		{
			D2D1_POINT_2F orgin;
			D2D1_RECT_F bkgarea;
			int pos = XWIN4_OFFSET;
			p = m_chatGroup->msgHead;
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
						dx = w - p->width - 38 - m_scrollWidth - XWIN4_OFFSET - XWIN4_OFFSET;
					}

					bkgarea.left = static_cast<FLOAT>(m_area.left + dx) - XWIN4_OFFSET - XWIN4_OFFSET;
					bkgarea.top = static_cast<FLOAT>(m_area.top + dy);
					bkgarea.right = bkgarea.left + static_cast<FLOAT>(p->width) + XWIN4_OFFSET + XWIN4_OFFSET;
					bkgarea.bottom = bkgarea.top + static_cast<FLOAT>(p->height - WIN4_GAP_MESSAGE) + XWIN4_OFFSET + XWIN4_OFFSET;
					if(isMe)
						pD2DRenderTarget->FillRectangle(bkgarea, pBkgBrush0);
					else
						pD2DRenderTarget->FillRectangle(bkgarea, pBkgBrush1);

					hr = g_pDWriteFactory->CreateTextLayout(p->message, p->msgLen, pTextFormat, Wf, static_cast<FLOAT>(1), (IDWriteTextLayout**)(&m_cacheTL[m_idxTL]));

					if (S_OK == hr && m_cacheTL[m_idxTL])
					{
						orgin.x = static_cast<FLOAT>(dx + m_area.left - XWIN4_OFFSET);
						orgin.y = static_cast<FLOAT>(dy + m_area.top + XWIN4_OFFSET);

						m_offsetX[m_idxTL] = orgin.x;
						m_offsetY[m_idxTL] = orgin.y;

						pD2DRenderTarget->DrawTextLayout(orgin, m_cacheTL[m_idxTL], pTextBrush);
						m_idxTL++;

						if (m_idxTL >= 1024) // it is impossible
						{
							assert(0);
						}
					}
				}
				pos += H;
				if (pos >= m_ptOffset.y + h) // out of the scope of the drawing area
					break;
				p = p->next;
			}
		}
		return 0; 
	}

	int Do_DUI_MOUSEMOVE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		int i, idxHit, r = 0;
		FLOAT xPos = static_cast<FLOAT>(GET_X_LPARAM(lParam));
		FLOAT yPos = static_cast<FLOAT>(GET_Y_LPARAM(lParam));
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		
		HRESULT hr;
		BOOL isTrailingHit = FALSE;
		BOOL isInside = FALSE;
		DWRITE_HIT_TEST_METRICS htm = { 0 };
		FLOAT dx, dy;

		idxHit = -1;
		for (i = 0; i < m_idxTL; i++)
		{
			assert(m_cacheTL[i]);
			dx = xPos - m_offsetX[i];
			dy = yPos - m_offsetY[i];
			hr = m_cacheTL[i]->HitTestPoint(dx, dy, &isTrailingHit, &isInside, &htm);
			if (S_OK == hr && isInside)
			{
				::SetCursor(dui_hCursorIBeam);
				SetDUIWindowCursor();
				idxHit = i;
				break;
			}
		}

		return r; 
	}
};

#endif  /* __DUI_WINDOW4_H__ */

