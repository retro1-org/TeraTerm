/*
 * Copyright (C) 2021- TeraTerm Project
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
#include <stdlib.h>
#include <direct.h>
#include <locale.h>
#include <string.h>
#include <errno.h>

#include "ttlib.h"
#include "asprintf.h"

#include "cyglib.h"

/**
 *	cygwin1.dll��T��
 *
 *	@param[in]	cygwin_dir		(���݂���ł��낤)�t�H���_
 *	@param[out]	find_dir		���������t�H���_ free() ���邱��
 *	@param[out]	find_in_path	���ϐ� PATH ���Ɍ�������
 *
 *	@retval		TRUE	��������
 *	@retval		FALSE	������Ȃ�
 */
static BOOL CygwinSearchDLL(const wchar_t *cygwin_dir, wchar_t **find_dir, BOOL *find_in_path)
{
	wchar_t file[MAX_PATH];
	wchar_t *filename;
	wchar_t c;
#if 1
	const wchar_t *dll_base = L"cygwin1";
	const wchar_t *search_paths[] = {
		L"%c:\\cygwin\\bin",
		L"%c:\\cygwin64\\bin",
		NULL,
	};
#endif
#if 0
	const wchar_t *dll_base = L"msys-2.0";
	const wchar_t *search_paths[] = {
		L"%c:\\msys\\usr\\bin",
		L"%c:\\msys64\\usr\\bin",
		NULL,
	};
#endif
	wchar_t *dll;
	int i;
	DWORD r;

	*find_in_path = FALSE;
	*find_dir = NULL;

	// �w�肳�ꂽ�t�H���_�ɑ��݂��邩?
	if (cygwin_dir != NULL && cygwin_dir[0] != 0) {
		// SearchPathW() �ŒT��
		dll = NULL;
		awcscats(&dll, L"bin\\", dll_base, L".dll", NULL);
		r = SearchPathW(cygwin_dir, dll, L".dll", _countof(file), file, &filename);
		free(dll);
		if (r > 0) {
			goto found_dll;
		}

		// SearchPathW() ���� "msys-2.0.dll" �������邱�Ƃ��ł��Ȃ� (Windows 10)
		dll = NULL;
		awcscats(&dll, cygwin_dir, L"\\", dll_base, L".dll", NULL);
		r = GetFileAttributesW(dll);
		if (r != INVALID_FILE_ATTRIBUTES) {
			// ��������
			wcscpy_s(file, _countof(file), dll);
			free(dll);
			goto found_dll;
		}
		free(dll);
	}

	// PATH ����T��
	if (SearchPathW(NULL, dll_base, L".dll", _countof(file), file, &filename) > 0) {
		*find_in_path = TRUE;
		goto found_dll;
	}

	// ���肻���ȏꏊ��T��
	for (c = 'C' ; c <= 'Z' ; c++) {
		for (i = 0; search_paths[i] != NULL; i++) {
			// SearchPathW() �ŒT��
			const wchar_t *search_path_base = search_paths[i];
			wchar_t *search_path;
			aswprintf(&search_path, search_path_base, c);
			r = SearchPathW(search_path, dll_base, L".dll", _countof(file), file, &filename);
			if (r > 0) {
				free(search_path);
				goto found_dll;
			}

			// �t�@�C�������݂��邩���ׂ�
			dll = NULL;
			awcscats(&dll, search_path, L"\\", dll_base, L".dll", NULL);
			r = GetFileAttributesW(dll);
			free(search_path);
			if (r != INVALID_FILE_ATTRIBUTES) {
				wcscpy_s(file, _countof(file), dll);
				free(dll);
				goto found_dll;
			}
			free(dll);
		}
	}

	// ������Ȃ�����
	return FALSE;

found_dll:
	{
		// cut "cygwin1.dll", �t�H���_�݂̂�Ԃ�
		wchar_t *p = wcsrchr(file, L'\\');
		*p = 0;
	}

	*find_dir = _wcsdup(file);
	return TRUE;
}

static errno_t __wdupenv_s(wchar_t** envptr, size_t* buf_size, const wchar_t* name)
{
#if defined(_MSC_VER)
	return _wdupenv_s(envptr, buf_size, name);
#else
    const wchar_t* s = _wgetenv(name);
	if (s == NULL) {
		// ���݂��Ȃ�
		*envptr = NULL;
		return EINVAL;
	}
	*envptr = _wcsdup(s);
	if (buf_size != NULL) {
		*buf_size = wcslen(*envptr);
	}
	return 0;
#endif
}

/**
 *	���ϐ� PATH �� add_path ��ǉ�
 */
static BOOL AddPath(const wchar_t *add_path)
{
	wchar_t *envptr;
	wchar_t *new_env;
	int r;
	errno_t e;

	e = __wdupenv_s(&envptr, NULL, L"PATH");
	if (e == 0) {
		aswprintf(&new_env, L"PATH=%s;%s", add_path, envptr);
		free(envptr);
	}
	else {
		// ���ϐ� PATH �����݂��Ȃ�
		aswprintf(&new_env, L"PATH=%s", add_path);
	}
	r = _wputenv(new_env);
	free(new_env);
	return r == 0 ? TRUE : FALSE;
}

/**
 *	Connect to local cygwin
 *	cygterm�����s
 *
 *	@param[in]	CygwinDirectory		Cygwin���C���X�g�[�����Ă���t�H���_
 *									������Ȃ���΃f�t�H���g�t�H���_�Ȃǂ�T��
 *	@param[in]	cmdline				cygterm�ɓn���R�}���h���C������
 *									NULL�̂Ƃ������Ȃ�
 *	@retval		NO_ERROR					���s�ł���
 *	@retval		ERROR_FILE_NOT_FOUND		cygwin��������Ȃ�(cygwin1.dll��������Ȃ�)
 *	@retval		ERROR_NOT_ENOUGH_MEMORY		�������s��
 *	@retval		ERROR_OPEN_FAILED			���s�ł��Ȃ�
 */
DWORD CygwinConnect(const wchar_t *CygwinDirectory, const wchar_t *cmdline)
{
	BOOL find_cygwin;
	wchar_t *find_dir;
	BOOL find_in_path;
	wchar_t *ExeDirW;
	wchar_t *cygterm_cmd;
	DWORD e;
	const wchar_t *cygterm_exe = L"cygterm.exe";
//	const wchar_t *cygterm_exe = L"msys2term.exe";

//	CygwinDirectory = NULL;
	find_cygwin = CygwinSearchDLL(CygwinDirectory, &find_dir, &find_in_path);
	if (find_cygwin == FALSE) {
		return ERROR_FILE_NOT_FOUND;
	}

	if (!find_in_path) {
		// ���ϐ� PATH �ɒǉ�
		// cygterm.exe �����s����Ƃ��� cygwin1.dll �����[�h�ł���悤�ɂ���
		BOOL r = AddPath(find_dir);
		if (r == FALSE) {
			free(find_dir);
			return ERROR_NOT_ENOUGH_MEMORY;
		}
	}
	free(find_dir);

	ExeDirW = GetExeDirW(NULL);
	cygterm_cmd = NULL;
	awcscats(&cygterm_cmd, ExeDirW, L"\\", cygterm_exe, NULL);
	if (cmdline != NULL && cmdline[0] != 0) {
		awcscats(&cygterm_cmd, L" ", cmdline, NULL);
	}

	e = TTWinExec(cygterm_cmd);
	free(cygterm_cmd);
	if (e != NO_ERROR) {
		return ERROR_OPEN_FAILED;
	}

	return NO_ERROR;
}
