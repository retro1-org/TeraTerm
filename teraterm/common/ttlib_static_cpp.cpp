/*
 * Copyright (C) 2020- TeraTerm Project
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

#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <wchar.h>
#include <shlobj.h>
#include <malloc.h>

#include "i18n.h"
#include "asprintf.h"
#include "win32helper.h"
#include "codeconv.h"
#include "compat_win.h"

#include "ttlib.h"

#if _WIN32_WINNT >= 0x0600 // Vista+
#define IFILEOPENDIALOG_ENABLE 1
#endif

/**
 *	MessageBox��\������
 *
 *	@param[in]	hWnd			�e window
 *	@param[in]	info			�^�C�g���A���b�Z�[�W
 *	@param[in]	UILanguageFile	lng�t�@�C��
 *	@param[in]	...				�t�H�[�}�b�g����
 *
 *	info.message ��������������Ƃ��āA
 *	UILanguageFile�����̈������o�͂���
 *
 *	info.message_key, info.message_default �����Ƃ�NULL�̏ꍇ
 *		�ψ�����1�ڂ�������������Ƃ��Ďg�p����
 */
int TTMessageBoxW(HWND hWnd, const TTMessageBoxInfoW *info, const wchar_t *UILanguageFile, ...)
{
	const char *section = info->section;
	const UINT uType = info->uType;
	wchar_t *title;
	if (info->title_key == NULL) {
		title = _wcsdup(info->title_default);
	}
	else {
		GetI18nStrWW(section, info->title_key, info->title_default, UILanguageFile, &title);
	}

	wchar_t *message = NULL;
	if (info->message_key == NULL && info->message_default == NULL) {
		wchar_t *format;
		va_list ap;
		va_start(ap, UILanguageFile);
		format = va_arg(ap, wchar_t *);
		vaswprintf(&message, format, ap);
	}
	else {
		wchar_t *format;
		GetI18nStrWW(section, info->message_key, info->message_default, UILanguageFile, &format);
		va_list ap;
		va_start(ap, UILanguageFile);
		vaswprintf(&message, format, ap);
		free(format);
	}

	int r = MessageBoxW(hWnd, message, title, uType);

	free(title);
	free(message);

	return r;
}

/**
 *	MessageBox��\������
 *
 *	@param[in]	hWnd			�e window
 *	@param[in]	info			�^�C�g���A���b�Z�[�W
 *	@param[in]	UILanguageFile	lng�t�@�C��
 *	@param[in]	...				�t�H�[�}�b�g����
 *
 *	info.message ��������������Ƃ��āA
 *	UILanguageFile�����̈������o�͂���
 *
 *	info.message_key, info.message_default �����Ƃ�NULL�̏ꍇ
 *		�ψ�����1�ڂ�������������Ƃ��Ďg�p����
 */
int TTMessageBoxA(HWND hWnd, const TTMessageBoxInfoW *info, const char *UILanguageFile, ...)
{
	const char *section = info->section;
	const UINT uType = info->uType;
	wchar_t *title;
	if (info->title_key == NULL) {
		title = _wcsdup(info->title_default);
	}
	else {
		title = TTGetLangStrW(section, info->title_key, info->title_default, UILanguageFile);
	}

	wchar_t *message = NULL;
	if (info->message_key == NULL && info->message_default == NULL) {
		wchar_t *format;
		va_list ap;
		va_start(ap, UILanguageFile);
		format = va_arg(ap, wchar_t *);
		vaswprintf(&message, format, ap);
	}
	else {
		wchar_t *format = TTGetLangStrW(section, info->message_key, info->message_default, UILanguageFile);
		va_list ap;
		va_start(ap, UILanguageFile);
		vaswprintf(&message, format, ap);
		free(format);
	}

	int r = MessageBoxW(hWnd, message, title, uType);

	free(title);
	free(message);

	return r;
}

/**
 *	str���e�L�X�g���ǂ����`�F�b�N
 *
 *	@param[in]	str		�e�X�g���镶��
 *	@param[in]	len		�e�X�g����e�L�X�g��(L'\0'�͊܂܂Ȃ�)
 *						0�̂Ƃ�L'\0'�̑O�܂Ńe�X�g����
 *	@retval		TRUE	�e�L�X�g�Ǝv����(����0�̂Ƃ����܂�)
 *				FALSE	�e�L�X�g�ł͂Ȃ�
 *
 *	��������f�[�^����0x20�����̃e�L�X�g�ɂ͏o�Ă��Ȃ������������
 *	�o�C�i���Ɣ��f
 *	IsTextUnicode() �̂ق����ǂ����H
 */
BOOL IsTextW(const wchar_t *str, size_t len)
{
	if (len == 0) {
		len = wcslen(str);
		if (len == 0) {
			return TRUE;
		}
	}

	BOOL result = TRUE;
	while(len-- > 0) {
		wchar_t c = *str++;
		if (c >= 0x20) {
			continue;
		}
		if ((7 <= c && c <= 0x0d) || c == 0x1b) {
			/* \a, \b, \t, \n, \v, \f, \r, \e */
			continue;
		}
		result = FALSE;
		break;
	}
	return result;
}

/**
 *	�N���b�v�{�[�h����wchar_t��������擾����
 *	�����񒷂��K�v�ȂƂ���wcslen()���邱��
 *	@param	hWnd
 *	@param	emtpy	TRUE�̂Ƃ��N���b�v�{�[�h����ɂ���
 *	@retval	������ւ̃|�C���^ �g�p��free()���邱��
 *			�������Ȃ�(�܂��̓G���[��)��NULL
 */
