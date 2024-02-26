#include "pch.h"
#include "resource.h"
#include "wochat.h"
#include "winlog.h"

static const wchar_t txtVeryBad[] = { 0x53d1, 0x751f, 0x9519, 0x8bef, 0 };
static const wchar_t txtVeryGood[] = { 0x4e00, 0x5207, 0x6b63, 0x5e38, 0 };
static const wchar_t txtImportData[] = { 0x5bfc,0x5165, 0 };

static wchar_t titleRegistration[] = {0x6ce8, 0x518c, 0x8d26,0x53f7, 0 };
static wchar_t titleLogin[] = { 0x767b, 0x5f55, 0};
static wchar_t txtBtnCancel[] = { 0x53d6, 0x6d88, 0 };
static wchar_t txtBtnRegistration[] = { 0x6ce8, 0x518c, 0 };
static wchar_t txtStaticName[] = { 0x7528, 0x6237, 0x540d, 0xff1a, 0 };
static wchar_t txtStaticPWD0[] = { 0x5bc6, 0x7801, 0xff1a, 0 };
static wchar_t txtHide[] = { 0x9690, 0x85cf ,0 };

static const wchar_t txtCannotOpenSK[] = { 0x65e0,0x6cd5,0x6253,0x5f00,0x79c1,0x94a5,0xff01,0 };

static wchar_t txtCreateGood[] = { 0x8d26, 0x53f7, 0x521b, 0x5efa, 0x6210, 0x529f, 0x0021, 0 };
static wchar_t txtCreateBad[] = { 0x8d26, 0x53f7, 0x521b, 0x5efa, 0x5931, 0x8d25, 0x0021, 0x0020, 0x4f60, 0x53ef, 0x4ee5, 0x91cd, 0x65b0, 0x521b, 0x5efa, 0x3002, 0x003a, 0x002d, 0x0029, 0 };
static wchar_t txtPWDIsNotSame[] = {
0x4e24, 0x6b21, 0x8f93, 0x5165, 0x7684, 0x5bc6, 0x7801, 0x4e0d, 0x76f8, 0x540c, 0xff0c, 0x8bf7, 0x91cd, 0x65b0, 0x8bbe, 0x7f6e, 0x5bc6, 0x7801, 0xff01, 0 };

static wchar_t txtNameEmpty[] = { 0x7528, 0x6237, 0x540d, 0x4e0d, 0x80fd, 0x4e3a, 0x7a7a, 0xff01, 0 };
static wchar_t txtPWDEmpty[] = { 0x5bc6, 0x7801, 0x4e0d, 0x80fd, 0x4e3a, 0x7a7a, 0xff01, 0 };

static wchar_t txtN1[] = { 0x6625, 0x7533, 0x95e8, 0x4e0b, 0x4e00, 0x5200, 0x5ba2, 0 };
static wchar_t txtN2[] = { 0x57ce, 0x5357, 0x5c0f, 0x675c, 0 };

static int WidthofLoginDlg = 0;
static int HeightofLoginDlg = 0;

BOOL	g_fThemeApiAvailable = FALSE;

HTHEME _OpenThemeData(HWND hwnd, LPCWSTR pszClassList)
{
	if (g_fThemeApiAvailable)
		return OpenThemeData(hwnd, pszClassList);
	else
		return NULL;
}

HRESULT _CloseThemeData(HTHEME hTheme)
{
	if (g_fThemeApiAvailable)
		return CloseThemeData(hTheme);
	else
		return E_FAIL;
}

static int loginResult = 1;

static void DebugDisplay(const char* s) noexcept 
{
	::OutputDebugStringA(s);
}

void DebugPrintf(const char* format, ...) noexcept 
{
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsnprintf(buffer, std::size(buffer), format, pArguments);
	va_end(pArguments);
	DebugDisplay(buffer);
}

static BOOL DrawBitmapButton(DRAWITEMSTRUCT* dis);
static void MakeBitmapButton(HWND hwnd, UINT uIconId);

static INT_PTR RegistrationDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR LoginDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define SZCLASS_LOGIN			_T("XWinLogin")
#define SZCLASS_REGISTRAION		_T("XWinRegistration")

