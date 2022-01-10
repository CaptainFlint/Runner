#pragma once

#include "stdafx.h"

typedef std::map<DWORD, LPCWSTR> TranslationMap;

class Configuration
{
public:
	WCHAR m_Language[MAX_PATH];
	UINT m_ActivateKey;
	UINT m_ActivateModifiers;

	inline Configuration()
	{
		wcscpy_s(m_Language, L"english");
		m_ActivateKey = 'R';
		m_ActivateModifiers = MOD_WIN;
	}
};

extern TranslationMap translationMap;
extern Configuration config;

void ReadConfig();

LPCWSTR _(DWORD id);

void TranslateDialog(HWND hDlg, const DWORD* ids);
