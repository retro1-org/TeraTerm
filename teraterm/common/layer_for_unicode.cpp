/*
 * Copyright (C) 2019- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * W to A Wrapper
 *
 * API����W�ł̓��� '_' ��t�������̂��g�p����
 */

#include <windows.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "codeconv.h"
#include "compat_win.h"
#include "dllutil.h"
#include "ttlib.h"		// for IsWindowsNTKernel()

#include "layer_for_unicode.h"

class Initializer {
public:
	Initializer() {
		DLLInit();
		WinCompatInit();
	}
	~Initializer() {
		DLLExit();
	}
};

static Initializer initializer;

BOOL WINAPI _SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString)
{
	if (pSetDlgItemTextW != NULL) {
		return pSetDlgItemTextW(hDlg, nIDDlgItem, lpString);
	}

	char *strA = ToCharW(lpString);
	BOOL retval = SetDlgItemTextA(hDlg, nIDDlgItem, strA);
	free(strA);
	return retval;
}

UINT WINAPI _DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
	if (pDragQueryFileW != NULL) {
		return pDragQueryFileW(hDrop, iFile, lpszFile, cch);
	}

	UINT retval;
	if (iFile == 0xffffffff) {
		// �t�@�C�����₢���킹
		retval = DragQueryFileA(hDrop, iFile, NULL, 0);
	}
	else if (lpszFile == NULL) {
		// �t�@�C�����̕������₢���킹
		char FileNameA[MAX_PATH];
		retval = DragQueryFileA(hDrop, iFile, FileNameA, MAX_PATH);
		if (retval != 0) {
			wchar_t *FileNameW = ToWcharA(FileNameA);
			retval = (UINT)(wcslen(FileNameW) + 1);
			free(FileNameW);
		}
	}
	else {
		// �t�@�C�����擾
		char FileNameA[MAX_PATH];
		retval = DragQueryFileA(hDrop, iFile, FileNameA, MAX_PATH);
		if (retval != 0) {
			wchar_t *FileNameW = ToWcharA(FileNameA);
			wcscpy_s(lpszFile, cch, FileNameW);
			free(FileNameW);
		}
	}
	return retval;
}

DWORD WINAPI _GetFileAttributesW(LPCWSTR lpFileName)
{
	if (pGetFileAttributesW != NULL) {
		return pGetFileAttributesW(lpFileName);
	}

	char *FileNameA;
	if (lpFileName == NULL) {
		FileNameA = NULL;
	} else {
		FileNameA = ToCharW(lpFileName);
	}
	const DWORD attr = GetFileAttributesA(FileNameA);
	free(FileNameA);
	return attr;
}

/**
 * hWnd �ɐݒ肳��Ă��镶������擾
 *
 * @param[in]		hWnd
 * @param[in,out]	lenW	������(L'\0'���܂܂Ȃ�)
 * @return			������
 */
static wchar_t *SendMessageAFromW_WM_GETTEXT(HWND hWnd, size_t *lenW)
{
	// lenA = excluding the terminating null character.
	size_t lenA = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
	char *strA = (char *)malloc(lenA + 1);
	if (strA == NULL) {
		*lenW = 0;
		return NULL;
	}
	lenA = GetWindowTextA(hWnd, strA, (int)(lenA + 1));
	strA[lenA] = '\0';
	wchar_t *strW = ToWcharA(strA);
	free(strA);
	if (strW == NULL) {
		*lenW = 0;
		return NULL;
	}
	*lenW = wcslen(strW);
	return strW;
}

/**
 * hWnd(ListBox) �ɐݒ肳��Ă��镶������擾
 *
 * @param[in]		hWnd
 * @param[in]		wParam	���ڔԍ�(0�`)
 * @param[in,out]	lenW	������(L'\0'���܂܂Ȃ�)
 * @return			������
 */