static int DoWoChatRegistration(HINSTANCE hInstance)
{
	int r = 0;
	HWND hWnd;
	WNDCLASSEX wc;
	MSG msg;

	wc.cbSize = sizeof(wc);
	GetClassInfoEx(0, _T("#32770"), &wc);
	wc.style &= ~CS_GLOBALCLASS;
	wc.lpszClassName = SZCLASS_REGISTRAION;

	if (RegisterClassExW(&wc))
	{
		hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_REGISTER), 0, RegistrationDialogProc);
		if (::IsWindow(hWnd))
		{
			RECT rcDlg;
			::GetWindowRect(hWnd, &rcDlg);
			SetWindowPos(hWnd, 0, 500, 300, rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top, SWP_SHOWWINDOW);
			while (GetMessage(&msg, NULL, 0, 0))
			{
				// Get the accelerator keys before IsDlgMsg gobbles them up!
				if (!TranslateAccelerator(hWnd, NULL, &msg))
				{
					// Let IsDialogMessage process TAB etc
					if (!IsDialogMessage(hWnd, &msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
		}
		UnregisterClass(SZCLASS_REGISTRAION, hInstance);
	}

	return loginResult;
}

static int DoWoChatLogin(HINSTANCE hInstance)
{
	int r = 0;
	HWND hWnd;
	WNDCLASSEX wc;
	MSG msg;

	wc.cbSize = sizeof(wc);
	GetClassInfoEx(0, _T("#32770"), &wc);
	wc.style &= ~CS_GLOBALCLASS;
	wc.lpszClassName = SZCLASS_LOGIN;

	if (RegisterClassExW(&wc))
	{
		hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_LOGIN), 0, LoginDialogProc);
		if (::IsWindow(hWnd))
		{
			RECT rcDlg;
			::GetWindowRect(hWnd, &rcDlg);
			WidthofLoginDlg = rcDlg.right - rcDlg.left;
			HeightofLoginDlg = rcDlg.bottom - rcDlg.top;
			SetWindowPos(hWnd, 0, 500, 300, rcDlg.right - rcDlg.left - 280, rcDlg.bottom - rcDlg.top, SWP_SHOWWINDOW);
			while (GetMessage(&msg, NULL, 0, 0))
			{
				// Get the accelerator keys before IsDlgMsg gobbles them up!
				if (!TranslateAccelerator(hWnd, NULL, &msg))
				{
					// Let IsDialogMessage process TAB etc
					if (!IsDialogMessage(hWnd, &msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
		}
		UnregisterClass(SZCLASS_LOGIN, hInstance);
	}

	return loginResult;
}

static const wchar_t txtWtDBError[] = 
{ 0x65e0, 0x6cd5, 0x8bfb, 0x53d6, 0x0077, 0x0074, 0x002e, 0x0064, 0x0062, 0x6570, 0x636e, 0x5e93, 0x7684, 0x5bc6, 0x94a5, 0x6570, 0x636e, 0xff01, 0 };

int DoWoChatLoginOrRegistration(HINSTANCE hInstance)
{
	U32 total = 0;
	U32 status = GetSecretKeyNumber(&total);
	
	if (WT_OK != status)
	{
		MessageBox(NULL, txtWtDBError, txtVeryBad, MB_OK);
		return (int)status;
	}

	return total ? DoWoChatLogin(hInstance) : DoWoChatRegistration(hInstance);
}


INT_PTR RCommandHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR r = 0;
	HWND hWndCtl; 
	switch (LOWORD(wParam))
	{
	case IDB_CREATE:
		{
			int checkIsGood = 1;
			int len, len0, len1;
			wchar_t name[64 + 1] = { 0 };
			wchar_t pwd0[64 + 1] = { 0 };
			wchar_t pwd1[64 + 1] = { 0 };

			hWndCtl = GetDlgItem(hWnd, IDC_EDIT_NAME);
			len = GetWindowTextW(hWndCtl, name, 64);
			if (0 == len)
			{
				MessageBox(hWnd, txtNameEmpty, txtVeryBad, MB_OK);
				checkIsGood = 0;
			}
			else
			{
				hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
				len0 = GetWindowTextW(hWndCtl, pwd0, 64);
				hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
				len1 = GetWindowTextW(hWndCtl, pwd1, 64);
				if (len0 == 0 && len1 == 0)
				{
					MessageBox(hWnd, txtPWDEmpty, txtVeryBad, MB_OK);
					checkIsGood = 0;
				}
				else
				{
					if (len0 != len1)
					{
						MessageBox(hWnd, txtPWDIsNotSame, txtVeryBad, MB_OK);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
						SetWindowTextW(hWndCtl, nullptr);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
						SetWindowTextW(hWndCtl, nullptr);
						checkIsGood = 0;
					}
					else if (wmemcmp(pwd0, pwd1, len0))
					{
						MessageBox(hWnd, txtPWDIsNotSame, txtVeryBad, MB_OK);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
						SetWindowTextW(hWndCtl, nullptr);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
						SetWindowTextW(hWndCtl, nullptr);
						checkIsGood = 0;
					}
				}
			}
			if (checkIsGood)
			{
				U32 idxSk = 0xFFFFFFFF;
				if (WT_OK == CreateNewAccount(name, len, pwd0, len0, &idxSk))
				{
					MessageBox(hWnd, txtCreateGood, txtVeryGood, MB_OK);
					if (WT_OK == OpenAccount(idxSk, (U16*)pwd0, (U32)len0))
					{
						loginResult = 0;
						PostMessage(hWnd, WM_CLOSE, 0, 0);
					}
					else
					{
						MessageBox(hWnd, txtCannotOpenSK, txtVeryBad, MB_OK);
					}
				}
			}
		}
		break;
	case IDB_CANCEL:
		loginResult = 1;
		PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	default:
		break;
	}

	return r;
}

INT_PTR RegistrationDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = 0;
	switch (msg)
	{
	case WM_COMMAND:
		ret = RCommandHandler(hWnd, msg, wParam, lParam);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		ret = 1;
	case WM_INITDIALOG:
		{
			HICON hIcon = (HICON)LoadImageW(g_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
			if(hIcon)
				SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			HWND hWndCtl;
			SetWindowTextW(hWnd, (LPCWSTR)titleRegistration);
			hWndCtl = GetDlgItem(hWnd, IDB_CANCEL);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnCancel);
			hWndCtl = GetDlgItem(hWnd, IDB_CREATE);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnRegistration);
			hWndCtl = GetDlgItem(hWnd, IDB_IMPORT);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtImportData);
			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_NAME);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticName);
			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_PWD0);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticPWD0);
			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_PWD1);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticPWD0);
		}
		ret = 1;
		break;
	default:
		break;
	}
	return ret;
}

