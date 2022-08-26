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

#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "tttypes.h"
#include "ttlib.h"
#include "codeconv.h"
#include "compat_win.h"
#include "win32helper.h"
#include "asprintf.h"
#include "fileread.h"
#include "ttcmn_dup.h"
#include "ttcommon.h"
#include "ttcmn_shared_memory.h"
#include "ttcmn_i.h"

#include "ttcmn_static.h"

// TMap ���i�[����t�@�C���}�b�s���O�I�u�W�F�N�g(���L������)�̖��O
// TMap(�Ƃ��̃����o)�̍X�V���͋��o�[�W�����Ƃ̓����N���ׂ̈ɕς���K�v�����邪
// �A�Ԃ���o�[�W�����ԍ����g���悤�ɕύX�����ׁA�ʏ�͎蓮�ŕύX����K�v�͖���
#define TT_FILEMAPNAME "ttset_memfilemap_" TT_VERSION_STR("_")

static PMap pm;

static const char DupDataName[] = "dupdata";

void CopyShmemToTTSet(PTTSet ts)
{
	DWORD size = pm->DuplicateDataSizeLow;
	HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, DupDataName);
	void *ptr = (void *)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	TTCMNUnserialize(ptr, (size_t)size, ts);
	UnmapViewOfFile(ptr);
	CloseHandle(handle);
	pm->DuplicateDataSizeLow = 0;
}

void CopyTTSetToShmem(PTTSet ts)
{
	size_t size;
	void *ptr = TTCMNSerialize(ts, &size);
	if (ptr != NULL) {
		HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, DupDataName);
		void *dest = (void *)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS,0,0,0);
		memcpy(dest, ptr, size);
		UnmapViewOfFile(dest);
		//CloseHandle(handle);	do not close, �g�p���Ȃ���ԂɂȂ�ƂȂ��Ȃ��Ă��܂�

		if (pm->DuplicateDataHandle != NULL) {
			// 1��O�̏����폜����
			CloseHandle(pm->DuplicateDataHandle);
		}
		pm->DuplicateDataHandle = handle;
		pm->DuplicateDataSizeLow = (DWORD)size;
		free(ptr);
	}
}

static void CopyFiles(const wchar_t *file_list[], const wchar_t *src_dir, const wchar_t *dest_dir)
{
	for (;;) {
		wchar_t *dest;
		size_t len;
		const wchar_t *filename = *file_list;
		file_list++;
		if (filename == NULL) {
			break;
		}

		dest = NULL;
		awcscats(&dest, dest_dir, L"\\", filename, NULL);

		len = wcslen(dest);
		if (dest[len - 1] == '\\') {
			// �t�H���_�쐬
			CreateDirectoryW(dest, NULL);
		}
		else {
			wchar_t *src = NULL;
			awcscats(&src, src_dir, L"\\", filename, NULL);
			CopyFileW(src, dest, TRUE);		// TRUE = do not copy if exists
			free(src);
		}
		free(dest);
	}
}

static void ConvertIniFiles(const wchar_t *filelist[],  const wchar_t *dir, const wchar_t *date_str)
{
	while(1) {
		wchar_t *fname;
		if (*filelist == NULL) {
			break;
		}

		fname = NULL;
		awcscats(&fname, dir, L"\\", *filelist, NULL);
		ConvertIniFileCharCode(fname, date_str);
		free(fname);
		filelist++;
	}
}

/*
 *	@return		�G���[���L��ꍇ FALSE
 *	@param[in]	BOOL first_instance
 */
BOOL OpenSharedMemory(BOOL *first_instance_, PMap *pm_, HANDLE *HMap_)
{
	int i;

	for (i = 0; i < 100; i++) {
		char tmp[32];
		HANDLE hMap;
		BOOL first_instance;
		TMap *map;
		_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, i == 0 ? "%s" : "%s_%d", TT_FILEMAPNAME, i);
		hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
								 0, sizeof(TMap), tmp);
		if (hMap == NULL) {
			return FALSE;
		}

		first_instance = (GetLastError() != ERROR_ALREADY_EXISTS);

		map = (TMap *)MapViewOfFile(hMap,FILE_MAP_WRITE,0,0,0);
		if (map == NULL) {
			return FALSE;
		}

		if (first_instance ||
			(map->size_tmap == sizeof(TMap) &&
			 map->size_tttset == sizeof(TTTSet)))
		{
			map->size_tmap = sizeof(TMap);
			map->size_tttset = sizeof(TTTSet);
			*HMap_ = hMap;
			*pm_ = map;
			*first_instance_ = first_instance;
			return TRUE;
		}

		// next try
		UnmapViewOfFile(map);
		CloseHandle(hMap);
	}
	return FALSE;
}

void CloseSharedMemory(PMap pm, HANDLE HMap)
{
	UnmapViewOfFile(pm);
	CloseHandle(HMap);
}

