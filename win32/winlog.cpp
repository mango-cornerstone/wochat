#include "pch.h"
#include "resource.h"
#include "wochat.h"
#include "winlog.h"

static wchar_t titleRegistration[] = {0x6ce8, 0x518c, 0x8d26,0x53f7, 0 };
static wchar_t titleLogin[] = { 0x767b, 0x5f55, 0};
static wchar_t txtBtnCancel[] = { 0x53d6, 0x6d88, 0 };
static wchar_t txtBtnRegistration[] = { 0x6ce8, 0x518c, 0 };
static wchar_t txtStaticName[] = { 0x7528, 0x6237, 0x540d, 0xff1a, 0 };
static wchar_t txtStaticPWD0[] = { 0x5bc6, 0x7801, 0xff1a, 0 };

static wchar_t txtCreateGood[] = { 0x8d26, 0x53f7, 0x521b, 0x5efa, 0x6210, 0x529f, 0x0021, 0 };
static wchar_t txtCreateBad[] = { 0x8d26, 0x53f7, 0x521b, 0x5efa, 0x5931, 0x8d25, 0x0021, 0x0020, 0x4f60, 0x53ef, 0x4ee5, 0x91cd, 0x65b0, 0x521b, 0x5efa, 0x3002, 0x003a, 0x002d, 0x0029, 0 };
static wchar_t txtPWDIsNotSame[] = {
0x4e24, 0x6b21, 0x8f93, 0x5165, 0x7684, 0x5bc6, 0x7801, 0x4e0d, 0x76f8, 0x540c, 0xff0c, 0x8bf7, 0x91cd, 0x65b0, 0x8bbe, 0x7f6e, 0x5bc6, 0x7801, 0xff01, 0 };

static wchar_t txtNameEmpty[] = { 0x7528, 0x6237, 0x540d, 0x4e0d, 0x80fd, 0x4e3a, 0x7a7a, 0xff01, 0 };
static wchar_t txtPWDEmpty[] = { 0x5bc6, 0x7801, 0x4e0d, 0x80fd, 0x4e3a, 0x7a7a, 0xff01, 0 };

static wchar_t txtN1[] = { 0x6625, 0x7533, 0x95e8, 0x4e0b, 0x4e00, 0x5200, 0x5ba2, 0 };
static wchar_t txtN2[] = { 0x57ce, 0x5357, 0x5c0f, 0x675c, 0 };


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


int DoWoChatLoginOrRegistration(HINSTANCE hInstance)
{
	U16 skCount = GetSecretKeyNumber();
	
	return skCount ? DoWoChatLogin(hInstance) : DoWoChatRegistration(hInstance);
}


INT_PTR CommandHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
				MessageBox(hWnd, txtNameEmpty, _T("Bad"), MB_OK);
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
					MessageBox(hWnd, txtPWDEmpty, _T("Bad"), MB_OK);
					checkIsGood = 0;
				}
				else
				{
					if (len0 != len1)
					{
						MessageBox(hWnd, txtPWDIsNotSame, _T("Bad"), MB_OK);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD0);
						SetWindowTextW(hWndCtl, nullptr);
						hWndCtl = GetDlgItem(hWnd, IDC_EDIT_PASSWORD1);
						SetWindowTextW(hWndCtl, nullptr);
						checkIsGood = 0;
					}
					else if (wmemcmp(pwd0, pwd1, len0))
					{
						MessageBox(hWnd, txtPWDIsNotSame, _T("Bad"), MB_OK);
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
				if (0 == CreateNewScretKey(name, len, pwd0, len0))
				{
					MessageBox(hWnd, txtCreateGood, _T("Good"), MB_OK);
					loginResult = 0;
					PostMessage(hWnd, WM_CLOSE, 0, 0);
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
		ret = CommandHandler(hWnd, msg, wParam, lParam);
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

INT_PTR LoginDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = 0;
	switch (msg)
	{
	case WM_COMMAND:
		//ret = CommandHandler(hWnd, msg, wParam, lParam);
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
			hWndCtl = GetDlgItem(hWnd, IDC_STATIC_PWD0);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtStaticPWD0);
			hWndCtl = GetDlgItem(hWnd, IDB_CANCEL);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnCancel);
			hWndCtl = GetDlgItem(hWnd, IDOK);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)titleLogin);
			hWndCtl = GetDlgItem(hWnd, IDB_REGISTER);
			if (hWndCtl)
				SetWindowTextW(hWndCtl, (LPCWSTR)txtBtnRegistration);

			hWndCtl = GetDlgItem(hWnd, IDC_COMBO_NAME);
			if (hWndCtl)
			{
				SendMessage(hWndCtl, CB_ADDSTRING, 0, (LPARAM)txtN1);
				SendMessage(hWndCtl, CB_ADDSTRING, 0, (LPARAM)txtN2);
				SendMessage(hWndCtl, CB_SETCURSEL, 0, 0);
			}
		}
		ret = 1;
		break;
	default:
		break;
	}
	return ret;
}

