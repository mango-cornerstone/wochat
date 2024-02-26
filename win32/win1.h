#ifndef __WOCHAT_WINDOWS1_H__
#define __WOCHAT_WINDOWS1_H__

#include "dui/dui_win.h"

static const wchar_t txtWin1PK[] = { 0x516c,0x94a5,0x0020,0x003a,0x0020,0 };
static const wchar_t txtWin1DT[] = { 0x521b,0x5efa,0x65f6,0x95f4,0xff1a,0 };
static const wchar_t txtWin1TBD[] = 
{ 0x672c,0x529f,0x80fd,0x6b63,0x5728,0x5f00,0x53d1,0x4e2d,0xff0c,0x8bf7,0x8010,0x5fc3,0x7b49,0x5f85,0xff01,0x003a,0x002d,0x0029,0 };

static const wchar_t txtNoMotto[] = { 0x6ca1, 0x6709, 0x5ea7, 0x53f3, 0x94ed, 0 };
static const wchar_t txtNoDOB[] = { 0x751f, 0x65e5, 0x672a, 0x77e5, 0 };
static const wchar_t txtUnknownGender[] = { 0x6027, 0x522b, 0x672a, 0x77e5, 0 };
static const wchar_t txtUnknownArea[] = { 0x6240, 0x5728, 0x5730, 0x533a, 0x672a, 0x77e5, 0 };

#define WIN1_MODE_TALK		DUI_MODE0
#define WIN1_MODE_ME		DUI_MODE1
#define WIN1_MODE_FRIEND	DUI_MODE2
#define WIN1_MODE_TBD		DUI_MODE3

class XWindow1 : public XWindowT <XWindow1>
{
public:
	XWindow1()
	{
		m_backgroundColor = 0xFFEAECED;
		m_property |= (DUI_PROP_MOVEWIN | DUI_PROP_HANDLETIMER | DUI_PROP_HANDLEKEYBOARD | DUI_PROP_HANDLETEXT);;
		m_message = WM_XWINDOWS01;
	}

	~XWindow1()	{}

	void InitBitmap()
	{
		U8 id;
		int w, h;
		XBitmap* bmp;

		w = 27; h = 27;
		id = XWIN1_BITMAP_TALK_SEARCHN; bmp = &(m_bmpArray[WIN1_MODE_TALK][id]); bmp->id = id; bmp->data = (U32*)xbmpSouAllN; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_TALK_SEARCHH; bmp = &(m_bmpArray[WIN1_MODE_TALK][id]); bmp->id = id; bmp->data = (U32*)xbmpSouAllH; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_TALK_SEARCHP; bmp = &(m_bmpArray[WIN1_MODE_TALK][id]); bmp->id = id; bmp->data = (U32*)xbmpSouAllH; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_TALK_SEARCHA; bmp = &(m_bmpArray[WIN1_MODE_TALK][id]); bmp->id = id; bmp->data = (U32*)xbmpSouAllH; bmp->w = w; bmp->h = h;

		w = 128; h = 128;
		id = XWIN1_BITMAP_ME_ICON;      bmp = &(m_bmpArray[WIN1_MODE_ME][id]); bmp->id = id; bmp->data = (U32*)g_myImage128; bmp->w = w; bmp->h = h;
		w = 32; h = 32;
		id = XWIN1_BITMAP_ME_PUBLICKEY; bmp = &(m_bmpArray[WIN1_MODE_ME][id]); bmp->id = id; bmp->data = (U32*)xbmpKey;    bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_ME_DOB;       bmp = &(m_bmpArray[WIN1_MODE_ME][id]); bmp->id = id; bmp->data = (U32*)xbmpDOB;    bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_ME_SEX;       bmp = &(m_bmpArray[WIN1_MODE_ME][id]); bmp->id = id; bmp->data = (U32*)xbmpGender; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_ME_AREA;      bmp = &(m_bmpArray[WIN1_MODE_ME][id]); bmp->id = id; bmp->data = (U32*)xbmpArea;   bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_ME_EDIT;      bmp = &(m_bmpArray[WIN1_MODE_ME][id]); bmp->id = id; bmp->data = (U32*)xbmpEdit;   bmp->w = w; bmp->h = h;

		w = 27; h = 27;
		id = XWIN1_BITMAP_FRIEND_SEARCHN; bmp = &(m_bmpArray[WIN1_MODE_FRIEND][id]); bmp->id = id; bmp->data = (U32*)xbmpSerachN; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_FRIEND_SEARCHN; bmp = &(m_bmpArray[WIN1_MODE_FRIEND][id]); bmp->id = id; bmp->data = (U32*)xbmpSerachH; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_FRIEND_SEARCHN; bmp = &(m_bmpArray[WIN1_MODE_FRIEND][id]); bmp->id = id; bmp->data = (U32*)xbmpSerachP; bmp->w = w; bmp->h = h;
		id = XWIN1_BITMAP_FRIEND_SEARCHN; bmp = &(m_bmpArray[WIN1_MODE_FRIEND][id]); bmp->id = id; bmp->data = (U32*)xbmpSerachP; bmp->w = w; bmp->h = h;
	}

