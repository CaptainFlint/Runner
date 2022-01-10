#include "stdafx.h"
#include "Runner.h"
#include "Config.h"

TranslationMap translationMap;
Configuration config;

const size_t BUF_SZ = MAX_PATH;

bool ParseLanguageFile(const WCHAR* buf, size_t buf_len)
{
	size_t pos = (((buf_len > 0) && (buf[0] == 0xfeff)) ? 1 : 0);
	enum ParserFSM
	{
		PARSER_NEWLINE,
		PARSER_COMMENT,
		PARSER_NUMBER,
		PARSER_STRING
	};
	ParserFSM mode = PARSER_NEWLINE;
	DWORD code = 0;
	size_t str_begin = 0;
	while (pos < buf_len)
	{
		switch (mode)
		{
		case PARSER_NEWLINE:
			if ((buf[pos] >= L'0') && (buf[pos] <= L'9'))
			{
				mode = PARSER_NUMBER;
				code = buf[pos] - L'0';
			}
			else
				mode = PARSER_COMMENT;
			break;
		case PARSER_COMMENT:
			if (buf[pos] == L'\x0a')
				mode = PARSER_NEWLINE;
			break;
		case PARSER_NUMBER:
			if ((buf[pos] >= L'0') && (buf[pos] <= L'9'))
				code = code * 10 + (buf[pos] - L'0');
			else if (buf[pos] == L'=')
			{
				str_begin = pos + 1;
				mode = PARSER_STRING;
			}
			else
				return false;
			break;
		case PARSER_STRING:
			if (buf[pos] == L'\x0d')
			{
				size_t len = pos - str_begin + 1;
				WCHAR* str = new WCHAR[len];
				wcsncpy_s(str, len, buf + str_begin, pos - str_begin);
				for (size_t i = 0; i < len; ++i)
				{
					if ((str[i] == L'\\') && (i < len - 1) && (str[i + 1] == L'n'))
					{
						str[i] = L'\x0d';
						str[i + 1] = L'\x0a';
					}
				}
				translationMap[code] = str;
				mode = PARSER_COMMENT;
			}
			break;
		default:
			return false;
		}
		++pos;
	}
	return true;
}

bool ReadLanguageFile(const WCHAR* filePath)
{
	WCHAR msgBuf[BUF_SZ];
	WCHAR* langData = NULL;

	// First read and parse the built-in language resource, to fill in all the string IDs
	HRSRC hres = FindResource(NULL, MAKEINTRESOURCE(IDR_DEFAULT_LANGUAGE), RT_RCDATA);
	if (hres == NULL)
		return false;
	HGLOBAL hg = LoadResource(NULL, hres);
	if (hg == NULL)
		return false;
	langData = (WCHAR*)LockResource(hg);
	if (langData == NULL)
		return false;
	if (!ParseLanguageFile(langData, SizeofResource(NULL, hres) / sizeof(WCHAR)))
	{
		MessageBox(NULL, L"Internal language file is corrupted!", AppName, MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	// Then check if the external language file is specified. If it is, read it, updating the translation map
	if ((filePath != NULL) && (filePath[0] != L'\0'))
	{
		HANDLE f = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
		if (f != INVALID_HANDLE_VALUE)
		{
			DWORD langDataLen = SetFilePointer(f, 0, NULL, FILE_END);
			if (langDataLen == INVALID_SET_FILE_POINTER)
			{
				CloseHandle(f);
				MessageBox(NULL, L"Cannot read language file: seek error!", AppName, MB_ICONEXCLAMATION | MB_OK);
				return false;
			}
			if (SetFilePointer(f, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			{
				CloseHandle(f);
				MessageBox(NULL, L"Cannot read language file: seek error!", AppName, MB_ICONEXCLAMATION | MB_OK);
				return false;
			}
			langData = new WCHAR[langDataLen / sizeof(WCHAR)];
			DWORD dw;
			BOOL res1 = ReadFile(f, langData, langDataLen, &dw, NULL);
			if (!res1 || (dw != langDataLen))
			{
				delete[] langData;
				CloseHandle(f);
				swprintf_s(msgBuf, BUF_SZ, L"Cannot read language file! Error code: %u", GetLastError());
				MessageBox(NULL, msgBuf, AppName, MB_ICONEXCLAMATION | MB_OK);
				return false;
			}
			CloseHandle(f);
			bool res2 = ParseLanguageFile(langData, langDataLen);
			if (!res2)
				MessageBox(NULL, L"Language file is corrupted! Using internal English.", AppName, MB_ICONEXCLAMATION | MB_OK);
			delete[] langData;
			if (!res2)
				return false;
		}
		else
		{
			swprintf_s(msgBuf, BUF_SZ, L"Cannot open language file! Error code: %u", GetLastError());
			MessageBox(NULL, msgBuf, AppName, MB_ICONEXCLAMATION | MB_OK);
		}
	}

	return true;
}

void ReadConfig()
{
	WCHAR msgBuf[BUF_SZ];

	WCHAR* workDir = new WCHAR[BUF_SZ];
	WCHAR* iniPath = new WCHAR[BUF_SZ];
	if ((workDir == NULL) || (msgBuf == NULL) || (iniPath == NULL))
	{
		MessageBox(NULL, L"Memory allocation error!", AppName, MB_ICONERROR | MB_OK);
		return;
	}

	size_t pathLen = GetModuleFileName(NULL, iniPath, BUF_SZ);
	if (pathLen == 0)
	{
		swprintf_s(msgBuf, BUF_SZ, L"Failed to find myself! Error code: %u", GetLastError());
		MessageBox(NULL, msgBuf, AppName, MB_ICONERROR | MB_OK);
		return;
	}

	WCHAR* idxSlash;
	WCHAR* idxDot;
	idxSlash = wcsrchr(iniPath, L'\\');
	idxDot = wcsrchr(iniPath, L'.');
	if ((idxSlash == NULL) || (idxDot == NULL) || (idxDot < idxSlash))
	{
		MessageBox(NULL, L"Invalid path to self!", AppName, MB_ICONERROR | MB_OK);
		return;
	}
	if (BUF_SZ - (idxDot - iniPath) < 4)
	{
		MessageBox(NULL, L"INI path buffer is too small!", AppName, MB_ICONERROR | MB_OK);
		return;
	}
	wcsncpy_s(workDir, BUF_SZ, iniPath, idxSlash - iniPath);
	wcscpy_s(idxDot, BUF_SZ - (idxDot - iniPath), L".ini");

	// INI file path constructed, read it
	if (GetFileAttributes(iniPath) != INVALID_FILE_ATTRIBUTES)
	{
		GetPrivateProfileString(L"Configuration", L"Language", L"english", config.m_Language, MAX_PATH, iniPath);
		//GetPrivateProfileString(L"Configuration", L"ActivateHotkey", L"Win+R", msgBuf, MAX_PATH, iniPath);
	}

	// Read the translation
	swprintf_s(msgBuf, BUF_SZ, L"%s\\Language\\%s.lng", workDir, config.m_Language);
	ReadLanguageFile(msgBuf);
		
	delete[] workDir;
	delete[] iniPath;
}

LPCWSTR _(DWORD id)
{
	auto item = translationMap.find(id);
	if (item == translationMap.end())
		return NULL;
	else
		return item->second;
}

void TranslateDialog(HWND hDlg, const DWORD* ids)
{
	for (size_t i = 0; ; i += 2)
	{
		if (ids[i] == 0)
		{
			SetWindowText(hDlg, _(ids[i + 1]));
			break;
		}
		else
			SetDlgItemText(hDlg, ids[i], _(ids[i + 1]));
	}
}