wchar_t *GetClipboardTextW(HWND hWnd, BOOL empty)
{
	UINT Cf;
	wchar_t *str_w = NULL;
	size_t str_w_len;
	HGLOBAL TmpHandle;

	// TODO GetPriorityClipboardFormat()
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		Cf = CF_UNICODETEXT;
	}
	else if (IsClipboardFormatAvailable(CF_TEXT)) {
		Cf = CF_TEXT;
	}
	else if (IsClipboardFormatAvailable(CF_OEMTEXT)) {
		Cf = CF_OEMTEXT;
	}
	else if (IsClipboardFormatAvailable(CF_HDROP)) {
		Cf = CF_HDROP;
	}
	else {
		return NULL;
	}

 	if (!OpenClipboard(hWnd)) {
		return NULL;
	}
	TmpHandle = GetClipboardData(Cf);
	if (TmpHandle == NULL) {
		return NULL;
	}
	if (Cf == CF_HDROP) {
		HDROP hDrop = (HDROP)TmpHandle;
		UINT count = DragQueryFileW(hDrop, (UINT)-1, NULL, 0);

		// text length
		size_t length = 0;
		for (UINT i = 0; i < count; i++) {
			const UINT l = DragQueryFileW(hDrop, i, NULL, 0);
			if (l == 0) {
				continue;
			}
			length += (l + 1);
		}

		// filename
		str_w = (wchar_t *)malloc(length * sizeof(wchar_t));
		wchar_t *p = str_w;
		for (UINT i = 0; i < count; i++) {
			const UINT l = DragQueryFileW(hDrop, i, p, (UINT)(length - (p - str_w)));
			if (l == 0) {
				continue;
			}
			p += l;
			*p++ = (i + 1 == count) ? L'\0' : '\n';
		}
	}
	else if (Cf == CF_UNICODETEXT) {
		const wchar_t *str_cb = (wchar_t *)GlobalLock(TmpHandle);
		if (str_cb != NULL) {
			size_t str_cb_len = GlobalSize(TmpHandle);	// bytes
			str_w_len = str_cb_len / sizeof(wchar_t);
			str_w = (wchar_t *)malloc((str_w_len + 1) * sizeof(wchar_t));	// +1 for terminator
			if (str_w != NULL) {
				memcpy(str_w, str_cb, str_cb_len);
				str_w[str_w_len] = L'\0';
			}
		}
	}
	else {
		const char *str_cb = (char *)GlobalLock(TmpHandle);
		if (str_cb != NULL) {
			size_t str_cb_len = GlobalSize(TmpHandle);
			str_w_len = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str_cb, (int)str_cb_len, NULL, 0);
			str_w = (wchar_t *)malloc(sizeof(wchar_t) * (str_w_len + 1));	// +1 for terminator
			if (str_w != NULL) {
				str_w_len = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str_cb, (int)str_cb_len, str_w, (int)str_w_len);
				str_w[str_w_len] = L'\0';
			}
		}
	}
	GlobalUnlock(TmpHandle);
	if (empty) {
		EmptyClipboard();
	}
	CloseClipboard();
	return str_w;
}

/**
 *	�N���b�v�{�[�h����ANSI��������擾����
 *	�����񒷂��K�v�ȂƂ���strlen()���邱��
 *	@param	hWnd
 *	@param	emtpy	TRUE�̂Ƃ��N���b�v�{�[�h����ɂ���
 *	@retval	������ւ̃|�C���^ �g�p��free()���邱��
 *			�������Ȃ�(�܂��̓G���[��)��NULL
 */
char *GetClipboardTextA(HWND hWnd, BOOL empty)
{
	HGLOBAL hGlobal;
	const char *lpStr;
	size_t length;
	char *pool;

    OpenClipboard(hWnd);
    hGlobal = (HGLOBAL)GetClipboardData(CF_TEXT);
    if (hGlobal == NULL) {
        CloseClipboard();
		return NULL;
    }
    lpStr = (const char *)GlobalLock(hGlobal);
	length = GlobalSize(hGlobal);
	if (length == 0) {
		pool = NULL;
	} else {
		pool = (char *)malloc(length + 1);	// +1 for terminator
		memcpy(pool, lpStr, length);
		pool[length] = '\0';
	}
	GlobalUnlock(hGlobal);
	if (empty) {
		EmptyClipboard();
	}
	CloseClipboard();

	return pool;
}

/**
 *	�N���b�v�{�[�h�Ƀe�L�X�g���Z�b�g����
 *	str_w	�N���b�v�{�[�h�ɃZ�b�g���镶����ւ̃|�C���^
 *			NULL�̂Ƃ��N���b�v�{�[�h����ɂ���(str_len�͎Q�Ƃ���Ȃ�)
 *	str_len	������
 *			0�̂Ƃ������񒷂������ŎZ�o�����
 */