static BOOL FirstInstance;
static HANDLE HMap = NULL;

BOOL StartTeraTerm(HINSTANCE hInst, PTTSet ts)
{
	OpenSharedMemory(&FirstInstance, &pm, &HMap);
	SetPMPtr(pm);

	if (FirstInstance) {
		// init window list
		pm->NWin = 0;
	}

	// if (FirstInstance) { �̕�������ړ� (2008.3.13 maya)
	// �N�����ɂ́A���L�������� HomeDir �� SetupFName �͋�ɂȂ�
	/* Get home directory (ttermpro.exe�̃t�H���_) */
	ts->ExeDirW = GetExeDirW(hInst);

	// LogDir
	ts->LogDirW = GetLogDirW(hInst);

	// HomeDir
	ts->HomeDirW = GetHomeDirW(hInst);
	WideCharToACP_t(ts->HomeDirW, ts->HomeDir, _countof(ts->HomeDir));
	SetCurrentDirectoryW(ts->HomeDirW);		// TODO �K�v??

	// TERATERM.INI �̃t���p�X
	ts->SetupFNameW = NULL;
	awcscats(&ts->SetupFNameW, ts->HomeDirW, L"\\TERATERM.INI", NULL);
	WideCharToACP_t(ts->SetupFNameW, ts->SetupFName, _countof(ts->SetupFName));

	// KEYBOARD.CNF �̃t���p�X
	ts->KeyCnfFNW = NULL;
	awcscats(&ts->KeyCnfFNW, ts->HomeDirW, L"\\KEYBOARD.CNF", NULL);
	WideCharToACP_t(ts->KeyCnfFNW, ts->KeyCnfFN, _countof(ts->KeyCnfFN));

	// �|�[�^�u�����[�h�ł͂Ȃ��A
	// �l�ݒ�t�@�C���̂���t�H���_ HomeDirW �� TERATERM.INI �����݂��Ȃ��Ƃ�
	if (!IsPortableMode() &&
		(GetFileAttributesW(ts->SetupFNameW) == INVALID_FILE_ATTRIBUTES)) {
		// �ݒ�t�@�C�����l�t�H���_�փR�s�[����
		static const wchar_t *filelist[] = {
			L"TERATERM.INI",
			L"KEYBOARD.CNF",
			L"cygterm.cfg",
			L"ssh_known_hosts",
			L"theme\\",
			L"theme\\Advanced.sample",
			L"theme\\ImageFile.INI",
			L"theme\\scale\\",
			L"theme\\scale\\23.jpg",
			L"theme\\scale\\43.jpg",
			L"theme\\Scale.INI",
			L"theme\\tile\\",
			L"theme\\tile\\03.jpg",
			L"theme\\tile\\44.jpg",
			L"theme\\Tile.INI",
			NULL,
		};
		CopyFiles(filelist, ts->ExeDirW, ts->HomeDirW);
	}

	// ini�t�@�C���̕����R�[�h��ϊ�����
	{
		static const wchar_t *filelist[] = {
			L"TERATERM.INI",
			L"KEYBOARD.CNF",
			NULL,
		};

		// backup �t�@�C���ɂ�����t������
		wchar_t *date_str = MakeISO8601Str(0);
		awcscat(&date_str, L"_");

		// ini�t�@�C����ϊ�����
		ConvertIniFiles(filelist, ts->HomeDirW, date_str);

		free(date_str);
	}

	ts->PluginVTIconInstance = NULL;
	ts->PluginVTIconID = 0;

	if (FirstInstance) {
		FirstInstance = FALSE;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

// �ݒ�t�@�C�����f�B�X�N�ɕۑ����ATera Term�{�̂��ċN������B
// (2012.4.30 yutaka)
// �g���Ă��Ȃ�?
#if 0
void WINAPI RestartTeraTerm(HWND hwnd, PTTSet ts)
{
	wchar_t *path;
	int ret;

	static const TTMessageBoxInfoW info = {
		"Tera Term",
		NULL, L"Tera Term: Configuration Warning",
		"MSG_TT_TAKE_EFFECT",
		L"This option takes effect the next time a session is started.\n"
		L"Are you sure that you want to relaunch Tera Term?",
		MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2
	};
	ret = TTMessageBoxA(hwnd, &info, ts->UILanguageFile);
	if (ret != IDYES)
		return;

	SendMessage(hwnd, WM_COMMAND, ID_SETUP_SAVE, 0);
	// ID_FILE_EXIT ���b�Z�[�W�ł̓A�v���������邱�Ƃ����邽�߁AWM_QUIT ���|�X�g����B
	//PostMessage(hwnd, WM_COMMAND, ID_FILE_EXIT, 0);
	PostQuitMessage(0);

	// ���v���Z�X�̍ċN���B
	if (hGetModuleFileNameW(NULL, &path) == 0) {
		TTWinExec(path);
		free(path);
	}
}
#endif