	void InitControl()
	{
		U16 id, mode;
		U32 objSize;
		U8* mem;

		assert(nullptr != m_pool);

		InitBitmap(); // inital all bitmap resource

		mode = WIN1_MODE_TALK;
		/////////////////////////////////////////////////////
		id = XWIN1_TALK_EDITBOX_SEARCH;
		objSize = sizeof(XEditBoxLine);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XEditBoxLine* eb = new(mem)XEditBoxLine;
			assert(nullptr != eb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_MAINTEXT);
			eb->Init(((mode <<8) | id), "W1EDITSEARCH", g_pDWriteFactory, pTextFormat);
			eb->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = eb;
		}
		else return;

		objSize = sizeof(XButton);
		id = XWIN1_TALK_BUTTON_SEARCH;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNSEARCH");
			XBitmap* bmpN = &(m_bmpArray[mode][XWIN1_BITMAP_TALK_SEARCHN]);
			XBitmap* bmpH = &(m_bmpArray[mode][XWIN1_BITMAP_TALK_SEARCHH]);
			XBitmap* bmpP = &(m_bmpArray[mode][XWIN1_BITMAP_TALK_SEARCHP]);
			XBitmap* bmpA = &(m_bmpArray[mode][XWIN1_BITMAP_TALK_SEARCHA]);
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
			m_activeControl = id; 
		}
		else return;

		m_maxCtl[mode] = id + 1;

		mode = WIN1_MODE_ME;
		/////////////////////////////////////////////////////
		id = XWIN1_ME_BUTTON_MYICON;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNICON");
			XBitmap* bmp = &(m_bmpArray[mode][XWIN1_BITMAP_ME_ICON]);
			button->setBitmap(bmp, bmp, bmp, bmp);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
		}
		else return;

		id = XWIN1_ME_LABEL_MYNAME;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_OTHER0);
			assert(pTextFormat);
			lb->Init(((mode << 8) | id), "W1NAME", g_pDWriteFactory, pTextFormat);
			assert(g_myName);
			lb->setText(g_myName, wcslen(g_myName));
			m_ctlArray[mode][id] = lb;
		}
		else return;

		id = XWIN1_ME_BUTTON_PUBLICKEY;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNICON");
			XBitmap* bmp = &(m_bmpArray[mode][XWIN1_BITMAP_ME_PUBLICKEY]);
			button->setBitmap(bmp, bmp, bmp, bmp);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
		}
		else return;

		id = XWIN1_ME_LABEL_PUBLICKEY;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			wchar_t hexPK[67] = { 0 };
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_OTHER1);
			assert(pTextFormat);
			lb->Init(((mode << 8) | id), "W1PKValue", g_pDWriteFactory, pTextFormat);
			assert(g_PK);
			wt_Raw2HexStringW(g_PK, 33, hexPK, nullptr);
			lb->setText((wchar_t*)hexPK, wcslen(hexPK));
			m_ctlArray[mode][id] = lb;
		}
		else return;

		id = XWIN1_ME_LABEL_MOTTO;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_OTHER1);
			assert(pTextFormat);
			lb->Init(((mode << 8) | id), "W1PKValue", g_pDWriteFactory, pTextFormat);
			lb->setText((wchar_t*)txtNoMotto, wcslen(txtNoMotto));
			m_ctlArray[mode][id] = lb;
		}
		else return;

		id = XWIN1_ME_BUTTON_DOB;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNICON");
			XBitmap* bmp = &(m_bmpArray[mode][XWIN1_BITMAP_ME_DOB]);
			button->setBitmap(bmp, bmp, bmp, bmp);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
		}
		else return;

		id = XWIN1_ME_LABEL_DOB;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_OTHER1);
			assert(pTextFormat);
			lb->Init(((mode << 8) | id), "W1PKValue", g_pDWriteFactory, pTextFormat);
			assert(g_PK);
			lb->setText((wchar_t*)txtNoDOB, wcslen(txtNoDOB));
			m_ctlArray[mode][id] = lb;
		}
		else return;

		id = XWIN1_ME_BUTTON_SEX;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNICON");
			XBitmap* bmp = &(m_bmpArray[mode][XWIN1_BITMAP_ME_SEX]);
			button->setBitmap(bmp, bmp, bmp, bmp);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
		}
		else return;

		id = XWIN1_ME_LABEL_SEX;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_OTHER1);
			assert(pTextFormat);
			lb->Init(((mode << 8) | id), "W1PKValue", g_pDWriteFactory, pTextFormat);
			assert(g_PK);
			lb->setText((wchar_t*)txtUnknownGender, wcslen(txtUnknownGender));
			m_ctlArray[mode][id] = lb;
		}
		else return;

		id = XWIN1_ME_BUTTON_AREA;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNICON");
			XBitmap* bmp = &(m_bmpArray[mode][XWIN1_BITMAP_ME_AREA]);
			button->setBitmap(bmp, bmp, bmp, bmp);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
		}
		else return;

		id = XWIN1_ME_LABEL_AREA;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_OTHER1);
			assert(pTextFormat);
			lb->Init(((mode << 8) | id), "W1PKValue", g_pDWriteFactory, pTextFormat);
			assert(g_PK);
			lb->setText((wchar_t*)txtUnknownArea, wcslen(txtUnknownArea));
			m_ctlArray[mode][id] = lb;
		}
		else return;

		id = XWIN1_ME_BUTTON_EDIT;
		objSize = sizeof(XButton);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNICON");
			XBitmap* bmp = &(m_bmpArray[mode][XWIN1_BITMAP_ME_EDIT]);
			button->setBitmap(bmp, bmp, bmp, bmp);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
		}
		else return;

		m_maxCtl[mode] = id + 1;

		mode = WIN1_MODE_FRIEND;
		/////////////////////////////////////////////////////
		id = XWIN1_FRIEND_EDITOBX_SEARCH;
		objSize = sizeof(XEditBoxLine);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XEditBoxLine* eb = new(mem)XEditBoxLine;
			assert(nullptr != eb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_MAINTEXT);
			eb->Init(((mode << 8) | id), "W1EDITSEARCH", g_pDWriteFactory, pTextFormat);
			eb->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = eb;
		}
		else return;

		objSize = sizeof(XButton);
		id = XWIN1_FRIEND_BUTTON_SEARCH;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XButton* button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(((mode << 8) | id), "W1BTNSEARCH");
			XBitmap* bmpN = &(m_bmpArray[mode][XWIN1_BITMAP_FRIEND_SEARCHN]);
			XBitmap* bmpH = &(m_bmpArray[mode][XWIN1_BITMAP_FRIEND_SEARCHN]);
			XBitmap* bmpP = &(m_bmpArray[mode][XWIN1_BITMAP_FRIEND_SEARCHN]);
			XBitmap* bmpA = &(m_bmpArray[mode][XWIN1_BITMAP_FRIEND_SEARCHN]);
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_ctlArray[mode][id] = button;
			m_activeControl = id;
		}
		else return;

		m_maxCtl[mode] = id + 1;

		mode = WIN1_MODE_TBD;
		/////////////////////////////////////////////////////
		id = XWIN1_TBD_LABEL_INFO;
		objSize = sizeof(XLabel);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			XLabel* lb = new(mem)XLabel;
			assert(nullptr != lb);
			IDWriteTextFormat* pTextFormat = GetTextFormatAndHeight(WT_TEXTFORMAT_OTHER2);
			assert(pTextFormat);
			lb->Init(((mode << 8) | id), "W1PKValue", g_pDWriteFactory, pTextFormat);
			lb->setText((wchar_t*)txtWin1TBD, wcslen(txtWin1TBD));
			m_ctlArray[mode][id] = lb;
		}
		else return;

		m_maxCtl[mode] = id + 1;
	}