static wchar_t *SendMessageAFromW_LB_GETTEXT(HWND hWnd, WPARAM wParam, size_t *lenW)
{
	// lenA = excluding the terminating null character.
	size_t lenA = SendMessageA(hWnd, LB_GETTEXTLEN, wParam, 0);
	char *strA = (char *)malloc(lenA + 1);
	if (strA == NULL) {
		*lenW = 0;
		return NULL;
	}
	lenA = SendMessageA(hWnd, LB_GETTEXT, wParam, (LPARAM)strA);
	wchar_t *strW = ToWcharA(strA);
	free(strA);
	if (strW == NULL) {
		*lenW = 0;
		return NULL;
	}
	*lenW = wcslen(strW);
	return strW;
}

int WINAPI _GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
	if (pGetWindowTextW != NULL) {
		return pGetWindowTextW(hWnd, lpString, nMaxCount);
	}

	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	wchar_t *dest_ptr = (wchar_t *)lpString;
	size_t dest_len = (size_t)nMaxCount;
	wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
	free(strW);
	return (int)(dest_len - 1);
}

int WINAPI _GetWindowTextLengthW(HWND hWnd)
{
	if (pGetWindowTextLengthW != NULL) {
		return pGetWindowTextLengthW(hWnd);
	}

	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	free(strW);
	return (int)(lenW - 1);
}

static LRESULT SendMessageAFromW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT retval;
	switch(Msg) {
	case CB_ADDSTRING:
	case CB_INSERTSTRING:
	case LB_ADDSTRING:
	case LB_INSERTSTRING: {
		char *strA = ToCharW((wchar_t *)lParam);
		retval = SendMessageA(hWnd, Msg, wParam, (LPARAM)strA);
		free(strA);
		return retval;
	}
	case WM_GETTEXTLENGTH:
	case LB_GETTEXTLEN: {
		size_t lenW;
		wchar_t *strW;
		if (Msg == WM_GETTEXTLENGTH) {
			strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
		}
		else {
			strW = SendMessageAFromW_LB_GETTEXT(hWnd, wParam, &lenW);
		}
		free(strW);
		return lenW;
	}
	case WM_GETTEXT:
	case LB_GETTEXT: {
		size_t lenW;
		wchar_t *strW;
		size_t dest_len;
		if (Msg == WM_GETTEXT) {
			strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
			dest_len = (size_t)wParam;
		}
		else {
			strW = SendMessageAFromW_LB_GETTEXT(hWnd, wParam, &lenW);
			dest_len = lenW + 1;
		}
		wchar_t *dest_ptr = (wchar_t *)lParam;
		wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
		free(strW);
		return dest_len - 1 < lenW ? dest_len - 1 : lenW;
	}
	default:
		retval = SendMessageA(hWnd, Msg, wParam, lParam);
		break;
	}
	return retval;
}

LRESULT WINAPI _SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (pSendMessageW != NULL) {
		return pSendMessageW(hWnd, Msg, wParam, lParam);
	}
	return SendMessageAFromW(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI _SendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (pSendDlgItemMessageW != NULL) {
		return pSendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);
	}

	HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
	return SendMessageAFromW(hWnd, Msg, wParam, lParam);
}

HWND WINAPI _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X,
									int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance,
									LPVOID lpParam)
{
	if (pCreateWindowExW != NULL) {
		return pCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu,
								hInstance, lpParam);
	}

	char *lpClassNameA = ToCharW(lpClassName);
	char *lpWindowNameA = ToCharW(lpWindowName);
	HWND hWnd = CreateWindowExA(dwExStyle, lpClassNameA, lpWindowNameA, dwStyle, X, Y, nWidth, nHeight, hWndParent,
								hMenu, hInstance, lpParam);
	free(lpClassNameA);
	if (lpWindowNameA != NULL) {
		free(lpWindowNameA);
	}
	return hWnd;
}