BOOL CBSetTextW(HWND hWnd, const wchar_t *str_w, size_t str_len)
{
	HGLOBAL CBCopyWideHandle;

	if (str_w == NULL) {
		str_len = 0;
	} else {
		if (str_len == 0)
			str_len = wcslen(str_w);
	}

	if (!OpenClipboard(hWnd)) {
		return FALSE;
	}

	EmptyClipboard();
	if (str_len == 0) {
		CloseClipboard();
		return TRUE;
	}

	{
		// ��������R�s�[�A�Ō��L'\0'���܂߂�
		wchar_t *CBCopyWidePtr;
		const size_t alloc_bytes = (str_len + 1) * sizeof(wchar_t);
		CBCopyWideHandle = GlobalAlloc(GMEM_MOVEABLE, alloc_bytes);
		if (CBCopyWideHandle == NULL) {
			CloseClipboard();
			return FALSE;
		}
		CBCopyWidePtr = (wchar_t *)GlobalLock(CBCopyWideHandle);
		if (CBCopyWidePtr == NULL) {
			CloseClipboard();
			return FALSE;
		}
		memcpy(CBCopyWidePtr, str_w, alloc_bytes - sizeof(wchar_t));
		CBCopyWidePtr[str_len] = L'\0';
		GlobalUnlock(CBCopyWideHandle);
	}

	SetClipboardData(CF_UNICODETEXT, CBCopyWideHandle);

	// TODO 9x�n�ł͎�����CF_TEXT�ɃZ�b�g����Ȃ��炵��?
	// ttl_gui.c �� TTLVar2Clipb() �ł͂���2���s���Ă���
	//		SetClipboardData(CF_TEXT, hText);
	//		SetClipboardData(CF_UNICODETEXT, wide_hText);
	CloseClipboard();

	return TRUE;
}

// from ttxssh
static void format_line_hexdump(char *buf, int buflen, int addr, int *bytes, int byte_cnt)
{
	int i, c;
	char tmp[128];

	buf[0] = 0;

	/* �擪�̃A�h���X�\�� */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%08X : ", addr);
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* �o�C�i���\���i4�o�C�g���Ƃɋ󔒂�}���j*/
	for (i = 0; i < byte_cnt; i++) {
		if (i > 0 && i % 4 == 0) {
			strncat_s(buf, buflen, " ", _TRUNCATE);
		}

		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02X", bytes[i]);
		strncat_s(buf, buflen, tmp, _TRUNCATE);
	}

	/* ASCII�\�������܂ł̋󔒂�₤ */
	_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "   %*s%*s", (16 - byte_cnt) * 2 + 1, " ", (16 - byte_cnt + 3) / 4, " ");
	strncat_s(buf, buflen, tmp, _TRUNCATE);

	/* ASCII�\�� */
	for (i = 0; i < byte_cnt; i++) {
		c = bytes[i];
		if (isprint(c)) {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", c);
			strncat_s(buf, buflen, tmp, _TRUNCATE);
		}
		else {
			strncat_s(buf, buflen, ".", _TRUNCATE);
		}
	}
}

void DebugHexDump(void (*f)(const char *s), const void *data_, size_t len)
{
	const char *data = (char *)data_;
	char buff[4096];
	int c, addr;
	int bytes[16], *ptr;
	int byte_cnt;

	addr = 0;
	byte_cnt = 0;
	ptr = bytes;
	for (size_t i = 0; i < len; i++) {
		c = data[i];
		*ptr++ = c & 0xff;
		byte_cnt++;

		if (byte_cnt == 16) {
			format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
			f(buff);

			addr += 16;
			byte_cnt = 0;
			ptr = bytes;
		}
	}

	if (byte_cnt > 0) {
		format_line_hexdump(buff, sizeof(buff), addr, bytes, byte_cnt);
		f(buff);
	}
}

static void OutputDebugHexDumpSub(const char *s)
{
	char *s_cr;
	asprintf(&s_cr, "%s\n", s);
	OutputDebugStringA(s_cr);
	free(s_cr);
}

void OutputDebugHexDump(const void *data, size_t len)
{
	DebugHexDump(OutputDebugHexDumpSub, data, len);
}

/**
 *	���j���[��ǉ�����
 *	InsertMenuA() �Ƃقړ�������
 *	before������ FALSE �ɂ���Ǝ��̍��ڂɒǉ��ł���
 *
 *	@param[in]	hMenu			���j���[�n���h�� (InsertMenuA() �̑�1����)
 *	@param[in]	targetItemID	����ID�̃��j���[�̑O���͌��Ƀ��j���[��ǉ� (InsertMenuA() �̑�2����)
 *	@param[in]	flags			���j���[flag (InsertMenuA() �̑�3����)
 *	@param[in]	newItemID		���j���[ID (InsertMenuA() �̑�4����)
 *	@param[in]	text			���j���[������ (InsertMenuA() �̑�5����)
 *	@param[in]	before			TRUE/FALSE �O�ɒǉ�/���ɒǉ� (TRUE�̂Ƃ� InsertMenuA() �Ɠ�������)
 */
void TTInsertMenuItemA(HMENU hMenu, UINT targetItemID, UINT flags, UINT newItemID, const char *text, BOOL before)
{
	assert((flags & MF_BYPOSITION) == 0);
	for (int i = GetMenuItemCount(hMenu) - 1; i >= 0; i--) {
		HMENU submenu = GetSubMenu(hMenu, i);

		for (int j = GetMenuItemCount(submenu) - 1; j >= 0; j--) {
			if (GetMenuItemID(submenu, j) == targetItemID) {
				const UINT position = (before == FALSE) ? j + 1 : j;
				InsertMenuA(submenu, position, MF_BYPOSITION | flags, newItemID, text);
				return;
			}
		}
	}
}