public:

	void AfterSetMode() 
	{
		switch (m_mode)
		{
		case WIN1_MODE_TALK:
		case WIN1_MODE_FRIEND:
			m_backgroundColor = 0xFFEAECED;
			break;
		case WIN1_MODE_ME:
		case WIN1_MODE_TBD:
			m_backgroundColor = 0xFFFFFFFF;
			break;
		default:
			assert(0);
			break;
		}
	}

	void UpdateControlPosition()
	{
		XControl* xctl;
		U16 id;
		int offset, gap = 10; // pixel
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int sw, sh, dx, dy;
		int L, R, T, B;

		switch (m_mode)
		{
		case WIN1_MODE_TALK:
			id = XWIN1_TALK_BUTTON_SEARCH;
			xctl = m_ctlArray[m_mode][id];
			assert(nullptr != xctl);
			sw = xctl->getWidth();
			sh = xctl->getHeight();
			dy = (h - sh) >> 1;
			dx = w - sw - gap;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_TALK_EDITBOX_SEARCH];
			assert(nullptr != xctl);
			L = gap;
			R = dx - (gap >> 1);
			T = dy;
			B = dy + sh;
			xctl->setPosition(L, T, R, B);
			break;
		case WIN1_MODE_ME:
			id = XWIN1_ME_BUTTON_MYICON;
			xctl = m_ctlArray[m_mode][id];
			assert(nullptr != xctl);
			sw = xctl->getWidth();
			sh = xctl->getHeight();
			dx = gap;
			dy = gap;
			xctl->setPosition(dx, dy);
			T = dy + sh + gap + gap + 1;
			R = dy + sh;

			id = XWIN1_ME_LABEL_MYNAME;
			xctl = m_ctlArray[m_mode][id];
			assert(nullptr != xctl);
			L = xctl->getHeight();
			offset = (sh - L) >> 1;
			dx = dx + sw + gap;
			dy = dy + offset;
			xctl->setPosition(dx, dy);

			id = XWIN1_ME_LABEL_MOTTO;
			xctl = m_ctlArray[m_mode][id];
			assert(nullptr != xctl);
			dy = dy + L + gap;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_BUTTON_PUBLICKEY];
			assert(nullptr != xctl);
			dx = gap;
			dy = T + (gap>>1);
			sw = xctl->getWidth();
			xctl->setPosition(dx, dy);
			B = xctl->getHeight();

			xctl = m_ctlArray[m_mode][XWIN1_ME_LABEL_PUBLICKEY];
			assert(nullptr != xctl);
			dx = dx + sw + gap;
			dy = dy + gap - 2;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_BUTTON_DOB];
			assert(nullptr != xctl);
			dx = gap;
			dy = dy + gap*4;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_LABEL_DOB];
			assert(nullptr != xctl);
			dx = dx + sw + gap;
			dy = dy + gap - 2;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_BUTTON_SEX];
			assert(nullptr != xctl);
			dx = gap;
			dy = dy + gap*4;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_LABEL_SEX];
			assert(nullptr != xctl);
			dx = dx + sw + gap;
			dy = dy + gap;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_BUTTON_AREA];
			assert(nullptr != xctl);
			dx = gap;
			dy = dy + gap * 4;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_LABEL_AREA];
			assert(nullptr != xctl);
			dx = dx + sw + gap;
			dy = dy + gap;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_ME_BUTTON_EDIT];
			assert(nullptr != xctl);
			dx = h - gap - xctl->getWidth();
			dy = R - xctl->getHeight();
			xctl->setPosition(dx, dy);

			break;
		case WIN1_MODE_FRIEND:
			xctl = m_ctlArray[m_mode][XWIN1_FRIEND_BUTTON_SEARCH];
			assert(nullptr != xctl);
			sw = xctl->getWidth();
			sh = xctl->getHeight();
			dy = (h - sh) >> 1;
			dx = w - sw - gap;
			xctl->setPosition(dx, dy);

			xctl = m_ctlArray[m_mode][XWIN1_FRIEND_EDITOBX_SEARCH];
			assert(nullptr != xctl);
			L = gap;
			R = dx - (gap >> 1);
			T = dy;
			B = dy + sh;
			xctl->setPosition(L, T, R, B);

			break;
		case WIN1_MODE_TBD:
			xctl = m_ctlArray[m_mode][XWIN1_TBD_LABEL_INFO];
			assert(nullptr != xctl);
			sw = xctl->getWidth();
			sh = xctl->getHeight();
			dx = (w - sw) >> 1;
			dy = (h - sh) >> 1;
			xctl->setPosition(dx, dy);
			break;
		default:
			assert(0);
			break;
		}
	}

	int Do_DUI_PAINT(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		if (WIN1_MODE_ME == m_mode)
		{
			int sw, sh, gap = 10; // pixel
			int w = m_area.right - m_area.left;
			int h = m_area.bottom - m_area.top;
			XControl* xctl;
			xctl = xctl = m_ctlArray[m_mode][XWIN1_ME_BUTTON_MYICON];
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

	void CopyPublicKey()
	{
		// Open and empty existing contents.
		if (OpenClipboard(nullptr))
		{
			if (EmptyClipboard())
			{
				size_t byteSize = 66 *sizeof(wchar_t) + 1;
				HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);
				if (hClipboardData != NULL)
				{
					void* memory = GlobalLock(hClipboardData);  // [byteSize] in bytes
					if (memory != NULL)
					{
						// Copy text to memory block.
						wchar_t hexPK[67];
						wt_Raw2HexStringW(g_PK, 33, hexPK, nullptr);
						memcpy(memory, hexPK, byteSize);
						GlobalUnlock(hClipboardData);
						if (SetClipboardData(CF_UNICODETEXT, hClipboardData) != NULL)
							hClipboardData = NULL; // system now owns the clipboard, so don't touch it.
					}
					GlobalFree(hClipboardData); // free if failed
				}
			}
			CloseClipboard();
		}
	}

};

#endif  /* __WOCHAT_WINDOWS1_H__ */