ATOM WINAPI _RegisterClassW(const WNDCLASSW *lpWndClass)
{
	if (pRegisterClassW != NULL) {
		return pRegisterClassW(lpWndClass);
	}

	char *menu_nameA = ToCharW(lpWndClass->lpszMenuName);
	char *class_nameA = ToCharW(lpWndClass->lpszClassName);

	WNDCLASSA WndClassA;
	WndClassA.style = lpWndClass->style;
	WndClassA.lpfnWndProc = lpWndClass->lpfnWndProc;
	WndClassA.cbClsExtra = lpWndClass->cbClsExtra;
	WndClassA.cbWndExtra = lpWndClass->cbWndExtra;
	WndClassA.hInstance = lpWndClass->hInstance;
	WndClassA.hIcon = lpWndClass->hIcon;
	WndClassA.hCursor = lpWndClass->hCursor;
	WndClassA.hbrBackground = lpWndClass->hbrBackground;
	WndClassA.lpszMenuName = menu_nameA;
	WndClassA.lpszClassName = class_nameA;
	ATOM atom = RegisterClassA(&WndClassA);

	if (menu_nameA != NULL) {
		free(menu_nameA);
	}
	if (class_nameA != NULL) {
		free(class_nameA);
	}
	return atom;
}

BOOL WINAPI _SetWindowTextW(HWND hWnd, LPCWSTR lpString)
{
	if (pSetWindowTextW != NULL) {
		return pSetWindowTextW(hWnd, lpString);
	}

	char *strA = ToCharW(lpString);
	BOOL retval = SetWindowTextA(hWnd, strA);
	free(strA);
	return retval;
}

UINT WINAPI _GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax)
{
	if (pGetDlgItemTextW != NULL) {
		return pGetDlgItemTextW(hDlg, nIDDlgItem, lpString, cchMax);
	}

	if (cchMax <= 1) {
		return 0;
	}

	HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
	size_t lenW;
	wchar_t *strW = SendMessageAFromW_WM_GETTEXT(hWnd, &lenW);
	wchar_t *dest_ptr = lpString;
	size_t dest_len = (size_t)cchMax;
	wcsncpy_s(dest_ptr, dest_len, strW, _TRUNCATE);
	free(strW);
	return (UINT)(dest_len - 1 < lenW ? dest_len - 1 : lenW);
}

/**
 *	@param[in]		hdc
 *	@param[in]		lpchText	������
 *	@param[in]		cchText		������(-1�̂Ƃ�lpchText�̕�����)
 *	@param[in]		lprc		�\��rect
 *	@param[in]		format
 *
 *		TODO:9x�n��DrawTextW�����������삷��?
 */
int WINAPI _DrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format)
{
	if (IsWindowsNTKernel()) {
		return DrawTextW(hdc, lpchText, cchText, lprc, format);
	}

	int strW_len = (cchText == -1) ? 0 : cchText;
	size_t strA_len;
	char *strA = _WideCharToMultiByte(lpchText, strW_len, CP_ACP, &strA_len);
	int result = DrawTextA(hdc, strA, (int)strA_len, lprc, format);
	free(strA);
	return result;
}

int WINAPI _MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
	if (pMessageBoxW != NULL) {
		return pMessageBoxW(hWnd, lpText, lpCaption, uType);
	}

	char *textA = ToCharW(lpText);
	char *captionA = ToCharW(lpCaption);
	int result = MessageBoxA(hWnd, textA, captionA, uType);
	free(textA);
	free(captionA);
	return result;
}

BOOL WINAPI _InsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	if (pInsertMenuW != NULL) {
		return pInsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	}

	char *itemA = ToCharW(lpNewItem);
	int result = InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, itemA);
	free(itemA);
	return result;
}

BOOL WINAPI _AppendMenuW(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem)
{
	if (pAppendMenuW != NULL) {
		return pAppendMenuW(hMenu, uFlags, uIDNewItem, lpNewItem);
	}
	char *itemA = ToCharW(lpNewItem);
	BOOL result = AppendMenuA(hMenu, uFlags, uIDNewItem, itemA);
	free(itemA);
	return result;
}

