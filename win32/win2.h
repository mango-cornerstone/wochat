#ifndef __DUI_WINDOW2_H__
#define __DUI_WINDOW2_H__

#include "dui/dui_win.h"

static const wchar_t AIGroupName[] = { 0x0041,0x0049,0x804a,0x5929,0x673a,0x5668,0x4eba,0 };

static const U16 txtGeneralSetting[] = { 0x901a,0x7528,0x8bbe,0x7f6e,0 };
static const U16 txtNetworkSetting[] = { 0x7f51,0x7edc,0x8bbe,0x7f6e,0 };
static const U16 txtAboutSetting[] = { 0x5173,0x4e8e,0x672c,0x8f6f,0x4ef6,0 };

static const wchar_t Win2LastMSG[] = {
	0x66fe, 0x7ecf, 0x6709, 0x4e00, 0x4efd, 0x771f, 0x8bda, 0x7684,
	0x7231, 0x60c5, 0x6446, 0x5728, 0x6211, 0x7684, 0x9762, 0x524d, 0 };

#define WIN2_MODE_TALK		DUI_MODE0
#define WIN2_MODE_FRIEND	DUI_MODE1
#define WIN2_MODE_SETTING	DUI_MODE2

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
		ICON_HEIGHTF = 32,
		ITEM_MARGIN = ((ITEM_HEIGHT - ICON_HEIGHT) >> 1),
		ITEM_MARGINF = ((ITEM_HEIGHT - ICON_HEIGHTF) >> 1)
	};

	int		m_lineHeight0 = 0;
	int		m_lineHeight1 = 0;

	XChatGroup* m_chatgroupRoot = nullptr;
	XChatGroup* m_chatgroupSelected = nullptr;
	XChatGroup* m_chatgroupSelectPrev = nullptr;
	XChatGroup* m_chatgroupHovered = nullptr;
	int m_chatgroupTotal = 0;

	WTSetting* m_settingRoot = nullptr;
	WTSetting* m_settingSelected = nullptr;
	WTSetting* m_settingSelectPrev = nullptr;
	WTSetting* m_settingHovered = nullptr;
	int m_settingTotal = 0;

	WTFriend* m_friendRoot = nullptr;
	WTFriend* m_friendSelected = nullptr;
	WTFriend* m_friendSelectPrev = nullptr;
	WTFriend* m_friendHovered = nullptr;
	int m_friendTotal = 0;

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

	void AfterSetMode()
	{
		switch (m_mode)
		{
		case WIN2_MODE_TALK:
			m_sizeAll.cy = m_chatgroupTotal * ITEM_HEIGHT;
			break;
		case WIN2_MODE_FRIEND:
			m_sizeAll.cy = m_friendTotal * ITEM_HEIGHT;
			break;
		case WIN2_MODE_SETTING:
			m_sizeAll.cy = m_settingTotal * ITEM_HEIGHT;
			break;
		default:
			assert(0);
			break;
		}
	}

	U32 LoadFriends()
	{
		assert(nullptr == m_friendRoot);
		assert(nullptr != m_pool);

		m_friendTotal = 0;
		m_friendRoot = (WTFriend*)wt_palloc0(m_pool, sizeof(WTFriend));
		if(m_friendRoot)
		{
			int rc;
			sqlite3* db;
			sqlite3_stmt* stmt = NULL;

			rc = sqlite3_open16(g_DBPath, &db);
			if (SQLITE_OK != rc)
			{
				sqlite3_close(db);
				wt_pfree(m_friendRoot);
				m_friendRoot = nullptr;
				return WT_SQLITE_OPEN_ERR;
			}

			rc = sqlite3_prepare_v2(db, (const char*)"SELECT pk,nm,b0,b1 FROM p WHERE at<>0", -1, &stmt, NULL);
			if (SQLITE_OK == rc)
			{
				rc = sqlite3_step(stmt);
				if (SQLITE_ROW == rc)
				{
					U8* hexPK = (U8*)sqlite3_column_text(stmt, 0);
					U8* utf8Name = (U8*)sqlite3_column_text(stmt, 1);
					U8* img32 = (U8*)sqlite3_column_blob(stmt, 2);
					int bytes32 = sqlite3_column_bytes(stmt, 2);
					U8* img128 = (U8*)sqlite3_column_blob(stmt, 3);
					int bytes128 = sqlite3_column_bytes(stmt, 3);
					size_t pkLen = strlen((const char*)hexPK);
					if (wt_IsPublicKey(hexPK, pkLen))
					{
						U32 utf16len;
						size_t utf8len = strlen((const char*)utf8Name);

						wt_HexString2Raw(hexPK, 66, m_friendRoot->pubkey, nullptr);

						U32 status = wt_UTF8ToUTF16(utf8Name, (U32)utf8len, nullptr, &utf16len);
						if (WT_OK == status)
						{
							m_friendRoot->name = (U16*)wt_palloc(m_pool, (utf16len + 1) * sizeof(U16));
							assert(m_friendRoot->name);
							m_friendRoot->nameLen = (U8)utf16len;
							wt_UTF8ToUTF16(utf8Name, (U32)utf8len, (U16*)m_friendRoot->name, nullptr);
							m_friendRoot->name[utf16len] = L'\0';

							m_friendTotal++;
							m_friendSelected = m_friendRoot;
							if (bytes32 == MY_ICON_SIZE32 * MY_ICON_SIZE32 * sizeof(U32))
							{
								m_friendRoot->iconSmall = (U32*)wt_palloc0(m_pool, bytes32);
								assert(m_friendRoot->iconSmall);
								memcpy(m_friendRoot->iconSmall, img32, bytes32);
								m_friendRoot->wSmall = MY_ICON_SIZE32;
								m_friendRoot->hSmall = MY_ICON_SIZE32;
							}

							if (bytes128 == MY_ICON_SIZE128 * MY_ICON_SIZE128 * sizeof(U32))
							{
								m_friendRoot->iconLarge = (U32*)wt_palloc0(m_pool, bytes128);
								assert(m_friendRoot->iconLarge);
								memcpy(m_friendRoot->iconLarge, img128, bytes128);
								m_friendRoot->wLarge = MY_ICON_SIZE128;
								m_friendRoot->hLarge = MY_ICON_SIZE128;
							}
						}
					}
					else
					{
						wt_pfree(m_friendRoot);
						m_friendRoot = nullptr;
					}
				}
				sqlite3_finalize(stmt);
			}
			else
			{
				wt_pfree(m_friendRoot);
				m_friendRoot = nullptr;
			}
			sqlite3_close(db);
		}

		return WT_OK;
	}

	U32 InitSettings()
	{
		U16 h = 20;
		assert(nullptr == m_settingRoot);
		assert(nullptr != m_pool);

		IDWriteTextLayout* pTextLayout = nullptr;
		IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
		HRESULT hr = g_pDWriteFactory->CreateTextLayout((wchar_t*)txtGeneralSetting, 4, pTextFormat, 600.f, 1.f, &pTextLayout);

		if (S_OK == hr && pTextLayout)
		{
			DWRITE_TEXT_METRICS tm;
			pTextLayout->GetMetrics(&tm);
			h = static_cast<int>(tm.height) + 1;
		}
		SafeRelease(&pTextLayout);

		m_settingTotal = 0;
		m_settingRoot = (WTSetting*)wt_palloc0(m_pool, sizeof(WTSetting));
		if (m_settingRoot)
		{
			m_settingTotal++;
			m_settingSelected = m_settingRoot;
			WTSetting* p = m_settingRoot;
			p->id = SETTING_GENERAL;
			p->height = h;
			p->name = (U16*)txtGeneralSetting;
			p->nameLen = 4;

			WTSetting* q = (WTSetting*)wt_palloc0(m_pool, sizeof(WTSetting));
			if (q)
			{
				m_settingTotal++;
				q->id = SETTING_NETWORK;
				p->height = h;
				q->name = (U16*)txtNetworkSetting;
				q->nameLen = 4;
				p->next = q;
				p = q;
				q = (WTSetting*)wt_palloc0(m_pool, sizeof(WTSetting));
				if (q)
				{
					m_settingTotal++;
					q->id = SETTING_ABOUT;
					p->height = h;
					q->name = (U16*)txtAboutSetting;
					q->nameLen = 5;
					p->next = q;
					p = q;
				}
			}



		}
		return WT_OK;
	}

	int LoadChatGroupList()
	{
#if 0
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
#endif
		return 0;
	}

	int Do_DUI_CREATE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int ret = 0;

		LoadFriends();
		InitSettings();