INT_PTR LCommandHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bLoginOnly = TRUE;
	INT_PTR r = 0;
	HWND hWndCtl;
	switch (LOWORD(wParam))
	{
	case IDOK:
		{
			hWndCtl = GetDlgItem(hWnd, IDC_COMBO_NAME);
			if (hWndCtl)
			{
				wchar_t pwd[64 + 1] = { 0 };
				int selectedIndex = SendMessage(hWndCtl, CB_GETCURSEL, 0, 0);
				U32 skIdx = (U32)SendMessage(hWndCtl, CB_GETITEMDATA, (WPARAM)selectedIndex, 0);

				hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD);
				int len = GetWindowTextW(hWndCtl, pwd, 64);
				if (len > 0)
				{
					U32 ret = OpenAccount(skIdx, (U16*)pwd, (U32)len);

					if (WT_OK == ret)
					{
						loginResult = 0;
						PostMessage(hWnd, WM_CLOSE, 0, 0);
					}
					else
					{
						SetWindowTextW(hWndCtl, nullptr);
						MessageBox(hWnd, txtCannotOpenSK, txtVeryBad, MB_OK);
					}
				}
			}
		}
		break;
	case IDB_REGISTER:
		{
			HICON hIcon, hOld;
			bLoginOnly = !bLoginOnly;
			int offset = bLoginOnly ? 280 : 0;
			RECT rcDlg;
			::GetWindowRect(hWnd, &rcDlg);

			SetWindowPos(hWnd, 0, rcDlg.left, rcDlg.top, WidthofLoginDlg - offset, HeightofLoginDlg, SWP_SHOWWINDOW);
			hWndCtl = GetDlgItem(hWnd, IDB_REGISTER);
			if (bLoginOnly)
			{
				if (hWndCtl)
				{
					SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnRegistration);
					hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON16), IMAGE_ICON, 16, 16, 0);
					hOld = (HICON)SendDlgItemMessage(hWnd, IDB_REGISTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
					DestroyIcon(hOld);
					hWndCtl = GetDlgItem(hWnd, IDC_EDIT_NAME);
					if (hWndCtl)
						EnableWindow(hWndCtl, FALSE);
					hWndCtl = GetDlgItem(hWnd, IDB_CREATE);
					if (hWndCtl)
						EnableWindow(hWndCtl, FALSE);
					hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
					if (hWndCtl)
						EnableWindow(hWndCtl, FALSE);
					hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
					if (hWndCtl)
						EnableWindow(hWndCtl, FALSE);
				}
			}
			else
			{
				if (hWndCtl)
				{
					SetWindowTextW(hWndCtl, (LPCWSTR)txtHide);
					hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON18), IMAGE_ICON, 16, 16, 0);
					hOld = (HICON)SendDlgItemMessage(hWnd, IDB_REGISTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
					DestroyIcon(hOld);
					hWndCtl = GetDlgItem(hWnd, IDC_EDIT_NAME);
					if (hWndCtl)
						EnableWindow(hWndCtl, TRUE);
					hWndCtl = GetDlgItem(hWnd, IDB_CREATE);
					if (hWndCtl)
						EnableWindow(hWndCtl, TRUE);
					hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
					if (hWndCtl)
						EnableWindow(hWndCtl, TRUE);
					hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
					if (hWndCtl)
						EnableWindow(hWndCtl, TRUE);
				}
			}
		}
		break;
	case IDB_CREATE:
		{
			int checkIsGood = 1;
			int len, len0, len1;
			wchar_t name[64 + 1] = { 0 };
			wchar_t pwd0[64 + 1] = { 0 };
			wchar_t pwd1[64 + 1] = { 0 };

			hWndCtl = GetDlgItem(hWnd, IDC_EDIT_NAME);
			len = GetWindowTextW(hWndCtl, name, 64);
			if (0 == len)
			{
				MessageBox(hWnd, txtNameEmpty, txtVeryBad, MB_OK);
				checkIsGood = 0;
			}
			else
			{
				hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
				len0 = GetWindowTextW(hWndCtl, pwd0, 64);
				hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
				len1 = GetWindowTextW(hWndCtl, pwd1, 64);
				if (len0 == 0 && len1 == 0)
				{
					MessageBox(hWnd, txtPWDEmpty, txtVeryBad, MB_OK);
					checkIsGood = 0;
				}
				else
				{
					if (len0 != len1)
					{
						MessageBox(hWnd, txtPWDIsNotSame, txtVeryBad, MB_OK);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
						SetWindowTextW(hWndCtl, nullptr);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
						SetWindowTextW(hWndCtl, nullptr);
						checkIsGood = 0;
					}
					else if (wmemcmp(pwd0, pwd1, len0))
					{
						MessageBox(hWnd, txtPWDIsNotSame, txtVeryBad, MB_OK);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
						SetWindowTextW(hWndCtl, nullptr);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
						SetWindowTextW(hWndCtl, nullptr);
						checkIsGood = 0;
					}
				}
			}
			if (checkIsGood)
			{
				U32 idxSk = 0xFFFFFFFF;
				if (WT_OK == CreateNewAccount(name, len, pwd0, len0, &idxSk))
				{
					MessageBox(hWnd, txtCreateGood, _T("Good"), MB_OK);
					if (WT_OK == OpenAccount(idxSk, (U16*)pwd0, (U32)len0))
					{
						loginResult = 0;
						PostMessage(hWnd, WM_CLOSE, 0, 0);
					}
					else
					{
						MessageBox(hWnd, txtCannotOpenSK, txtVeryBad, MB_OK);
					}
				}
			}
		}
		break;
	case IDB_CANCEL:
		loginResult = 1;
		PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	default:
		break;
	}

	return r;
}