int WINAPI _AddFontResourceW(LPCWSTR lpFileName)
{
	char *filenameA = ToCharW(lpFileName);
	int result = AddFontResourceA(filenameA);
	free(filenameA);
	return result;
}

BOOL WINAPI _RemoveFontResourceW(LPCWSTR lpFileName)
{
	char *filenameA = ToCharW(lpFileName);
	int result = RemoveFontResourceA(filenameA);
	free(filenameA);
	return result;
}

/*
 * lpData.cbSize == 952�̂Ƃ��̂� ANSI�֐��ŏ�������
 */
BOOL WINAPI _Shell_NotifyIconW(DWORD dwMessage, TT_NOTIFYICONDATAW_V2 *lpData)
{
	if (pShell_NotifyIconW != NULL) {
		return pShell_NotifyIconW(dwMessage, (PNOTIFYICONDATAW)lpData);
	}

	const TT_NOTIFYICONDATAW_V2 *w = lpData;
	if (w->cbSize != sizeof(TT_NOTIFYICONDATAW_V2)) {
		return FALSE;
	}

	TT_NOTIFYICONDATAA_V2 nid;
	TT_NOTIFYICONDATAA_V2 *p = &nid;
	p->cbSize = sizeof(nid);
	p->hWnd = w->hWnd;
	p->uID = w->uID;
	p->uFlags = w->uFlags;
	p->uCallbackMessage = w->uCallbackMessage;
	p->hIcon = w->hIcon;
	p->dwState = w->dwState;
	p->dwStateMask = w->dwStateMask;
	p->uTimeout = w->uTimeout;
	p->dwInfoFlags = w->dwInfoFlags;

	char *strA = ToCharW(w->szTip);
	strcpy_s(p->szTip, strA);
	free(strA);
	strA = ToCharW(w->szInfoTitle);
	strcpy_s(p->szInfoTitle, strA);
	free(strA);
	strA = ToCharW(w->szInfo);
	strcpy_s(p->szInfo, strA);
	free(strA);

	BOOL r = Shell_NotifyIconA(dwMessage, (PNOTIFYICONDATAA)p);
	return r;
}

HWND WINAPI _CreateDialogIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc,
								 LPARAM dwInitParam)
{
	if (pCreateDialogIndirectParamW != NULL) {
		return pCreateDialogIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
	}
	return CreateDialogIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

INT_PTR WINAPI _DialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent,
								 DLGPROC lpDialogFunc, LPARAM lParamInit)
{
	if (pDialogBoxIndirectParamW != NULL) {
		return pDialogBoxIndirectParamW(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParamInit);
	}
	return DialogBoxIndirectParamA(hInstance, hDialogTemplate, hWndParent, lpDialogFunc, lParamInit);
}

LONG WINAPI _SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
{
	if (pSetWindowLongW != NULL) {
		return pSetWindowLongW(hWnd, nIndex, dwNewLong);
	}
	return SetWindowLongA(hWnd, nIndex, dwNewLong);
}

LONG_PTR WINAPI _SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
#ifdef _WIN64
	if (pSetWindowLongPtrW != NULL) {
		return pSetWindowLongPtrW(hWnd, nIndex, dwNewLong);
	}
	return SetWindowLongPtrA(hWnd, nIndex, dwNewLong);
#else
	return _SetWindowLongW(hWnd, nIndex, dwNewLong);
#endif
}

LONG WINAPI _GetWindowLongW(HWND hWnd, int nIndex)
{
	if (pGetWindowLongW != NULL) {
		return pGetWindowLongW(hWnd, nIndex);
	}
	return GetWindowLongA(hWnd, nIndex);
}

LONG_PTR WINAPI _GetWindowLongPtrW(HWND hWnd, int nIndex)
{
#ifdef _WIN64
	if (pGetWindowLongPtrW != NULL) {
		return pGetWindowLongPtrW(hWnd, nIndex);
	}
	return GetWindowLongPtrA(hWnd, nIndex);
#else
	return _GetWindowLongW(hWnd, nIndex);
#endif
}