#if 0
		ret = LoadChatGroupList();
#endif 
		return ret;
	}

	int Do_DUI_DESTROY(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
#if 0
		XChatGroup* p = m_chatgroupRoot;

		while (nullptr != p)
		{
			assert(nullptr != p->mempool);
			wt_mempool_destroy(p->mempool);
			p->mempool = nullptr;
			p = p->next;
		}
#endif 
		return 0;
	}


public:
	int DoTalkDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{
#if 0
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
#endif 
		return 0;
	}

	int DoSettingDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{
		HRESULT hr;
		IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
		ID2D1HwndRenderTarget* pD2DRenderTarget = static_cast<ID2D1HwndRenderTarget*>(surface);
		ID2D1SolidColorBrush* pTextBrush = static_cast<ID2D1SolidColorBrush*>(brushText);
		ID2D1SolidColorBrush* pBkgBrush0 = static_cast<ID2D1SolidColorBrush*>(brushBkg0);
		ID2D1SolidColorBrush* pBkgBrush1 = static_cast<ID2D1SolidColorBrush*>(brushBkg1);
		IDWriteTextLayout* pTextLayout = nullptr;

		assert(pTextFormat);

		int dx, dy, pos;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		D2D1_POINT_2F orgin;
		dx = ITEM_MARGIN;

		WTSetting* p = m_settingRoot;
		pos = 0;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				dy = pos - m_ptOffset.y + ((ITEM_HEIGHT - p->height)>>1);
				hr = g_pDWriteFactory->CreateTextLayout((wchar_t*)p->name, p->nameLen, pTextFormat, static_cast<FLOAT>(w), static_cast<FLOAT>(1), &pTextLayout);
				if (S_OK == hr)
				{
					orgin.x = static_cast<FLOAT>(dx + m_area.left);
					orgin.y = static_cast<FLOAT>(dy + m_area.top);
					pD2DRenderTarget->DrawTextLayout(orgin, pTextLayout, pTextBrush);
				}
				SafeRelease(&pTextLayout);
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
		HRESULT hr;
		IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_GROUPNAME);
		ID2D1HwndRenderTarget* pD2DRenderTarget = static_cast<ID2D1HwndRenderTarget*>(surface);
		ID2D1SolidColorBrush* pTextBrush = static_cast<ID2D1SolidColorBrush*>(brushText);
		ID2D1SolidColorBrush* pBkgBrush0 = static_cast<ID2D1SolidColorBrush*>(brushBkg0);
		ID2D1SolidColorBrush* pBkgBrush1 = static_cast<ID2D1SolidColorBrush*>(brushBkg1);
		IDWriteTextLayout* pTextLayout = nullptr;

		assert(pTextFormat);

		int dx, dy, pos;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		D2D1_POINT_2F orgin;
		dx = ITEM_MARGINF;

		WTFriend* p = m_friendRoot;
		pos = 0;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				dx = ITEM_MARGINF + ICON_HEIGHTF + ITEM_MARGINF;
				dy = pos - m_ptOffset.y + ITEM_MARGINF;
				hr = g_pDWriteFactory->CreateTextLayout((wchar_t*)p->name, p->nameLen, pTextFormat, static_cast<FLOAT>(w), static_cast<FLOAT>(1), &pTextLayout);
				if (S_OK == hr)
				{
					orgin.x = static_cast<FLOAT>(dx + m_area.left);
					orgin.y = static_cast<FLOAT>(dy + m_area.top);
					pD2DRenderTarget->DrawTextLayout(orgin, pTextLayout, pTextBrush);
				}
				SafeRelease(&pTextLayout);
			}
			p = p->next;
			pos += ITEM_HEIGHT;
			if (pos >= (m_ptOffset.y + h))
				break;
		}

		return 0;
	}

	int DoDrawText(DUI_Surface surface, DUI_Brush brushText, DUI_Brush brushSelText, DUI_Brush brushCaret, DUI_Brush brushBkg0, DUI_Brush brushBkg1)
	{
		int r = 0;
		switch (m_mode)
		{
		case WIN2_MODE_TALK:
			r = DoTalkDrawText(surface, brushText, brushSelText, brushCaret, brushBkg0, brushBkg1);
			break;
		case WIN2_MODE_FRIEND:
			r = DoFriendDrawText(surface, brushText, brushSelText, brushCaret, brushBkg0, brushBkg1);
			break;
		case WIN2_MODE_SETTING:
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
#if 0
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
#endif 
		return 0;
	}

	int FriendPaint(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = 0;

		U32 color;
		int dx, dy, pos;
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		int H = ITEM_HEIGHT;

		dx = ITEM_MARGINF;
		WTFriend* p = m_friendRoot;
		pos = 0;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				if (p == m_friendSelected)
					color = SELECTED_COLOR;
				else if (p == m_friendHovered)
					color = HOVERED_COLOR;
				else
					color = DEFAULT_COLOR;

				dy = pos - m_ptOffset.y;
				DUI_ScreenFillRect(m_screen, w, h, color, w - margin, ITEM_HEIGHT, 0, dy);
				DUI_ScreenDrawRectRound(m_screen, w, h, p->iconSmall, p->wSmall, p->hSmall, dx, dy + ITEM_MARGINF, color, color);
			}
			p = p->next;
			pos += ITEM_HEIGHT;
			if (pos >= (m_ptOffset.y + h))
				break;
		}
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

		WTSetting* p = m_settingRoot;
		pos = 0;
		p = m_settingRoot;
		while (nullptr != p)
		{
			if (pos + ITEM_HEIGHT > m_ptOffset.y)
			{
				if (p == m_settingSelected)
					color = SELECTED_COLOR;
				else if (p == m_settingHovered)
					color = HOVERED_COLOR;
				else
					color = DEFAULT_COLOR;
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
		case WIN2_MODE_TALK:
			r = TalkPaint(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_FRIEND:
			r = FriendPaint(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_SETTING:
			r = SettingPaint(uMsg, wParam, lParam, lpData);
			break;
		default:
			assert(0);
			break;
		}
		return r;
	}

	int TalkMouseMove(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		return 0;
	}

	int FriendMouseMove(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int hit = -1, margin;
		int ret = 0;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		WTFriend* hovered = m_friendHovered;
		RECT area = { 0 };
		RECT* r = &area;
		margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		r->right = w - margin;
		r->left = 0;

		if (XWinPointInRect(xPos, yPos, &m_area))
		{
			int pos = 0;
			// transfer the coordination from screen to local virutal window
			xPos -= m_area.left;
			yPos -= m_area.top;

			assert(xPos >= 0);
			assert(yPos >= 0);

			WTFriend* p = m_friendRoot;
			while (nullptr != p)
			{
				r->top = pos; r->bottom = r->top + ITEM_HEIGHT;
				if (XWinPointInRect(xPos, yPos + m_ptOffset.y, r))
				{
					m_friendHovered = p;
					hit++;
					if (hovered != m_friendHovered) // The hovered item is changed, we need to redraw
						ret++;
					break;
				}
				p = p->next;
				pos += ITEM_HEIGHT;
				if (pos >= (m_ptOffset.y + h))
					break;
			}
		}
		if (-1 == hit)
		{
			if (nullptr != m_friendHovered)
			{
				m_friendHovered = nullptr;
				ret++;
			}
		}
		return ret;
	}

	int SettingMouseMove(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int hit = -1, margin;
		int ret = 0;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		WTSetting* hovered = m_settingHovered;
		RECT area = { 0 };
		RECT* r = &area;
		margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		r->right = w - margin;
		r->left = 0;

		if (XWinPointInRect(xPos, yPos, &m_area))
		{
			int pos = 0;
			// transfer the coordination from screen to local virutal window
			xPos -= m_area.left;
			yPos -= m_area.top;

			assert(xPos >= 0);
			assert(yPos >= 0);

			WTSetting* p = m_settingRoot;
			while (nullptr != p)
			{
				r->top = pos; r->bottom = r->top + ITEM_HEIGHT;
				if (XWinPointInRect(xPos, yPos + m_ptOffset.y, r))
				{
					m_settingHovered = p;
					hit++;
					if (hovered != m_settingHovered) // The hovered item is changed, we need to redraw
						ret++;
					break;
				}
				p = p->next;
				pos += ITEM_HEIGHT;
				if (pos >= (m_ptOffset.y + h))
					break;
			}
		}
		if (-1 == hit)
		{
			if (nullptr != m_settingHovered)
			{
				m_settingHovered = nullptr;
				ret++;
			}
		}
		return ret;
	}

	int Do_DUI_MOUSEMOVE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = 0;
		switch (m_mode)
		{
		case WIN2_MODE_TALK:
			r = TalkMouseMove(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_FRIEND:
			r = FriendMouseMove(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_SETTING:
			r = SettingMouseMove(uMsg, wParam, lParam, lpData);
			break;
		default:
			assert(0);
			break;
		}
		return r;
	}

	int TalkLButtonDown(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		return 0;
	}

	int FriendLButtonDown(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int hit = -1, margin;
		int ret = 0;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		RECT area = { 0 };
		RECT* r = &area;
		margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		r->right = w - margin;
		r->left = 0;

		if (XWinPointInRect(xPos, yPos, &m_area))
		{
			int pos = 0;
			// transfer the coordination from screen to local virutal window
			xPos -= m_area.left;
			yPos -= m_area.top;
			WTFriend* p = m_friendRoot;
			while (nullptr != p)
			{
				r->top = pos; r->bottom = r->top + ITEM_HEIGHT;
				if (XWinPointInRect(xPos, yPos + m_ptOffset.y, r))
				{
					m_friendHovered = p;
					m_friendSelected = p;
					hit++;
					ret++;
					break;
				}
				p = p->next;
				pos += ITEM_HEIGHT;
				if (pos >= (m_ptOffset.y + h))
					break;
			}
		}
		if (-1 == hit)
		{
			if (nullptr != m_friendHovered)
			{
				m_friendHovered = nullptr;
				ret++;;
			}
		}
		return ret;
	}

	int SettingLButtonDown(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int hit = -1, margin;
		int ret = 0;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		RECT area = { 0 };
		RECT* r = &area;
		margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		r->right = w - margin;
		r->left = 0;

		if (XWinPointInRect(xPos, yPos, &m_area))
		{
			int pos = 0;
			// transfer the coordination from screen to local virutal window
			xPos -= m_area.left;
			yPos -= m_area.top;
			WTSetting* p = m_settingRoot;
			while (nullptr != p)
			{
				r->top = pos; r->bottom = r->top + ITEM_HEIGHT;
				if (XWinPointInRect(xPos, yPos + m_ptOffset.y, r))
				{
					m_settingHovered = p;
					m_settingSelected = p;
					hit++;
					ret++;
					break;
				}
				p = p->next;
				pos += ITEM_HEIGHT;
				if (pos >= (m_ptOffset.y + h))
					break;
			}
		}
		if (-1 == hit)
		{
			if (nullptr != m_settingHovered)
			{
				m_settingHovered = nullptr;
				ret++;;
			}
		}
		return ret;

		return 0;
	}

	int Do_DUI_LBUTTONDOWN(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = 0;
		switch (m_mode)
		{
		case WIN2_MODE_TALK:
			r = TalkLButtonDown(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_FRIEND:
			r = FriendLButtonDown(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_SETTING:
			r = SettingLButtonDown(uMsg, wParam, lParam, lpData);
			break;
		default:
			assert(0);
			break;
		}
		return r;
	}

	int TalkLButtonUp(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		return 0;
	}

	int FriendLButtonUp(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int hit = -1, margin;
		int ret = 0;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		RECT area = { 0 };
		RECT* r = &area;
		margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		r->right = w - margin;
		r->left = 0;

		if (XWinPointInRect(xPos, yPos, &m_area))
		{
			int pos = 0;
			// transfer the coordination from screen to local virutal window
			xPos -= m_area.left;
			yPos -= m_area.top;

			WTFriend* p = m_friendRoot;
			while (nullptr != p)
			{
				r->top = pos; r->bottom = r->top + ITEM_HEIGHT;
				if (XWinPointInRect(xPos, yPos + m_ptOffset.y, r))
				{
					m_friendHovered = p;
					m_friendSelected = p;
					hit++;
					ret++;
					break;
				}
				p = p->next;
				pos += ITEM_HEIGHT;
				if (pos >= (m_ptOffset.y + h))
					break;
			}
		}
		if (-1 == hit)
		{
			if (nullptr != m_friendHovered)
			{
				m_friendHovered = nullptr;
				ret++;
			}
		}
		else // we hit some item
		{
			assert(nullptr != m_friendSelected);
			if (m_friendSelectPrev != m_friendSelected)
			{
				// do something here
				NotifyParent(m_message, (U64)m_mode, (U64)m_friendSelected);
			}
			m_friendSelectPrev = m_friendSelected; // prevent double selection
		}
		return ret;
	}

	int SettingLButtonUp(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int hit = -1, margin;
		int ret = 0;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;

		RECT area = { 0 };
		RECT* r = &area;
		margin = (DUI_STATUS_VSCROLL & m_status) ? m_scrollWidth : 0;
		r->right = w - margin;
		r->left = 0;

		if (XWinPointInRect(xPos, yPos, &m_area))
		{
			int pos = 0;
			// transfer the coordination from screen to local virutal window
			xPos -= m_area.left;
			yPos -= m_area.top;

			WTSetting* p = m_settingRoot;
			while (nullptr != p)
			{
				r->top = pos; r->bottom = r->top + ITEM_HEIGHT;
				if (XWinPointInRect(xPos, yPos + m_ptOffset.y, r))
				{
					m_settingHovered = p;
					m_settingSelected = p;
					hit++;
					ret++;
					break;
				}
				p = p->next;
				pos += ITEM_HEIGHT;
				if (pos >= (m_ptOffset.y + h))
					break;
			}
		}
		if (-1 == hit)
		{
			if (nullptr != m_settingHovered)
			{
				m_settingHovered = nullptr;
				ret++;
			}
		}
		else // we hit some item
		{
			assert(nullptr != m_settingSelected);
			if (m_settingSelectPrev != m_settingSelected)
			{
				// do something here
				//NotifyParent(m_message, (U64)m_mode, (U64)m_friendSelected);
			}
			m_settingSelectPrev = m_settingSelected; // prevent double selection
		}
		return ret;
	}

	int Do_DUI_LBUTTONUP(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr)
	{
		int r = 0;
		switch (m_mode)
		{
		case WIN2_MODE_TALK:
			r = TalkLButtonUp(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_FRIEND:
			r = FriendLButtonUp(uMsg, wParam, lParam, lpData);
			break;
		case WIN2_MODE_SETTING:
			r = SettingLButtonUp(uMsg, wParam, lParam, lpData);
			break;
		default:
			assert(0);
			break;
		}
		return r;
	}

};

#endif  /* __DUI_WINDOW2_H__ */

