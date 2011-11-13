// Runner.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Runner.h"

// Global variables
HINSTANCE hAppInstance;         // Application instance
HICON hIcon32 = NULL;           // Application icon 32x32
HICON hIcon16 = NULL;           // Application icon 16x16
HFONT hUnderlinedFont = NULL;   // Underlined font for drawing hyperlinks

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutProc(HWND, UINT, WPARAM, LPARAM);


// Main function
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	InitCommonControls();

	hAppInstance = hInstance;
	HWND hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, MainDlgProc);

	if (!RegisterHotKey(hDlg, 0, MOD_WIN, 'R'))
	{
		const size_t MAX_BUF = 32768;
		WCHAR msg[MAX_BUF];
		swprintf_s(msg, L"Не удалось зарегистрировать Win+R (код ошибки %d)", GetLastError());
		MessageBox(NULL, msg, L"Runner", MB_ICONERROR | MB_OK);
		return 1;
	}

	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(hDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	UnregisterHotKey(NULL, 0);
	return 0;
}

// Message handler for the main dialog
INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	const size_t MAX_BUF = 32768;
	WCHAR InputStr[MAX_BUF];

	HWND hComboBox = GetDlgItem(hDlg, IDC_CMDLINE);
	HKEY hKey = NULL;
	switch (message)
	{
	case WM_INITDIALOG:
		{
			////////////////////////////////////////////////////////////////////////////////
			// Initializing the dialog

			HMENU hSysMenu;

			// Load and set window icon
			hIcon16 = (HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_RUNNER), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			hIcon32 = (HICON)LoadImage(hAppInstance, MAKEINTRESOURCE(IDI_RUNNER), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon16);
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon32);

			// Add "About" command to the system menu
			hSysMenu = GetSystemMenu(hDlg, FALSE);
			AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hSysMenu, MF_STRING, IDM_ABOUTBOX, L"О программе…");

			return (INT_PTR)TRUE;
		}

	case WM_COMMAND:
		{
			// Command processing
			if (LOWORD(wParam) == IDOK)
			{
				////////////////////////////////////////////////////////////////////////////////
				// OK is pressed: execute the command

				ComboBox_GetText(hComboBox, InputStr, MAX_BUF);
				// Hide the dialog (imitating the native Run dialog behaviour)
				ShowWindow(hDlg, SW_HIDE);
				// Search for any space delimiter: if found, split the command into executable and arguments
				WCHAR* args;
				if (InputStr[0] == L'"')
				{
					args = wcschr(InputStr + 1, L'"');
					if (args != NULL)
					{
						args = wcschr(args, L' ');
						if (args != NULL)
							*args++ = L'\0';
					}
				}
				else
				{
					args = wcschr(InputStr, L' ');
					if (args != NULL)
						*args++ = L'\0';
				}
				// Run the command with current or elevated privilages, depending on the "As Admin" checkbox state
				int res = (int)ShellExecute(NULL, (Button_GetCheck(GetDlgItem(hDlg, IDC_AS_ADMIN)) == BST_CHECKED) ? L"runas" : NULL, InputStr, args, NULL, SW_SHOW);
				// Combine splitted command line back together (for reporting or saving into the registry)
				if (args != NULL)
					*(args - 1) = L' ';
				if (res >= 32)
				{
					// Command started successfully, try to update the history in the registry
					WCHAR mru[32];
					int pos = 0;
					int idx = ComboBox_FindStringExact(hComboBox, -1, InputStr);
					if (idx == CB_ERR)
					{
						// This is a new command
						int mru_num = ComboBox_GetCount(hComboBox);
						// Choose the letter to assign new command to: first unused (if any) or the last used (if all 26 are occupied)
						mru[pos++] = ((mru_num < 26) ? (WCHAR)(L'a' + mru_num) : (WCHAR)ComboBox_GetItemData(hComboBox, 25));
						// Fill in the rest of commands
						for (int i = 0; i < min(mru_num, 25); ++i)
							mru[pos++] = (WCHAR)ComboBox_GetItemData(hComboBox, i);
						mru[pos++] = L'\0';
						try
						{
							// Store the data into the registry
							if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
								throw 1;
							if (RegSetValueEx(hKey, L"MRUList", NULL, REG_SZ, (BYTE*)mru, pos * sizeof(WCHAR)) != ERROR_SUCCESS)
								throw 1;
							WCHAR name[2] = { mru[0], L'\0' };
							size_t len = wcslen(InputStr);
							wcscpy_s(InputStr + len, MAX_BUF - len, L"\\1");
							len += 2;
							if (RegSetValueEx(hKey, name, NULL, REG_SZ, (BYTE*)InputStr, (DWORD)len * sizeof(WCHAR)) != ERROR_SUCCESS)
								throw 1;
						}
						catch (...)
						{
						}
						if (hKey != NULL)
							RegCloseKey(hKey);
					}
					else
					{
						// This is a old command, taken from history
						// Put it to the first position
						mru[pos++] =(WCHAR)ComboBox_GetItemData(hComboBox, idx);
						// Fill in the rest of commands
						for (int i = 0; i < ComboBox_GetCount(hComboBox); ++i)
							if (i != idx)
								mru[pos++] = (WCHAR)ComboBox_GetItemData(hComboBox, i);
						mru[pos++] = L'\0';
						try
						{
							// Store the data into the registry
							if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
								throw 1;
							if (RegSetValueEx(hKey, L"MRUList", NULL, REG_SZ, (BYTE*)mru, pos * sizeof(WCHAR)) != ERROR_SUCCESS)
								throw 1;
						}
						catch (...)
						{
						}
						if (hKey != NULL)
							RegCloseKey(hKey);
					}
				}
				else
				{
					// Some error occurred - show the message and restore the dialog's visibility
					WCHAR msg[MAX_BUF];
					LPWSTR sysmsg;
					FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, res, 0, (LPWSTR)&sysmsg, 0, NULL);
					swprintf_s(msg, L"Не удалось выполнить команду:\n\n%s\n\n%s", InputStr, sysmsg);
					LocalFree(sysmsg);
					MessageBox(NULL, msg, L"Runner", MB_ICONERROR | MB_OK);
					ShowWindow(hDlg, SW_SHOW);
					SetFocus(hComboBox);
				}
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDCANCEL)
			{
				////////////////////////////////////////////////////////////////////////////////
				// Cancel is pressed: hide the dialog

				ShowWindow(hDlg, SW_HIDE);
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDC_AS_ADMIN)
			{
				////////////////////////////////////////////////////////////////////////////////
				// As Admin is (un)checked: set/clear the shield icon on the OK button

				Button_SetElevationRequiredState(GetDlgItem(hDlg, IDOK), Button_GetCheck(GetDlgItem(hDlg, IDC_AS_ADMIN)) == BST_CHECKED);
			}
			else if (LOWORD(wParam) == IDC_BROWSE)
			{
				////////////////////////////////////////////////////////////////////////////////
				// Browse is pressed: show the Open File dialog

				// Prepare for possible wrapping into quotes
				InputStr[0] = L'"';
				InputStr[1] = L'\0';

				OPENFILENAME ofn;
				memset(&ofn, 0, sizeof(OPENFILENAME));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter = L"Программы (*.exe;*.pif;*.com;*.bat;*.cmd)\0*.exe;*.pif;*.com;*.bat;*.cmd;*.lnk\0Все файлы (*.*)\0*.*\0\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = InputStr + 1;   // Reserve 2 characters for possible quotation
				ofn.nMaxFile = MAX_BUF - 2;
				ofn.lpstrTitle = L"Обзор";
				ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NODEREFERENCELINKS;
				if (GetOpenFileName(&ofn))
				{
					if (wcschr(ofn.lpstrFile, L' ') != NULL)
					{
						// If path contains space complete the quotation wrapping
						size_t len = wcslen(InputStr);
						InputStr[len] = L'"';
						InputStr[len + 1] = L'\0';
						ofn.lpstrFile = InputStr;
					}
					// Write the resultant path to the combobox
					ComboBox_SetText(hComboBox, ofn.lpstrFile);
				}
				return (INT_PTR)TRUE;
			}
			break;
		}
	case WM_SYSCOMMAND:
		{
			// System command processing
			if ((wParam & 0xFFF0) == IDM_ABOUTBOX)
				DialogBox(hAppInstance,  MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, AboutProc);
			else
				DefWindowProc(hDlg, message, wParam, lParam);
			break;
		}

	case WM_HOTKEY:
		{
			if (IsWindowVisible(hDlg))
				SetForegroundWindow(hDlg);
			else
			{
				// Reinitialize the dialog controls
				Button_SetCheck(GetDlgItem(hDlg, IDC_AS_ADMIN), FALSE);
				Button_SetElevationRequiredState(GetDlgItem(hDlg, IDOK), FALSE);
				ComboBox_ResetContent(hComboBox);

				/*
					Commands are stored in registry as follows:
					1) MRUList contains sequence of letters, the order defines the order of command lines.
					2) Each letter defines a parameter with this letter as name; value is the command line with \1 appended to the end.
					3) When an older command is run, it's position is changed to first (the top of the list); MRUList is updated accordingly.
					4) When a new command is run:
					  a) if a vacant letter is present, it is allocated for the new command and added to the first position of MRUList;
					  b) if all 26 letters are present, the last from MRUList is reused: the new command replaces the older contents,
						 the letter is shifted to the first position.
				*/
				// Read the list of MRU command lines from the registry and fill in the drop-down list of history
				try
				{
					// First, read the MRUList
					if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
						throw 1;
					WCHAR mru[32];
					DWORD mru_len = 32 * sizeof(WCHAR);
					if (RegQueryValueEx(hKey, L"MRUList", NULL, NULL, (BYTE*)mru, &mru_len) != ERROR_SUCCESS)
						throw 1;
					mru_len /= sizeof(WCHAR);
					// Now reading data for each letter and append the command line to the combobox
					WCHAR name[2];
					name[1] = L'\0';
					for (DWORD i = 0; i < mru_len; ++i)
					{
						name[0] = mru[i];
						DWORD BufSz = MAX_BUF * sizeof(WCHAR);
						if (RegQueryValueEx(hKey, name, NULL, NULL, (BYTE*)InputStr, &BufSz) != ERROR_SUCCESS)
							throw 1;
						size_t len = wcslen(InputStr);
						// Remove the \1 suffix
						if ((InputStr[len - 2] == L'\\') && (InputStr[len - 1] == L'1'))
							InputStr[len - 2] = L'\0';
						int idx = ComboBox_AddString(hComboBox, (LPARAM)InputStr);
						// Remember the letter this command is stored under
						ComboBox_SetItemData(hComboBox, idx, mru[i]);
						if (i == 0)
							ComboBox_SetText(hComboBox, InputStr);
					}
				}
				catch (...)
				{
				}
				if (hKey != NULL)
					RegCloseKey(hKey);
				// Set focus to the command combobox
				SetFocus(hComboBox);
				ShowWindow(hDlg, SW_SHOW);
			}
			break;
		}

	case WM_DESTROY:
		{
			DestroyIcon(hIcon32);
			DestroyIcon(hIcon16);
			if (hUnderlinedFont != NULL)
				DeleteObject(hUnderlinedFont);
			break;
		}
	}

	return FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			// Dialog initialization
			HWND hHomepage = GetDlgItem(hDlg, IDC_HOMEPAGE);
			HWND hEmail = GetDlgItem(hDlg, IDC_EMAIL);
			// Set the "hand" cursor over the hyperlink controls
			HCURSOR hHandCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(OCR_HAND), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
			SetClassLongPtr(hHomepage, GCLP_HCURSOR, (LPARAM)hHandCursor);
			SetClassLongPtr(hEmail, GCLP_HCURSOR, (LPARAM)hHandCursor);
			// Lazy initialization of the hyperlinks-specific underlined font
			if (hUnderlinedFont == NULL)
			{
				LOGFONT lf;
				hUnderlinedFont = GetWindowFont(hHomepage);
				GetObject(hUnderlinedFont, sizeof(LOGFONT), &lf);
				lf.lfUnderline = 1;
				hUnderlinedFont = CreateFontIndirect(&lf);
			}
			// Set the font to the hyperlink controls
			SetWindowFont(hHomepage, hUnderlinedFont, FALSE);
			SetWindowFont(hEmail, hUnderlinedFont, FALSE);
			return (INT_PTR)TRUE;
		}

	case WM_CTLCOLORSTATIC:
		{
			if (((HWND)lParam == GetDlgItem(hDlg, IDC_HOMEPAGE)) || ((HWND)lParam == GetDlgItem(hDlg, IDC_EMAIL)))
			{
				// Draw hyperlinks using blue color
				SetTextColor((HDC)wParam, RGB(0, 0, 255));
				SetBkMode((HDC)wParam, TRANSPARENT);
				return (INT_PTR)GetStockObject(NULL_BRUSH);
			}
			break;
		}

	case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDC_HOMEPAGE)
			{
				ShellExecute(hDlg, NULL, L"http://flint-inc.ru/", NULL, NULL, SW_SHOW);
			}
			else if (LOWORD(wParam) == IDC_EMAIL)
			{
				ShellExecute(hDlg, NULL, L"mailto:support@flint-inc.ru", NULL, NULL, SW_SHOW);
			}
			break;
		}
	}
	return (INT_PTR)FALSE;
}
