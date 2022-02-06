/*
 * Copyright (C) 2022- TeraTerm Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wchar.h>
#include <shlobj.h>
#define INITGUID
#include <knownfolders.h>
#include <tchar.h>

#include "sub.h"

/**
 *	�}���`�o�C�g�������wchar_t������֕ϊ�
 *	@param[in]	*str_ptr	mb(char)������
 *	@param[in]	str_len		mb(char)������(0�̂Ƃ������A�����̂Ƃ���'\0'�Ń^�[�~�l�[�g���邱��)
 *	@param[in]	code_page	�ϊ����R�[�h�y�[�W
 *	@param[out]	*w_len_		wchar_t������,wchar_t��,'\0'��ϊ�������L'\0'���܂�
 *							(NULL�̂Ƃ������񒷂�Ԃ��Ȃ�)
 *	@retval		wchar_t������ւ̃|�C���^(NULL�̎��ϊ��G���[)
 *				�g�p�� free() ���邱��
 */
static wchar_t *_MultiByteToWideChar(const char *str_ptr, size_t str_len, int code_page, size_t *w_len_)
{
	DWORD flags = MB_ERR_INVALID_CHARS;
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	if (code_page == CP_UTF8) {
		// CP_UTF8 When this is set, dwFlags must be zero.
		flags = 0;
	}
	if (w_len_ != NULL) {
		*w_len_ = 0;
	}
	if (str_len == 0) {
		str_len = strlen(str_ptr) + 1;
	}
	int len = ::MultiByteToWideChar(code_page, flags,
									str_ptr, (int)str_len,
									NULL, 0);
	if (len == 0) {
		return NULL;
	}
	wchar_t *wstr_ptr = (wchar_t *)malloc(len*sizeof(wchar_t));
	if (wstr_ptr == NULL) {
		return NULL;
	}
	len = ::MultiByteToWideChar(code_page, flags,
								str_ptr, (int)str_len,
								wstr_ptr, len);
	if (len == 0) {
		free(wstr_ptr);
		return NULL;
	}
	if (w_len_ != NULL) {
		// �ϊ�����������(wchar_t��)��Ԃ�
		*w_len_ = len;
	}
	return wstr_ptr;
}

/**
 *	wchar_t��������}���`�o�C�g������֕ϊ�
 *	�ϊ��ł��Ȃ������� '?' �ŏo�͂���
 *
 *	@param[in]	*wstr_ptr	wchar_t������
 *	@param[in]	wstr_len	wchar_t������(0�̂Ƃ������A�����̂Ƃ���L'\0'�Ń^�[�~�l�[�g���邱��)
 *	@param[in]	code_page	�ϊ���R�[�h�y�[�W
 *	@param[out]	*mb_len_	�ϊ�����������,byte��,L'\0'��ϊ�������'\0'���܂�
 *							(NULL�̂Ƃ������񒷂�Ԃ��Ȃ�)
 *	@retval		mb������ւ̃|�C���^(NULL�̎��ϊ��G���[)
 *				�g�p�� free() ���邱��
 */
static char *_WideCharToMultiByte(const wchar_t *wstr_ptr, size_t wstr_len, int code_page, size_t *mb_len_)
{
	const DWORD flags = 0;
	char *mb_ptr;
	if (code_page == CP_ACP) {
		code_page = (int)GetACP();
	}
	if (mb_len_ != NULL) {
		*mb_len_ = 0;
	}
	if (wstr_len == 0) {
		wstr_len = wcslen(wstr_ptr) + 1;
	}
	size_t len = ::WideCharToMultiByte(code_page, flags,
									   wstr_ptr, (DWORD)wstr_len,
									   NULL, 0,
									   NULL, NULL);
	if (len == 0) {
		return NULL;
	}
	mb_ptr = (char *)malloc(len);
	if (mb_ptr == NULL) {
		return NULL;
	}
	len = ::WideCharToMultiByte(code_page, flags,
								wstr_ptr, (DWORD)wstr_len,
								mb_ptr, (int)len,
								NULL,NULL);
	if (len == 0) {
		free(mb_ptr);
		return NULL;
	}
	if (mb_len_ != NULL) {
		// �ϊ�����������(byte��)��Ԃ�
		*mb_len_ = len;
	}
	return mb_ptr;
}

char *ToCharW(const wchar_t *strW)
{
	if (strW == NULL) return NULL;
	char *strA = _WideCharToMultiByte(strW, 0, CP_ACP, NULL);
	return strA;
}

wchar_t *ToWcharA(const char *strA)
{
	if (strA == NULL) return NULL;
	wchar_t *strW = _MultiByteToWideChar(strA, 0, CP_ACP, NULL);
	return strW;
}

wchar_t *ToWcharU8(const char *strU8)
{
	if (strU8 == NULL) return NULL;
	wchar_t *strW = _MultiByteToWideChar(strU8, 0, CP_UTF8, NULL);
	return strW;
}