LRESULT WINAPI _CallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (pCallWindowProcW != NULL) {
		return pCallWindowProcW(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
	}
	return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

void WINAPI _OutputDebugStringW(LPCWSTR lpOutputString)
{
	if (pOutputDebugStringW != NULL) {
		return pOutputDebugStringW(lpOutputString);
	}

	char *strA = ToCharW(lpOutputString);
	OutputDebugStringA(strA);
	free(strA);
}

DWORD WINAPI _GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	if (pGetCurrentDirectoryW != NULL) {
		return pGetCurrentDirectoryW(nBufferLength, lpBuffer);
	}
	char dir[MAX_PATH];
	GetCurrentDirectoryA(_countof(dir), dir);
	wchar_t *strW = ToWcharA(dir);
	wcsncpy_s(lpBuffer, nBufferLength, strW, _TRUNCATE);
	free(strW);
	DWORD r =  (DWORD)wcslen(lpBuffer);
	return r;
}

BOOL WINAPI _SetCurrentDirectoryW(LPCWSTR lpPathName)
{
	if (pSetCurrentDirectoryW != NULL) {
		return pSetCurrentDirectoryW(lpPathName);
	}
	char *strA = ToCharW(lpPathName);
	BOOL r = SetCurrentDirectoryA(strA);
	free(strA);
	return r;
}

LPITEMIDLIST WINAPI _SHBrowseForFolderW(LPBROWSEINFOW lpbi)
{
	if (pSHBrowseForFolderW != NULL) {
		return pSHBrowseForFolderW(lpbi);
	}

	BROWSEINFOA biA;
	biA.hwndOwner = lpbi->hwndOwner;
	biA.pidlRoot = lpbi->pidlRoot;
	biA.pszDisplayName = ToCharW(lpbi->pszDisplayName);
	biA.lpszTitle = ToCharW(lpbi->lpszTitle);
	biA.ulFlags = lpbi->ulFlags;
	biA.lpfn = lpbi->lpfn;
	biA.lParam = lpbi->lParam;
	LPITEMIDLIST pidlBrowse = SHBrowseForFolderA(&biA);
	free(biA.pszDisplayName);
	free((void *)biA.lpszTitle);

	return pidlBrowse;
}

BOOL WINAPI _SHGetPathFromIDListW(LPITEMIDLIST pidl, LPWSTR pszPath)
{
	if (pSHGetPathFromIDListW != NULL) {
		return pSHGetPathFromIDListW(pidl, pszPath);
	}

	char pathA[MAX_PATH];
	BOOL r = SHGetPathFromIDListA(pidl, pathA);
	::MultiByteToWideChar(CP_ACP, 0, pathA, -1, pszPath, MAX_PATH);
	return r;
}

DWORD WINAPI _GetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
								LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	if (pGetPrivateProfileStringW != NULL) {
		return pGetPrivateProfileStringW(lpAppName, lpKeyName, lpDefault,
										 lpReturnedString, nSize, lpFileName);
	}

	if (lpDefault == NULL) {
		lpDefault = L"";
	}
	char *buf = (char* )malloc(nSize);
	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *defA = ToCharW(lpDefault);
	char *fileA = ToCharW(lpFileName);
	DWORD r = GetPrivateProfileStringA(appA, keyA, defA, buf, nSize, fileA);
	::MultiByteToWideChar(CP_ACP, 0, buf, -1, lpReturnedString, nSize);
	r = (DWORD)wcslen(lpReturnedString);
	free(appA);
	free(keyA);
	free(defA);
	free(fileA);
	free(buf);
	return r;
}