/*
 *	������̉��s�R�[�h��CR(0x0d)�����ɂ���
 *
 *	@param [in]	*src	���͕�����ւ̃|�C���^
 *	@param [in] *len	���͕�����
 *						NULL �܂��� 0 �̂Ƃ������ŕ����񒷂𑪂邱�̎�L'\0'�͕K�{
 *	@param [out] *len	�o�͕�����(wcslen()�Ɠ���)
 *						NULL �̂Ƃ��o�͂���Ȃ�
 *	@return				�ϊ��㕶����(malloc()���ꂽ�̈�,free()���邱��)
 *						NULL ���������m�ۂł��Ȃ�����
 *
 *		���͕����񒷂̎w�肪���鎞
 *			���͕�����̓r���� L'\0' ������������A�����ŕϊ����I������
 *			������Ȃ��Ƃ��͓��͕��������ϊ�(�Ō��L'\0'�͕t������Ȃ�)
 */
wchar_t *NormalizeLineBreakCR(const wchar_t *src, size_t *len)
{
#define CR   0x0D
#define LF   0x0A
	size_t src_len = 0;
	if (len != NULL) {
		src_len = *len;
	}
	if (src_len == 0) {
		src_len = wcslen(src) + 1;
	}
	wchar_t *dest_top = (wchar_t *)malloc(sizeof(wchar_t) * src_len);
	if (dest_top == NULL) {
		*len = 0;
		return NULL;
	}

	const wchar_t *p = src;
	const wchar_t *p_end = src + src_len;
	wchar_t *dest = dest_top;
	BOOL find_eos = FALSE;
	while (p < p_end) {
		wchar_t c = *p++;
		if (c == CR) {
			if (*p == LF) {
				// CR+LF -> CR
				p++;
				*dest++ = CR;
			} else {
				// CR -> CR
				*dest++ = CR;
			}
		}
		else if (c == LF) {
			// LF -> CR
			*dest++ = CR;
		}
		else if (c == 0) {
			// EOS���������Ƃ��͑ł��؂�
			*dest++ = 0;
			find_eos = TRUE;
			break;
		}
		else {
			*dest++ = c;
		}
	}

	if (len != NULL) {
		*len = dest - dest_top;
		if (find_eos) {
			*len = *len - 1;
		}
	}
	return dest_top;
}

/**
 *	���s�R�[�h�� CR+LF �ɕϊ�����
 *	@return �ϊ����ꂽ������
 */
wchar_t *NormalizeLineBreakCRLF(const wchar_t *src_)
{
	const wchar_t *src = src_;
	wchar_t *dest_top;
	wchar_t *dest;
	size_t len, need_len, alloc_len;

	// �\��t���f�[�^�̒���(len)�A����ѐ��K����̃f�[�^�̒���(need_len)�̃J�E���g
	for (len=0, need_len=0, src=src_; *src != '\0'; src++, len++, need_len++) {
		if (*src == CR) {
			need_len++;
			if (*(src+1) == LF) {
				len++;
				src++;
			}
		}
		else if (*src == LF) {
			need_len++;
		}
	}

	// ���K������f�[�^�����ς��Ȃ� => ���K���͕K�v�Ȃ�
	if (need_len == len) {
		dest = _wcsdup(src_);
		return dest;
	}
	alloc_len = need_len + 1;

	dest_top = (wchar_t *)malloc(sizeof(wchar_t) * alloc_len);

	src = src_ + len - 1;
	dest = dest_top + need_len;
	*dest-- = '\0';

	while (len > 0 && dest_top <= dest) {
		if (*src == LF) {
			*dest-- = *src--;
			if (--len == 0) {
				*dest = CR;
				break;
			}
			if (*src != CR) {
				*dest-- = CR;
				continue;
			}
		}
		else if (*src == CR) {
			*dest-- = LF;
			if (src == dest)
				break;
		}
		*dest-- = *src--;
		len--;
	}

	return dest_top;
}

unsigned long long GetFSize64H(HANDLE hFile)
{
	DWORD file_size_hi;
	DWORD file_size_low;
	file_size_low = GetFileSize(hFile, &file_size_hi);
	if (file_size_low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
		// error
		return 0;
	}
	unsigned long long file_size = ((unsigned long long)file_size_hi << 32) + file_size_low;
	return file_size;
}

