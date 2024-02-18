#ifndef __DUI_WINDOW5_H__
#define __DUI_WINDOW5_H__

#include "dui/dui_win.h"

class XWindow5 : public XWindowT <XWindow5>
{
public:
	XWindow5()
	{
		m_backgroundColor = 0xFFFFFFFF;
		m_property |= (DUI_PROP_MOVEWIN | DUI_PROP_HANDLETIMER | DUI_PROP_HANDLEKEYBOARD | DUI_PROP_HANDLETEXT);
		m_message = WM_XWINDOWS05;
	}
	~XWindow5() {}

public:
	void UpdateControlPosition()
	{
		XControl* xctl;
		int sw, sh, dx, dy, top, bottom, gap = 10; // pixel
		int w = m_area.right - m_area.left;
		int h = m_area.bottom - m_area.top;
		int L, T, R, B;

		xctl = m_controlArray[XWIN5_BUTTON_EMOJI];

		R = w;
		L = gap;
		T = xctl->getBottom() + 10;

		xctl = m_controlArray[XWIN5_BUTTON_HINT];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dx = gap; dy = h - gap - sh;
		xctl->setPosition(dx, dy);

		B = xctl->getTop();

		xctl = m_controlArray[XWIN5_BUTTON_SENDMESSAGE];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dx = w - sw - gap / 2; dy = h - sh - gap / 2;
		xctl->setPosition(dx, dy);
		bottom = dy - gap / 2;

		dy = xctl->getTop();
		if (B > dy)
			B = dy;

		xctl = m_controlArray[XWIN5_BUTTON_VIDEOCALL];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dy = xctl->getTop();
		dx = w - sw - gap - 1;
		xctl->setPosition(dx, dy);
		top = dy + sh + gap / 2;

		xctl = m_controlArray[XWIN5_BUTTON_AUDIOCALL];
		assert(nullptr != xctl);
		sw = xctl->getWidth();
		sh = xctl->getHeight();
		dy = xctl->getTop();
		dx = dx - sw - gap;
		xctl->setPosition(dx, dy);

		// adjust the editbox position
		xctl = m_controlArray[XWIN5_EDITBOX_INPUT];
		xctl->setPosition(L, T, R, B, false); // the outer size
		R -= 12;
		L += 4;
		xctl->setPosition(L, T, R, B, true);  // the inner size
	}

private:
	void InitBitmap()
	{
		U8 id;
		XBitmap* bmp;

		int w = 19;
		int h = 19;
		id = XWIN5_BITMAP_EMOJIN;       bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpEmojiN;       bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_EMOJIH;       bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpEmojiH;       bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_EMOJIP;       bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpEmojiP;       bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_EMOJIA;       bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpEmojiN;       bmp->w = w; bmp->h = h;

		id = XWIN5_BITMAP_UPLOADN;      bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpUploadN;      bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_UPLOADH;      bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpUploadH;      bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_UPLOADP;      bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpUploadP;      bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_UPLOADA;      bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpUploadN;      bmp->w = w; bmp->h = h;

		id = XWIN5_BITMAP_CAPTUREN;     bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpCaptureN;     bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_CAPTUREH;     bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpCaptureH;     bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_CAPTUREP;     bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpCaptureP;     bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_CAPTUREA;     bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpCaptureN;     bmp->w = w; bmp->h = h;

		id = XWIN5_BITMAP_CHATHISTORYN; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpChatHistoryN; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_CHATHISTORYH; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpChatHistoryH; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_CHATHISTORYP; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpChatHistoryP; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_CHATHISTORYA; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpChatHistoryN; bmp->w = w; bmp->h = h;

		id = XWIN5_BITMAP_AUDIOCALLN;  bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpAudioCallN;  bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_AUDIOCALLH;  bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpAudioCallH;  bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_AUDIOCALLP;  bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpAudioCallP;  bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_AUDIOCALLA;  bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpAudioCallN;  bmp->w = w; bmp->h = h;

		w = 21; h = 19;
		id = XWIN5_BITMAP_VIDEOCALLN;   bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpVideoCallN;   bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_VIDEOCALLH;   bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpVideoCallH;   bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_VIDEOCALLP;   bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpVideoCallP;   bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_VIDEOCALLA;   bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpVideoCallN;   bmp->w = w; bmp->h = h;

		w = 52; h = 34;
		id = XWIN5_BITMAP_SENDMESSAGEN; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSendMessageN; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_SENDMESSAGEH; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSendMessageH; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_SENDMESSAGEP; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSendMessageP; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_SENDMESSAGEA; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpSendMessageH; bmp->w = w; bmp->h = h;

		w = 108; h = 14;
		id = XWIN5_BITMAP_HINTN; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpHint; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_HINTH; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpHint; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_HINTP; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpHint; bmp->w = w; bmp->h = h;
		id = XWIN5_BITMAP_HINTA; bmp = &m_bitmapArray[id]; bmp->id = id; bmp->data = (U32*)xbmpHint; bmp->w = w; bmp->h = h;
	}

public:
	int DoEditBoxPaste()
	{
		int r = 0;
		XEditBox* xeb;
		xeb = (XEditBox*)m_controlArray[XWIN5_EDITBOX_INPUT];
		r = xeb->PasteFromClipboard();

		if (r)
		{
			m_status |= DUI_STATUS_NEEDRAW;
			InvalidateDUIWindow();
		}

		return r;
	}