static INT_PTR LoginDialogAddUserName(HWND hWnd)
{
	sqlite3* db;
	sqlite3_stmt* stmt = NULL;
	int  rc, id;
	U8* utf8;
	size_t utf8len;
	U32 utf16len, status, rowCount=0;
	U16 utf16[256] = { 0 };

	rc = sqlite3_open16(g_DBPath, &db);
	if (SQLITE_OK != rc)
	{
		sqlite3_close(db);
		return WT_SQLITE_OPEN_ERR;
	}

	rc = sqlite3_prepare_v2(db, (const char*)"SELECT id, nm FROM k WHERE pt=0 AND at<>0 ORDER BY id", -1, &stmt, NULL);
	if (SQLITE_OK == rc)
	{
		rc = sqlite3_step(stmt);
		while (SQLITE_ROW == rc)
		{
			id = sqlite3_column_int(stmt, 0);
			utf8 = (U8*)sqlite3_column_text(stmt, 1);
			if (utf8)
			{
				utf8len = strlen((const char*)utf8);
				status = wt_UTF8ToUTF16(utf8, (U32)utf8len, nullptr, &utf16len);
				if (WT_OK == status)
				{
					wt_UTF8ToUTF16(utf8, (U32)utf8len, utf16, nullptr);
					utf16[utf16len] = 0;
					SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)utf16);
					SendMessage(hWnd, CB_SETITEMDATA, rowCount, (LPARAM)(id));
					rowCount++;
				}
			}
			rc = sqlite3_step(stmt);
		}
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);

	if(rowCount)
		SendMessage(hWnd, CB_SETCURSEL, 0, 0);
	return 0;
}