unsigned long long GetFSize64W(const wchar_t *FName)
{
	HANDLE hFile = CreateFileW(FName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	unsigned long long file_size = GetFSize64H(hFile);
	CloseHandle(hFile);
	return file_size;
}

unsigned long long GetFSize64A(const char *FName)
{
	HANDLE hFile = CreateFileA(FName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}
	unsigned long long file_size = GetFSize64H(hFile);
	CloseHandle(hFile);
	return file_size;
}

// Append a slash to the end of a path name
void AppendSlashW(wchar_t *Path, size_t destlen)
{
	size_t len = wcslen(Path);
	if (len > 0) {
		if (Path[len - 1] != L'\\') {
			wcsncat_s(Path,destlen,L"\\",_TRUNCATE);
		}
	}
}

/**
 *	�t�@�C����(�p�X��)����͂���
 *	GetFileNamePos() �� UTF-8��
 *
 *	@param[in]	PathName	�t�@�C�����A�t���p�X
 *	@param[out]	DirLen		�����̃X���b�V�����܂ރf�B���N�g���p�X��
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@param[out]	FNPos		�t�@�C�����ւ�index
 *							&PathName[FNPos] ���t�@�C����
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@retval		FALSE		PathName���s��
 */
BOOL GetFileNamePosU8(const char *PathName, int *DirLen, int *FNPos)
{
	BYTE b;
	const char *Ptr;
	const char *DirPtr;
	const char *FNPtr;
	const char *PtrOld;

	if (DirLen != NULL) *DirLen = 0;
	if (FNPos != NULL) *FNPos = 0;

	if (PathName==NULL)
		return FALSE;

	if ((strlen(PathName)>=2) && (PathName[1]==':'))
		Ptr = &PathName[2];
	else
		Ptr = PathName;
	if (Ptr[0]=='\\' || Ptr[0]=='/')
		Ptr++;

	DirPtr = Ptr;
	FNPtr = Ptr;
	while (Ptr[0]!=0) {
		b = Ptr[0];
		PtrOld = Ptr;
		Ptr++;
		switch (b) {
			case ':':
				return FALSE;
			case '/':	/* FALLTHROUGH */
			case '\\':
				DirPtr = PtrOld;
				FNPtr = Ptr;
				break;
		}
	}
	if (DirLen != NULL) *DirLen = DirPtr-PathName;
	if (FNPos != NULL) *FNPos = FNPtr-PathName;
	return TRUE;
}

/**
 *	�t�@�C����(�p�X��)����͂���
 *	GetFileNamePos() �� wchar_t��
 *
 *	@param[in]	PathName	�t�@�C�����A�t���p�X
 *	@param[out]	DirLen		�����̃X���b�V�����܂ރf�B���N�g���p�X��
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@param[out]	FNPos		�t�@�C�����ւ�index
 *							&PathName[FNPos] ���t�@�C����
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@retval		FALSE		PathName���s��
 */
BOOL GetFileNamePosW(const wchar_t *PathName, size_t *DirLen, size_t *FNPos)
{
	const wchar_t *Ptr;
	const wchar_t *DirPtr;
	const wchar_t *FNPtr;
	const wchar_t *PtrOld;

	if (DirLen != NULL) *DirLen = 0;
	if (FNPos != NULL) *FNPos = 0;

	if (PathName==NULL)
		return FALSE;

	if ((wcslen(PathName)>=2) && (PathName[1]==L':'))
		Ptr = &PathName[2];
	else
		Ptr = PathName;
	if (Ptr[0]=='\\' || Ptr[0]=='/')
		Ptr++;

	DirPtr = Ptr;
	FNPtr = Ptr;
	while (Ptr[0]!=0) {
		wchar_t b = Ptr[0];
		PtrOld = Ptr;
		Ptr++;
		switch (b) {
			case L':':
				return FALSE;
			case L'/':	/* FALLTHROUGH */
			case L'\\':
				DirPtr = PtrOld;
				FNPtr = Ptr;
				break;
		}
	}
	if (DirLen != NULL) *DirLen = DirPtr-PathName;
	if (FNPos != NULL) *FNPos = FNPtr-PathName;
	return TRUE;
}

/**
 *	ConvHexCharW() �� wchar_t ��
 */
BYTE ConvHexCharW(wchar_t b)
{
	if ((b>='0') && (b<='9')) {
		return (b - 0x30);
	}
	else if ((b>='A') && (b<='F')) {
		return (b - 0x37);
	}
	else if ((b>='a') && (b<='f')) {
		return (b - 0x57);
	}
	else {
		return 0;
	}
}

/**
 *	Hex2Str() �� wchar_t ��
 */
int Hex2StrW(const wchar_t *Hex, wchar_t *Str, size_t MaxLen)
{
	wchar_t b, c;
	size_t i, imax, j;

	j = 0;
	imax = wcslen(Hex);
	i = 0;
	while ((i < imax) && (j<MaxLen)) {
		b = Hex[i];
		if (b=='$') {
			i++;
			if (i < imax) {
				c = Hex[i];
			}
			else {
				c = 0x30;
			}
			b = ConvHexCharW(c) << 4;
			i++;
			if (i < imax) {
				c = (BYTE)Hex[i];
			}
			else {
				c = 0x30;
			}
			b = b + ConvHexCharW(c);
		};

		Str[j] = b;
		j++;
		i++;
	}
	if (j<MaxLen) {
		Str[j] = 0;
	}

	return (int)j;
}

/**
 *	ExtractFileName() �� wchar_t ��
 *	�t���p�X����t�@�C�������������o��
 *
 *	@return	�t�@�C��������(�s�v�ɂȂ�����free()����)
 */
wchar_t *ExtractFileNameW(const wchar_t *PathName)
{
	size_t i;
	if (!GetFileNamePosW(PathName, NULL, &i))
		return NULL;
	wchar_t *filename = _wcsdup(&PathName[i]);
	return filename;
}

/**
 *	ExtractDirName() �� wchar_t ��
 *
 *	@return	�f�B���N�g��������(�s�v�ɂȂ�����free()����)
 */
wchar_t *ExtractDirNameW(const wchar_t *PathName)
{
	size_t i;
	wchar_t *DirName = _wcsdup(PathName);
	if (!GetFileNamePosW(DirName, &i, NULL))
		return NULL;
	DirName[i] = 0;
	return DirName;
}

/*
 * Get Exe(exe,dll) directory
 *	ttermpro.exe, �v���O�C��������t�H���_
 *	ttypes.ExeDirW �Ɠ���
 *	���Ƃ� GetHomeDirW() ������
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			ExeDir		�s�v�ɂȂ����� free() ���邱��
 */
wchar_t *GetExeDirW(HINSTANCE hInst)
{
	wchar_t *TempW;
	wchar_t *dir;
	DWORD error = hGetModuleFileNameW(hInst, &TempW);
	if (error != NO_ERROR) {
		// �p�X�̎擾�Ɏ��s�����B�v���I�Aabort() ����B
		abort();
	}
	dir = ExtractDirNameW(TempW);
	free(TempW);
	return dir;
}

/*
 * Get home directory
 *		�l�p�ݒ�t�@�C���t�H���_�擾
 *		ttypes.HomeDirW �Ɠ���
 *		TERATERM.INI �Ȃǂ������Ă���t�H���_
 *		ttermpro.exe ������t�H���_�� GetExeDirW() �Ŏ擾
 *		%APPDATA%\teraterm5 (%USERPROFILE%\AppData\Roaming\teraterm5)
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			HomeDir		�s�v�ɂȂ����� free() ���邱��
 */
wchar_t *GetHomeDirW(HINSTANCE hInst)
{
	wchar_t *path;
	_SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path);
	wchar_t *ret = NULL;
	awcscats(&ret, path, L"\\teraterm5", NULL);
	free(path);
	return ret;
}