	void InitControl()
	{
		U8 id;
		int offsetX = 10, offsetY = 10, gap = 20;
		int dx, dy, sw, sh;
		U32 objSize = sizeof(XButton);
		U8* mem;
		XBitmap* bmpN;
		XBitmap* bmpH;
		XBitmap* bmpP;
		XBitmap* bmpA;
		XButton* button;
		XEditBox* editbox;

		assert(nullptr != m_pool);

		InitBitmap(); // inital all bitmap resource

		m_maxControl = 0;

		objSize = sizeof(XButton);
		id = XWIN5_BUTTON_EMOJI;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5EMOJI");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_EMOJIN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_EMOJIH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_EMOJIP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_EMOJIA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN5_BUTTON_UPLOAD;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5UPLOAD");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_UPLOADN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_UPLOADH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_UPLOADP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_UPLOADA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN5_BUTTON_CAPTURE;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5CAPTURE");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_CAPTUREN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_CAPTUREH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_CAPTUREP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_CAPTUREA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN5_BUTTON_CHATHISTORY;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5CHATHISTORY");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_CHATHISTORYN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_CHATHISTORYH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_CHATHISTORYP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_CHATHISTORYA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN5_BUTTON_AUDIOCALL;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5AUDIOCALL");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_AUDIOCALLN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_AUDIOCALLH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_AUDIOCALLP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_AUDIOCALLA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN5_BUTTON_VIDEOCALL;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5VIDEOCALL");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_VIDEOCALLN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_VIDEOCALLH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_VIDEOCALLP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_VIDEOCALLA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN5_BUTTON_SENDMESSAGE;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5SENDMESSAGE");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_SENDMESSAGEN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_SENDMESSAGEH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_SENDMESSAGEP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_SENDMESSAGEA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else return;

		id = XWIN5_BUTTON_HINT;
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			button = new(mem)XButton;
			assert(nullptr != button);
			button->Init(id, "W5HINT");
			bmpN = &m_bitmapArray[XWIN5_BITMAP_HINTN];
			bmpH = &m_bitmapArray[XWIN5_BITMAP_HINTH];
			bmpP = &m_bitmapArray[XWIN5_BITMAP_HINTP];
			bmpA = &m_bitmapArray[XWIN5_BITMAP_HINTA];
			button->setBitmap(bmpN, bmpH, bmpP, bmpA);
			sw = button->getWidth(); sh = button->getHeight();
			dx = offsetX;
			dy = offsetY;
			offsetX = dx + sw + gap;
			button->setPosition(dx, dy);
			button->setProperty(XCONTROL_PROP_STATIC);
			button->setRoundColor(m_backgroundColor, m_backgroundColor);
			m_controlArray[id] = button;
		}
		else
			return;

		id = XWIN5_EDITBOX_INPUT;
		objSize = sizeof(XEditBox);
		mem = (U8*)wt_palloc(m_pool, objSize);
		if (NULL != mem)
		{
			editbox = new(mem)XEditBox;
			assert(nullptr != editbox);
			IDWriteTextFormat* pTextFormat = GetTextFormat(WT_TEXTFORMAT_MAINTEXT);
			editbox->Init(id, "W5EDITINPUT", g_pDWriteFactory, pTextFormat);
			editbox->setProperty(XCONTROL_PROP_RBUP);
			m_controlArray[id] = editbox;
		}
		else
			return;

		m_maxControl = XWIN5_EDITBOX_INPUT + 1;
	}

	int Do_DUI_MOUSEMOVE(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		return 0; 
	}

	int Do_DUI_TIMER(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		return 0;
	}

	int Do_DUI_LBUTTONDOWN(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		return 0; 
	}

	int Do_DUI_KEYDOWN(U32 uMsg, U64 wParam, U64 lParam, void* lpData = nullptr) 
	{ 
		int ret = 0;
		U32 keyCode = static_cast<U32>(wParam);
		bool heldControl = (GetKeyState(VK_CONTROL) & 0x80) != 0;

		if (DUI_KEYDOWN == uMsg && VK_RETURN == keyCode && !heldControl)
		{
			XEditBox* eb = (XEditBox*)m_controlArray[XWIN5_EDITBOX_INPUT];
			assert(eb);

			U32 len = eb->GetInputMessageLengthInBytes();
			if (len > 0) // the user input some message that is ready to send
			{
				MessageTask* mt = (MessageTask*)wt_palloc0(m_pool, sizeof(MessageTask));
				if (mt)
				{
					U8* msgInput = (U8*)wt_palloc(m_pool, len + 4);
					if (msgInput)
					{
						if (eb->GetInputMessage(msgInput + 4, len))
						{
							int r = GetReceiverPublicKey(lpData, mt->pubkey);
							if (0 == r && (0x02 == mt->pubkey[0] || 0x03 == mt->pubkey[0])) // the first byte of public key is 02 or 03
							{
								U32* p = (U32*)msgInput;
								*p = ++g_messageSequence; // record the sequence
								mt->state = MESSAGE_TASK_STATE_NULL;
								mt->next = nullptr;
								mt->message = msgInput;
								mt->msgLen = len + 4;
								mt->type = 'T'; // this is the text message
								wt_sha256_hash((const unsigned char*)mt->message, mt->msgLen, mt->hash);
								PushTaskIntoSendMessageQueue(mt); // send to the network
								PostWindowMessage(WM_XWINDOWS04, WIN4_UPDATE_MESSAGE, (LPARAM)mt);
								ret = 1;
							}
							else
							{
								wt_pfree(msgInput);
								wt_pfree(mt);
							}
						}
					}
					else
					{
						wt_pfree(mt);
					}
				}
			}
		}
		return ret; 
	}

	int SelectUploadedFile(LPWSTR upfile)
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
		ofn.lpstrFilter = _T("All files(*.*)\0*.*\0\0");
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
};

#endif  /* __DUI_WINDOW5_H__ */