INT_PTR LoginDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = 0;
	switch (msg)
	{
	case WM_COMMAND:
		ret = LCommandHandler(hWnd, msg, wParam, lParam);
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		ret = 1;
	case WM_INITDIALOG:
		{
			HWND hWndCtl;
			HICON hIcon = (HICON)LoadImageW(g_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
			if (hIcon)
				SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SetWindowTextW(hWnd, (LPCWSTR)titleLogin);
			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_NAME);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticName);
			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_NNAME);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticName);

			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_PWD0);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticPWD0);
			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_NPWD);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticPWD0);

			hWndCtl = GetDlgItem(hWnd, IDB_CANCEL);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnCancel);
			hWndCtl = GetDlgItem(hWnd, IDOK);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)titleLogin);
			hWndCtl = GetDlgItem(hWnd, IDB_CREATE);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnRegistration);

			hWndCtl = GetDlgItem(hWnd, IDB_REGISTER);
			if (hWndCtl)
			{
				SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnRegistration);
				MakeBitmapButton(hWndCtl, IDI_ICON16);
			}

			hWndCtl = GetDlgItem(hWnd, IDC_COMBO_NAME);
			if (hWndCtl)
				LoginDialogAddUserName(hWndCtl);

			hWndCtl = GetDlgItem(hWnd, IDC_EDIT_NAME);
			if (hWndCtl)
				EnableWindow(hWndCtl, FALSE);
			hWndCtl = GetDlgItem(hWnd, IDB_CREATE);
			if (hWndCtl)
				EnableWindow(hWndCtl, FALSE);
			hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
			if (hWndCtl)
				EnableWindow(hWndCtl, FALSE);
			hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
			if (hWndCtl)
				EnableWindow(hWndCtl, FALSE);
		}

		ret = 1;
		break;
	case WM_DRAWITEM:
		return DrawBitmapButton((DRAWITEMSTRUCT*)lParam);
		break;
	default:
		break;
	}
	return ret;
}


//
//	Subclass procedure for an owner-drawn button.
//  All this does is to re-enable double-click behaviour for
//  an owner-drawn button.
//
static LRESULT CALLBACK BBProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC oldproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	TRACKMOUSEEVENT tme = { sizeof(tme) };

	static BOOL mouseOver = FALSE;
	POINT pt;
	RECT  rect;

	switch (msg)
	{
	case WM_LBUTTONDBLCLK:
		msg = WM_LBUTTONDOWN;
		break;

	case WM_MOUSEMOVE:

		if (!mouseOver)
		{
			SetTimer(hwnd, 0, 15, 0);
			mouseOver = FALSE;
		}
		break;

	case WM_TIMER:

		GetCursorPos(&pt);
		ScreenToClient(hwnd, &pt);
		GetClientRect(hwnd, &rect);

		if (PtInRect(&rect, pt))
		{
			if (!mouseOver)
			{
				mouseOver = TRUE;
				InvalidateRect(hwnd, 0, 0);
			}
		}
		else
		{
			mouseOver = FALSE;
			KillTimer(hwnd, 0);
			InvalidateRect(hwnd, 0, 0);
		}

		return 0;

		// Under Win2000 / XP, Windows sends a strange message
		// to dialog controls, whenever the ALT key is pressed
		// for the first time (i.e. to show focus rect / & prefixes etc).
		// msg = 0x0128, wParam = 0x00030003, lParam = 0
	case 0x0128:
		InvalidateRect(hwnd, 0, 0);
		break;
	}

	return CallWindowProc(oldproc, hwnd, msg, wParam, lParam);
}