/*
 * Get log directory
 *		���O�ۑ��t�H���_�擾
 *		ttypes.LogDirW �Ɠ���
 *		%LOCALAPPDATA%\teraterm5 (%USERPROFILE%\AppData\Local\teraterm5)
 *
 * @param[in]		hInst		WinMain()�� HINSTANCE �܂��� NULL
 * @return			LogDir		�s�v�ɂȂ����� free() ���邱��
 */
wchar_t* GetLogDirW(void)
{
	wchar_t *path;
	_SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
	wchar_t *ret = NULL;
	awcscats(&ret, path, L"\\teraterm5", NULL);
	free(path);
	return ret;
}

/*
 *	UILanguageFile�̃t���p�X���擾����
 *
 *	@param[in]		HomeDir					exe,dll�̑��݂���t�H���_ GetExeDir()�Ŏ擾�ł���
 *	@param[in]		UILanguageFileRel		lng�t�@�C���AHomeDir����̑��΃p�X
 *	@param[in,out]	UILanguageFileFull		lng�t�@�C��ptr�A�t���p�X
 *	@param[in]		UILanguageFileFullLen	lng�t�@�C��len�A�t���p�X
 */
wchar_t *GetUILanguageFileFullW(const wchar_t *HomeDir, const wchar_t *UILanguageFileRel)
{
	wchar_t *fullpath;
	size_t size = wcslen(HomeDir) + 1 + wcslen(UILanguageFileRel) + 1;
	wchar_t *rel = (wchar_t *)malloc(sizeof(wchar_t) * size);
	wcscpy(rel, HomeDir);
	wcscat(rel, L"\\");
	wcscat(rel, UILanguageFileRel);
	hGetFullPathNameW(rel, &fullpath, NULL);
	free(rel);
	return fullpath;
}

/**
 *	�ݒ�t�@�C���̃t���p�X���擾����
 *
 *	@param[in]	home	ttermpro.exe ���̎��s�t�@�C���̂���t�H���_
 *						My Documents �Ƀt�@�C�����������ꍇ�͎g�p����Ȃ�
 *	@param[in]	file	�ݒ�t�@�C����(�p�X�͊܂܂Ȃ�)
 *	@return		�t���p�X (�s�v�ɂȂ����� free() ���邱��)
 *
 *	- My Documents �Ƀt�@�C�����������ꍇ,
 *		- "%USERPROFILE%\My Documents\{file}"
 *		- "%USERPROFILE%\Documents\{file}" �Ȃ�
 *	- �Ȃ������ꍇ
 *		- "{home}\{file}"
 */
wchar_t *GetDefaultFNameW(const wchar_t *home, const wchar_t *file)
{
	// My Documents �� file ������ꍇ�A
	// �����ǂݍ��ނ悤�ɂ����B(2007.2.18 maya)
	wchar_t *MyDoc;
	_SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &MyDoc);

	if (MyDoc[0] != 0) {
		// My Documents �� file �����邩?
		size_t destlen = (wcslen(MyDoc) + wcslen(file) + 1 + 1) * sizeof(wchar_t);
		wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * destlen);
		wcscpy(dest, MyDoc);
		AppendSlashW(dest,destlen);
		wcsncat_s(dest, destlen, file, _TRUNCATE);
		DWORD r = GetFileAttributesW(dest);
		free(MyDoc);
		if (r != INVALID_FILE_ATTRIBUTES) {
			// My Documents �̐ݒ�t�@�C��
			return dest;
		}
		free(dest);
	}
	else {
		free(MyDoc);
	}

	size_t destlen = (wcslen(home) + wcslen(file) + 1 + 1) * sizeof(wchar_t);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * destlen);
	wcscpy(dest, home);
	AppendSlashW(dest,destlen);
	wcsncat_s(dest, destlen, file, _TRUNCATE);
	return dest;
}

// �f�t�H���g�� TERATERM.INI �̃t���p�X���擾
wchar_t *GetDefaultSetupFNameW(const wchar_t *home)
{
	const wchar_t *ini = L"TERATERM.INI";
	wchar_t *buf = GetDefaultFNameW(home, ini);
	return buf;
}