BOOL WINAPI _WritePrivateProfileStringW(LPCWSTR lpAppName,LPCWSTR lpKeyName,LPCWSTR lpString,LPCWSTR lpFileName)
{
	if (pWritePrivateProfileStringW != NULL) {
		return pWritePrivateProfileStringW(lpAppName, lpKeyName, lpString, lpFileName);
	}

	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *strA = ToCharW(lpString);
	char *fileA = ToCharW(lpFileName);
	BOOL r = WritePrivateProfileStringA(appA, keyA, strA, fileA);
	free(appA);
	free(keyA);
	free(strA);
	free(fileA);
	return r;
}

UINT WINAPI _GetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName)
{
	if (pGetPrivateProfileIntW != NULL) {
		return pGetPrivateProfileIntW(lpAppName, lpKeyName, nDefault, lpFileName);
	}

	char *appA = ToCharW(lpAppName);
	char *keyA = ToCharW(lpKeyName);
	char *fileA = ToCharW(lpFileName);
	UINT r = GetPrivateProfileIntA(appA, keyA, nDefault, fileA);
	free(appA);
	free(keyA);
	free(fileA);
	return r;
}

BOOL WINAPI _CreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
					 LPSECURITY_ATTRIBUTES lpProcessAttributes,
					 LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
					 DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
					 LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	if (pCreateProcessW != NULL) {
		return pCreateProcessW(lpApplicationName, lpCommandLine,
							   lpProcessAttributes,
							   lpThreadAttributes,  bInheritHandles,
							   dwCreationFlags,  lpEnvironment,  lpCurrentDirectory,
							   lpStartupInfo,  lpProcessInformation);
	}

	STARTUPINFOA suiA = {};
	suiA.cb = sizeof(suiA);
	suiA.lpReserved = NULL;
	suiA.lpDesktop = ToCharW(lpStartupInfo->lpDesktop);
	suiA.lpTitle = ToCharW(lpStartupInfo->lpTitle);
	suiA.dwX = lpStartupInfo->dwX;
	suiA.dwY = lpStartupInfo->dwY;
	suiA.dwXSize = lpStartupInfo->dwXSize;
	suiA.dwYSize = lpStartupInfo->dwYSize;
	suiA.dwXCountChars = lpStartupInfo->dwXCountChars;
	suiA.dwYCountChars = lpStartupInfo->dwYCountChars;
	suiA.dwFillAttribute = lpStartupInfo->dwFillAttribute;
	suiA.dwFlags = lpStartupInfo->dwFlags;
	suiA.wShowWindow = lpStartupInfo->wShowWindow;
	suiA.cbReserved2 = lpStartupInfo->cbReserved2;
	suiA.lpReserved2 = lpStartupInfo->lpReserved2;
	suiA.hStdInput = lpStartupInfo->hStdInput;
	suiA.hStdOutput = lpStartupInfo->hStdOutput;
	suiA.hStdError = lpStartupInfo->hStdError;

	char *appA = ToCharW(lpApplicationName);
	char *cmdA = ToCharW(lpCommandLine);
	char *curA = ToCharW(lpCurrentDirectory);
	BOOL r =
		CreateProcessA(appA, cmdA, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
					   dwCreationFlags, lpEnvironment, curA, &suiA, lpProcessInformation);
	free(appA);
	free(cmdA);
	free(curA);
	free(suiA.lpDesktop);
	free(suiA.lpTitle);

	return r;
}

BOOL WINAPI _CopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
	if (pCopyFileW != NULL) {
		return pCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
	}
	char *lpExistingFileNameA = ToCharW(lpExistingFileName);
	char *lpNewFileNameA = ToCharW(lpNewFileName);
	BOOL r = CopyFileA(lpExistingFileNameA, lpNewFileNameA, bFailIfExists);
	free(lpExistingFileNameA);
	free(lpNewFileNameA);
	return r;
}

BOOL WINAPI _DeleteFileW(LPCWSTR lpFileName)
{
	if (pDeleteFileW != NULL) {
		return pDeleteFileW(lpFileName);
	}
	char *lpFileNameA = ToCharW(lpFileName);
	BOOL r = DeleteFileA(lpFileNameA);
	free(lpFileNameA);
	return r;
}