//
//	Convert the specified button into an owner-drawn button.
//  The button does NOT need owner-draw or icon styles set
//  in the resource editor - this function sets these
//  styles automatically
//
void MakeBitmapButton(HWND hwnd, UINT uIconId)
{
	WNDPROC oldproc;
	DWORD   dwStyle;

	if (GetModuleHandle(_T("uxtheme.dll")))
		g_fThemeApiAvailable = TRUE;
	else
		g_fThemeApiAvailable = FALSE;

	HICON hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(uIconId), IMAGE_ICON, 16, 16, 0);

	// Add on BS_ICON and BS_OWNERDRAW styles
	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	SetWindowLong(hwnd, GWL_STYLE, dwStyle | BS_ICON | BS_OWNERDRAW);

	// Assign icon to the button
	SendMessage(hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	// Subclass (to reenable double-clicks)
	oldproc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)BBProc);

	// Store old procedure
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

	if (g_fThemeApiAvailable)
		SetWindowTheme(hwnd, L"explorer", NULL);
}

#define X_ICON_BORDER	3	// 3 pixels

//
//	Call this function whenever you get a WM_DRAWITEM in the parent dialog..
//
BOOL DrawBitmapButton(DRAWITEMSTRUCT* dis)
{
	RECT rect;			// Drawing rectangle
	POINT pt;

	int ix, iy;			// Icon offset
	int bx, by;			// border sizes
	int sxIcon, syIcon;	// Icon size
	int xoff, yoff;		// 

	TCHAR szText[100];
	int   nTextLen;

	HICON hIcon;
	DWORD dwStyle = GetWindowLong(dis->hwndItem, GWL_STYLE);

	DWORD dwDTflags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
	BOOL  fRightAlign;

	// XP/Vista theme support
	DWORD dwThemeFlags;
	HTHEME hTheme;
	BOOL   fDrawThemed = g_fThemeApiAvailable;

	if (dis->itemState & ODS_NOFOCUSRECT)
		dwDTflags |= DT_HIDEPREFIX;

	fRightAlign = (dwStyle & BS_RIGHT) ? TRUE : FALSE;
	fRightAlign = TRUE;

	// do the theme thing
	hTheme = _OpenThemeData(dis->hwndItem, L"Button");
	switch (dis->itemAction)
	{
		// We need to redraw the whole button, no
		// matter what DRAWITEM event we receive..
	case ODA_FOCUS:
	case ODA_SELECT:
	case ODA_DRAWENTIRE:

		// Retrieve button text
		GetWindowText(dis->hwndItem, szText, sizeof(szText) / sizeof(TCHAR));

		nTextLen = lstrlen(szText);

		// Retrieve button icon
		hIcon = (HICON)SendMessage(dis->hwndItem, BM_GETIMAGE, IMAGE_ICON, 0);

		// Find icon dimensions
		sxIcon = 16;
		syIcon = 16;

		CopyRect(&rect, &dis->rcItem);
		GetCursorPos(&pt);
		ScreenToClient(dis->hwndItem, &pt);

		if (PtInRect(&rect, pt))
			dis->itemState |= ODS_HOTLIGHT;

		// border dimensions
		bx = 2;
		by = 2;

		// icon offsets
		if (nTextLen == 0)
		{
			// center the image if no text
			ix = (rect.right - rect.left - sxIcon) / 2;
		}
		else
		{
			if (fRightAlign)
				ix = rect.right - bx - X_ICON_BORDER - sxIcon;
			else
				ix = rect.left + bx + X_ICON_BORDER;
		}

		// center image vertically
		iy = (rect.bottom - rect.top - syIcon) / 2;

		InflateRect(&rect, -5, -5);

		// Draw a single-line black border around the button
		if (hTheme == NULL && (dis->itemState & (ODS_FOCUS | ODS_DEFAULT)))
		{
			FrameRect(dis->hDC, &dis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
			InflateRect(&dis->rcItem, -1, -1);
		}

		if (dis->itemState & ODS_FOCUS)
			dwThemeFlags = PBS_DEFAULTED;
		if (dis->itemState & ODS_DISABLED)
			dwThemeFlags = PBS_DISABLED;
		else if (dis->itemState & ODS_SELECTED)
			dwThemeFlags = PBS_PRESSED;
		else if (dis->itemState & ODS_HOTLIGHT)
			dwThemeFlags = PBS_HOT;
		else if (dis->itemState & ODS_DEFAULT)
			dwThemeFlags = PBS_DEFAULTED;
		else
			dwThemeFlags = PBS_NORMAL;

		// Button is DOWN
		if (dis->itemState & ODS_SELECTED)
		{
			// Draw a button
			if (hTheme != NULL)
				DrawThemeBackground(hTheme, dis->hDC, BP_PUSHBUTTON, dwThemeFlags, &dis->rcItem, 0);
			else
				DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_PUSHED | DFCS_FLAT);

			// Offset contents to make it look "pressed"
			if (hTheme == NULL)
			{
				OffsetRect(&rect, 1, 1);
				xoff = yoff = 1;
			}
			else
				xoff = yoff = 0;
		}
		// Button is UP
		else
		{
			//
			if (hTheme != NULL)
				DrawThemeBackground(hTheme, dis->hDC, BP_PUSHBUTTON, dwThemeFlags, &dis->rcItem, 0);
			else
				DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH);

			xoff = yoff = 0;
		}

		// Draw the icon
		DrawIconEx(dis->hDC, ix + xoff, iy + yoff, hIcon, sxIcon, syIcon, 0, 0, DI_NORMAL);

		// Adjust position of window text 
		if (fRightAlign)
		{
			rect.left += bx + X_ICON_BORDER;
			rect.right -= sxIcon + bx + X_ICON_BORDER;
		}
		else
		{
			rect.right -= bx + X_ICON_BORDER;
			rect.left += sxIcon + bx + X_ICON_BORDER;
		}

		// Draw the text
		OffsetRect(&rect, 0, -1);
		SetBkMode(dis->hDC, TRANSPARENT);
		DrawText(dis->hDC, szText, -1, &rect, dwDTflags);
		OffsetRect(&rect, 0, 1);

		// Draw the focus rectangle (only if text present)
		if ((dis->itemState & ODS_FOCUS) && nTextLen > 0)
		{
			if (!(dis->itemState & ODS_NOFOCUSRECT))
			{
				// Get a "fresh" copy of the button rectangle
				CopyRect(&rect, &dis->rcItem);

				if (fRightAlign)
					rect.right -= sxIcon + bx;
				else
					rect.left += sxIcon + bx + 2;

				InflateRect(&rect, -3, -3);
				DrawFocusRect(dis->hDC, &rect);
			}
		}

		break;
	}

	_CloseThemeData(hTheme);

	return TRUE;
}


static const wchar_t txtSearchAdd[] = { 0x6dfb, 0x52a0, 0x597d, 0x53cb, 0 };
static const wchar_t txtSearch[] = { 0x641c, 0 };
static const wchar_t txtAdd[] = { 0x6dfb, 0x52a0, 0 };
static const wchar_t txtCancel[] = { 0x53d6, 0x6d88, 0 };

INT_PTR CALLBACK SearchDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hWndCtl;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowTextW(hWnd, (LPCWSTR)txtSearchAdd);
		hWndCtl = GetDlgItem(hWnd, IDC_BTN_SEARCH);
		if (hWndCtl)
			SetWindowTextW(hWndCtl, (LPCWSTR)txtSearch);
		hWndCtl = GetDlgItem(hWnd, IDOK);
		if (hWndCtl)
			SetWindowTextW(hWndCtl, (LPCWSTR)txtAdd);
		hWndCtl = GetDlgItem(hWnd, IDCANCEL);
		if (hWndCtl)
			SetWindowTextW(hWndCtl, (LPCWSTR)txtCancel);
		hWndCtl = GetDlgItem(hWnd, IDC_PROGRESS_SEARCH);
		if (hWndCtl)
			ShowWindow(hWndCtl, SW_HIDE);
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