/**
 *	�G�X�P�[�v��������������
 *	\\,\n,\t,\0 ��u��������
 *	@return		�������iL'\0'���܂�)
 */
size_t RestoreNewLineW(wchar_t *Text)
{
	size_t i;
	int j=0;
	size_t size= wcslen(Text);
	wchar_t *buf = (wchar_t *)_alloca((size+1) * sizeof(wchar_t));

	memset(buf, 0, (size+1) * sizeof(wchar_t));
	for (i=0; i<size; i++) {
		if (Text[i] == L'\\' && i<size ) {
			switch (Text[i+1]) {
				case L'\\':
					buf[j] = L'\\';
					i++;
					break;
				case L'n':
					buf[j] = L'\n';
					i++;
					break;
				case L't':
					buf[j] = L'\t';
					i++;
					break;
				case L'0':
					buf[j] = L'\0';
					i++;
					break;
				default:
					buf[j] = L'\\';
			}
			j++;
		}
		else {
			buf[j] = Text[i];
			j++;
		}
	}
	/* use memcpy to copy with '\0' */
	j++;	// ������
	memcpy(Text, buf, j * sizeof(wchar_t));
	return j;
}

BOOL GetNthString(PCHAR Source, int Nth, int Size, PCHAR Dest)
{
	int i, j, k;

	i = 1;
	j = 0;
	k = 0;

	while (i<Nth && Source[j] != 0) {
		if (Source[j++] == ',') {
			i++;
		}
	}

	if (i == Nth) {
		while (Source[j] != 0 && Source[j] != ',' && k<Size-1) {
			Dest[k++] = Source[j++];
		}
	}

	Dest[k] = 0;
	return (i>=Nth);
}

void GetNthNum(PCHAR Source, int Nth, int far *Num)
{
	char T[15];

	GetNthString(Source,Nth,sizeof(T),T);
	if (sscanf_s(T, "%d", Num) != 1) {
		*Num = 0;
	}
}

int GetNthNum2(PCHAR Source, int Nth, int defval)
{
	char T[15];
	int v;

	GetNthString(Source, Nth, sizeof(T), T);
	if (sscanf_s(T, "%d", &v) != 1) {
		v = defval;
	}

	return v;
}

wchar_t *GetDownloadFolderW(void)
{
	wchar_t *download;
	_SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &download);
	return download;
}

#if IFILEOPENDIALOG_ENABLE
static BOOL doSelectFolderWCOM(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder)
{
	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void **)&pDialog);
	if (FAILED(hr)) {
		*folder = NULL;
		return FALSE;
	}

	DWORD options;
	pDialog->GetOptions(&options);
	pDialog->SetOptions(options | FOS_PICKFOLDERS);
	pDialog->SetTitle(msg);
	{
		IShellItem *psi;
		hr = SHCreateItemFromParsingName(def, NULL, IID_IShellItem, (void **)&psi);
		if (SUCCEEDED(hr)) {
			hr = pDialog->SetFolder(psi);
			psi->Release();
		}
	}
	hr = pDialog->Show(hWnd);

	BOOL result = FALSE;
	if (SUCCEEDED(hr)) {
		IShellItem *pItem;
		hr = pDialog->GetResult(&pItem);
		if (SUCCEEDED(hr)) {
			PWSTR pPath;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
			if (SUCCEEDED(hr)) {
				*folder = _wcsdup(pPath);
				CoTaskMemFree(pPath);
				result = TRUE;
			}
		}
	}

	if (!result) {
		// cancel(or some error)
		*folder = NULL;
	}
	pDialog->Release();
	return result;
}
#else
static int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch(uMsg) {
	case BFFM_INITIALIZED: {
		// ��������

		const wchar_t *folder = (wchar_t *)lpData;
		if (folder == NULL || folder[0] == 0) {
			// �I���t�H���_���w�肳��Ă��Ȃ�
			break;
		}
		// �t�H���_��I����Ԃɂ���
		SendMessageW(hwnd, BFFM_SETSELECTIONW, (WPARAM)TRUE, (LPARAM)folder);
		break;
	}
	default:
		break;
	}
	return 0;
}

static BOOL doSelectFolderWAPI(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder)
{
	wchar_t buf[MAX_PATH];
	BROWSEINFOW bi = {};
	bi.hwndOwner = hWnd;
	bi.pidlRoot = 0;	// 0 = from desktop
	bi.pszDisplayName = buf;
	bi.lpszTitle = msg;
	bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallback;
	bi.lParam = (LPARAM)def;
	LPITEMIDLIST pidlBrowse = SHBrowseForFolderW(&bi);
	if (pidlBrowse == NULL) {
		*folder = NULL;
		return FALSE;
	}

	// PIDL�`���̖߂�l�̃t�@�C���V�X�e���̃p�X�ɕϊ�
	// TODO SHGetPathFromIDListEx() �֐؂�ւ�?
	if (!SHGetPathFromIDListW(pidlBrowse, buf)) {
		return FALSE;
	}
	*folder = _wcsdup(buf);
	CoTaskMemFree(pidlBrowse);
	return TRUE;
}
#endif