char *ToU8W(const wchar_t *strW)
{
	if (strW == NULL) return NULL;
	char *strU8 = _WideCharToMultiByte(strW, 0, CP_UTF8, NULL);
	return strU8;
}

/**
 *	GetModuleFileNameW() �̓��I�o�b�t�@��
 *
 *	@param buf	��������i�[����o�b�t�@
 *				�s�v�ɂȂ�����free()����
 *	@return	�G���[�R�[�h,0(=NO_ERROR)�̂Ƃ��G���[�Ȃ�
 */
DWORD hGetModuleFileNameT(HMODULE hModule, TCHAR **buf)
{
	size_t size = MAX_PATH;
	TCHAR *b = (TCHAR *)malloc(sizeof(TCHAR) * size);
	DWORD error;
	if (b == NULL) {
		error = ERROR_NOT_ENOUGH_MEMORY;
		goto error_return;
	}

	for(;;) {
		DWORD r = GetModuleFileName(hModule, b, (DWORD)size);
		if (r == 0) {
			// �֐������s
			error = GetLastError();
			break;
		} else if (r < size - 1) {
			// �擾����
			size = r + 1;
			b = (TCHAR *)realloc(b, sizeof(TCHAR) * size);
			*buf = b;
			return NO_ERROR;
		} else {
			size *= 2;
			TCHAR *p = (TCHAR *)realloc(b, sizeof(TCHAR) * size);
			if (p == NULL) {
				free(b);
				error = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}
			b = p;
		}
    }

	// error
	free(b);
error_return:
	*buf = NULL;
	return error;
}

/**
 *	�|�[�^�u���łƂ��ē��삷�邩
 *
 *	@retval		TRUE		�|�[�^�u����
 *	@retval		FALSE		�ʏ�C���X�g�[����
 */
BOOL IsPortableMode(void)
{
	static BOOL called = FALSE;
	static BOOL ret_val = FALSE;
	if (called == FALSE) {
		called = TRUE;
		TCHAR *exe;
		hGetModuleFileNameT(NULL, &exe);
#if UNICODE
		wchar_t *exe_path = wcsdup(exe);
#else
		wchar_t *exe_path = ToWcharA(exe);
#endif
		wchar_t *bs = wcsrchr(exe_path, L'\\');
		*bs = 0;
		const wchar_t *portable_ini_base = L"\\portable.ini";
		size_t len = wcslen(exe_path) + wcslen(portable_ini_base) + 1;
		wchar_t *portable_iniW = (wchar_t *)malloc(sizeof(wchar_t) * len);
		wcscpy(portable_iniW, exe_path);
		wcscat(portable_iniW, portable_ini_base);
#if UNICODE
		DWORD r = GetFileAttributesW(portable_iniW);
#else
		char *portable_iniA = ToCharW(portable_iniW);
		DWORD r = GetFileAttributesA(portable_iniA);
		free(portable_iniA);
#endif
		free(portable_iniW);
		if (r == INVALID_FILE_ATTRIBUTES) {
			//�t�@�C�������݂��Ȃ�
			ret_val = FALSE;
		}
		else {
			ret_val = TRUE;
		}
	}
	return ret_val;
}

// $APPDATA wchar_t
static wchar_t *get_appdata_dir(void)
{
#if _WIN32_WINNT > 0x0600
	wchar_t *appdata;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appdata);
	wchar_t *retval = wcsdup(appdata);
	CoTaskMemFree(appdata);
	return retval;
#else
	LPITEMIDLIST pidl;
	HRESULT r = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
	if (r == NOERROR) {
		wchar_t appdata[MAX_PATH];
		SHGetPathFromIDListW(pidl, appdata);
		wchar_t *retval = wcsdup(appdata);
		CoTaskMemFree(pidl);
		return retval;
	}
	char *env = getenv("APPDATA");
	if (env == NULL) {
		// �����ƌÂ� windows ?
		abort();
	}
	wchar_t *appdata = ToWcharA(env);
	return appdata;
#endif
}

// L'\\' -> L'/'
static void convert_bsW(wchar_t *path)
{
	wchar_t *p = path;
	while(*p != 0) {
		if (*p == L'\\') {
			*p = L'/';
		}
		p++;
	}
}

// cygwin ���� $APPDATA utf-8 ��Ԃ�
char *GetAppDataDirU8()
{
	wchar_t *appdataW = get_appdata_dir();
	convert_bsW(appdataW);
	char *appdataU8 = ToU8W(appdataW);
	free(appdataW);
	return appdataU8;
}

/**
 *	full path exe�t�@�C������Ԃ�
 */
char *GetModuleFileNameU8(void)
{
	TCHAR *buf;
	hGetModuleFileNameT(NULL, &buf);
	convert_bsW(buf);
#if UNICODE
	char *exe = ToU8W(buf);
#else
	char *exe = strdup(buf);
#endif
	return exe;
}