BOOL WINAPI _MoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	if (pMoveFileW != NULL) {
		return pMoveFileW(lpExistingFileName, lpNewFileName);
	}
	char *lpExistingFileNameA = ToCharW(lpExistingFileName);
	char *lpNewFileNameA = ToCharW(lpNewFileName);
	BOOL r = MoveFileA(lpExistingFileNameA, lpNewFileNameA);
	free(lpExistingFileNameA);
	free(lpNewFileNameA);
	return r;
}

HANDLE WINAPI _CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
					LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
					HANDLE hTemplateFile)
{
	if (pCreateFileW != NULL) {
		return pCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
							dwFlagsAndAttributes, hTemplateFile);
	}

	char *lpFileNameA = ToCharW(lpFileName);
	HANDLE handle = CreateFileA(lpFileNameA, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
								dwFlagsAndAttributes, hTemplateFile);
	free(lpFileNameA);
	return handle;
}

static void FindDataAW(const WIN32_FIND_DATAA *a, WIN32_FIND_DATAW *w)
{
	w->dwFileAttributes = a->dwFileAttributes;
	w->ftCreationTime = a->ftCreationTime;
	w->ftLastAccessTime = a->ftLastAccessTime;
	w->ftLastWriteTime = a->ftLastWriteTime;
	w->nFileSizeHigh = a->nFileSizeHigh;
	w->nFileSizeLow = a->nFileSizeLow;
	w->dwReserved0 = a->dwReserved0;
	w->dwReserved1 = a->dwReserved1;
	::MultiByteToWideChar(CP_ACP, 0, a->cFileName, _countof(a->cFileName), w->cFileName, _countof(w->cFileName));
	::MultiByteToWideChar(CP_ACP, 0, a->cAlternateFileName, _countof(a->cAlternateFileName), w->cAlternateFileName, _countof(w->cAlternateFileName));
}

HANDLE WINAPI _FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (pFindFirstFileW != NULL) {
		return pFindFirstFileW(lpFileName, lpFindFileData);
	}

	WIN32_FIND_DATAA find_file_data;
	char *lpFileNameA = ToCharW(lpFileName);
	HANDLE handle = FindFirstFileA(lpFileNameA, &find_file_data);
	free(lpFileNameA);
	FindDataAW(&find_file_data, lpFindFileData);
	return handle;
}

BOOL WINAPI _FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (pFindNextFileW != NULL) {
		return pFindNextFileW(hFindFile, lpFindFileData);
	}
	WIN32_FIND_DATAA find_file_data;
	BOOL r = FindNextFileA(hFindFile, &find_file_data);
	FindDataAW(&find_file_data, lpFindFileData);
	return r;
}

BOOL WINAPI _RemoveDirectoryW(LPCWSTR lpPathName)
{
	if (pRemoveDirectoryW != NULL) {
		return pRemoveDirectoryW(lpPathName);
	}
	char *lpPathNameA = ToCharW(lpPathName);
	BOOL r = RemoveDirectoryA(lpPathNameA);
	free(lpPathNameA);
	return r;
}

DWORD WINAPI _GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart)
{
	if (pGetFullPathNameW != NULL) {
		return pGetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);
	}

	if (nBufferLength == 0 || lpBuffer == NULL) {
		char *filenameA = ToCharW(lpFileName);
		DWORD r = GetFullPathNameA(filenameA, 0, NULL, NULL);
		free(filenameA);
		return r;
	}
	else {
		char *filenameA = ToCharW(lpFileName);
		char bufA[MAX_PATH];
		char *filepartA;
		DWORD r = GetFullPathNameA(filenameA, sizeof(bufA), bufA, &filepartA);
		if (r == 0) {
			// error
			free(filenameA);
			return 0;
		}
		wchar_t *bufW = ToWcharA(bufA);
		r = (DWORD)wcslen(bufW);
		if (nBufferLength == 0 || lpBuffer == NULL) {
			// �K�v�ȕ�������Ԃ�('\0'�܂�)
			r = r + 1;
		} else {
			// �p�X���R�s�[���āA�����񒷂�Ԃ�('\0'�܂܂Ȃ�)
			wcsncpy_s(lpBuffer, nBufferLength, bufW, _TRUNCATE);
			if (lpFilePart != NULL) {
				*lpFilePart = lpBuffer + (filepartA - filenameA) * sizeof(wchar_t);
			}
		}
		free(filenameA);
		free(bufW);
		return r;
	}
}