/**
 *	�t�H���_��I������
 *	SHBrowseForFolderW() ���R�[������
 *
 *	@param[in]	def			�I���t�H���_�̏����l(���Ɏw�肵�Ȃ��Ƃ��� NULL or "")
 *	@param[out]	**folder	�I�������t�H���_�̃t���p�X(�L�����Z�����̓Z�b�g����Ȃ�)
 *							�s�v�ɂȂ����� free() ���邱��(�L�����Z������free()�s�v)
 *	@retval	TRUE	�I������
 *	@retval	FALSE	�L�����Z������
 *
 */
BOOL doSelectFolderW(HWND hWnd, const wchar_t *def, const wchar_t *msg, wchar_t **folder)
{
	// TODO ����������
#if IFILEOPENDIALOG_ENABLE
	return doSelectFolderWCOM(hWnd, def, msg, folder);
#else
	return doSelectFolderWAPI(hWnd, def, msg, folder);
#endif
}

/* fit a filename to the windows-filename format */
/* FileName must contain filename part only. */
void FitFileNameW(wchar_t *FileName, size_t destlen, const wchar_t *DefExt)
{
	int i, j, NumOfDots;
	wchar_t Temp[MAX_PATH];
	wchar_t b;

	NumOfDots = 0;
	i = 0;
	j = 0;
	/* filename started with a dot is illeagal */
	if (FileName[0]==L'.') {
		Temp[0] = L'_';  /* insert an underscore char */
		j++;
	}

	do {
		b = FileName[i];
		i++;
		if (b==L'.')
			NumOfDots++;
		if ((b!=0) &&
		    (j < MAX_PATH-1)) {
			Temp[j] = b;
			j++;
		}
	} while (b!=0);
	Temp[j] = 0;

	if ((NumOfDots==0) &&
	    (DefExt!=NULL)) {
		/* add the default extension */
		wcsncat_s(Temp,_countof(Temp),DefExt,_TRUNCATE);
	}

	wcsncpy_s(FileName,destlen,Temp,_TRUNCATE);
}

void ConvFNameW(const wchar_t *HomeDir, wchar_t *Temp, size_t templen, const wchar_t *DefExt, wchar_t *FName, size_t destlen)
{
	// destlen = sizeof FName
	size_t DirLen, FNPos;

	FName[0] = 0;
	if ( ! GetFileNamePosW(Temp,&DirLen,&FNPos) ) {
		return;
	}
	FitFileNameW(&Temp[FNPos],templen - FNPos,DefExt);
	if ( DirLen==0 ) {
		wcsncpy_s(FName,destlen,HomeDir,_TRUNCATE);
		AppendSlashW(FName,destlen);
	}
	wcsncat_s(FName,destlen,Temp,_TRUNCATE);
}

/**
 *	path�����΃p�X���ǂ�����Ԃ�
 *		TODO "\path\path" �� ���΃p�X�ł͂Ȃ��̂ł͂Ȃ���?
 */
BOOL IsRelativePathW(const wchar_t *path)
{
	if (path[0] == '\\' || path[0] == '/' || path[0] != '\0' && path[1] == ':') {
		return FALSE;
	}
	return TRUE;
}

BOOL IsRelativePathA(const char *path)
{
	if (path[0] == '\\' || path[0] == '/' || path[0] != '\0' && path[1] == ':') {
		return FALSE;
	}
	return TRUE;
}

// OS�� �w�肳�ꂽ�o�[�W�����Ɠ����� ���ǂ����𔻕ʂ���B
BOOL IsWindowsVer(DWORD dwPlatformId, DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEXA osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwPlatformId = dwPlatformId;
	osvi.dwMajorVersion = dwMajorVersion;
	osvi.dwMinorVersion = dwMinorVersion;
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_PLATFORMID, op);
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, op);
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, op);
	ret = _VerifyVersionInfoA(&osvi, VER_PLATFORMID | VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ret);
}

// OS�� �w�肳�ꂽ�o�[�W�����ȍ~ ���ǂ����𔻕ʂ���B
//   dwPlatformId �����Ă��Ȃ��̂� NT �J�[�l�����ł�����r�ł��Ȃ�
//   5.0 �ȏ�Ŕ�r���邱��
BOOL IsWindowsVerOrLater(DWORD dwMajorVersion, DWORD dwMinorVersion)
{
	OSVERSIONINFOEXA osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_GREATER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = dwMajorVersion;
	osvi.dwMinorVersion = dwMinorVersion;
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MAJORVERSION, op);
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, op);
	ret = _VerifyVersionInfoA(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
	return (ret);
}

// OS�� Windows XP �ȍ~ ���ǂ����𔻕ʂ���B
//
// return TRUE:  XP or later
//        FALSE: 2000 or earlier
BOOL IsWindowsXPOrLater(void)
{
	return IsWindowsVerOrLater(5, 1);
}

/**
 *	DeleteComment �� wchar_t ��
 */
wchar_t *DeleteCommentW(const wchar_t *src)
{
	size_t dest_size = wcslen(src);
	wchar_t *dest = (wchar_t *)malloc(sizeof(wchar_t) * (dest_size + 1));
	wchar_t *dest_top = dest;
	BOOL quoted = FALSE;
	wchar_t *dest_end = dest + dest_size - 1;

	while (*src != '\0' && dest < dest_end && (quoted || *src != ';')) {
		*dest++ = *src;

		if (*src++ == '"') {
			if (*src == '"' && dest < dest_end) {
				*dest++ = *src++;
			}
			else {
				quoted = !quoted;
			}
		}
	}

	*dest = '\0';

	return dest_top;
}