HMODULE WINAPI _LoadLibraryW(LPCWSTR lpLibFileName)
{
	if (pLoadLibraryW != NULL) {
		return pLoadLibraryW(lpLibFileName);
	}
	char *LibFileNameA = ToCharW(lpLibFileName);
	HMODULE r = LoadLibraryA(LibFileNameA);
	free(LibFileNameA);
	return r;
}

DWORD WINAPI _GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize)
{
	if (pGetModuleFileNameW != NULL) {
		return pGetModuleFileNameW(hModule, lpFilename, nSize);
	}

	char filenameA[MAX_PATH];
	DWORD r = GetModuleFileNameA(hModule, filenameA, sizeof(filenameA));
	if (r == 0) {
		return 0;
	}
	DWORD wlen = ACPToWideChar_t(filenameA, lpFilename, nSize);
	return wlen - 1;	// not including the terminating null character
}

DWORD WINAPI _ExpandEnvironmentStringsW(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize)
{
	if (pExpandEnvironmentStringsW != NULL) {
		return pExpandEnvironmentStringsW(lpSrc, lpDst, nSize);
	}

	char *srcA = ToCharW(lpSrc);
	char dstA[MAX_PATH];	// MAX_PATH?
	DWORD r = ExpandEnvironmentStringsA(srcA, dstA, sizeof(dstA));
	wchar_t *dstW = ToWcharA(dstA);
	wcsncpy_s(lpDst, nSize, dstW, _TRUNCATE);
	r = (DWORD)wcslen(dstW);
	free(srcA);
	free(dstW);
	return r;
}

HMODULE WINAPI _GetModuleHandleW(LPCWSTR lpModuleName)
{
	char *lpStringA = ToCharW(lpModuleName);
	HMODULE h = GetModuleHandleA(lpStringA);
	free(lpStringA);
	return h;
}

UINT WINAPI _GetSystemDirectoryW(LPWSTR lpBuffer, UINT uSize)
{
	char buf[MAX_PATH];
	UINT r = GetSystemDirectoryA(buf, _countof(buf));
	if (r == 0) {
		return 0;
	}
	size_t wlen = ACPToWideChar_t(buf, lpBuffer, uSize);
	return wlen - 1;	// not including the terminating null character
}

DWORD WINAPI _GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	if (pGetTempPathW != NULL) {
		return pGetTempPathW(nBufferLength, lpBuffer);
	}

	char buf[MAX_PATH];
	DWORD r = GetTempPathA(_countof(buf), buf);
	if (r == 0) {
		return 0;
	}
	size_t wlen = ACPToWideChar_t(buf, lpBuffer, nBufferLength);
	return wlen - 1;	// not including the terminating null character
}

UINT WINAPI _GetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName)
{
	if (pGetTempFileNameW != NULL) {
		return pGetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);
	}

	char buf[MAX_PATH];
	char *lpPathNameA = ToCharW(lpPathName);
	char *lpPrefixStringA = ToCharW(lpPrefixString);
	UINT r = GetTempFileNameA(lpPathNameA, lpPrefixStringA, uUnique, buf);
	ACPToWideChar_t(buf, lpTempFileName, MAX_PATH);
	free(lpPathNameA);
	free(lpPrefixStringA);
	return r;
}
