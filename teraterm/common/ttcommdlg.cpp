/*
 * Copyright (C) 2008- TeraTerm Project
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

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#include <shlobj.h>
#include <assert.h>

#include "win32helper.h"
#include "ttlib.h"

#include "ttcommdlg.h"

static int CALLBACK BrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch(uMsg) {
	case BFFM_INITIALIZED: {
		// ��������
		const wchar_t *folder = (wchar_t *)lpData;
		if (folder != NULL && folder[0] != 0) {
			// �t�H���_��I����Ԃɂ���
			//		�t�H���_�����݂��Ȃ��Ƃ��� 0 ���Ԃ��Ă���
			SendMessageW(hwnd, BFFM_SETSELECTIONW, (WPARAM)TRUE, (LPARAM)folder);
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

/**
 *	API��,�Â�
 */
static BOOL TTSHBrowseForFolderWAPI(TTBROWSEINFOW *bi, const wchar_t *def, wchar_t **folder)
{
	wchar_t buf[MAX_PATH];	// PIDL�`���Ŏ󂯎��̂Ń_�~�[,�ꉞMAX_PATH���Ŏ󂯂�
	BROWSEINFOW b = {};
	b.hwndOwner = bi->hwndOwner;
	b.pidlRoot = 0;	// 0 = from desktop
	b.pszDisplayName = buf;
	b.lpszTitle = bi->lpszTitle;
	b.ulFlags = bi->ulFlags;
	if (def != NULL && def[0] != 0) {
		if (GetFileAttributesW(def) == INVALID_FILE_ATTRIBUTES) {
			MessageBoxA(bi->hwndOwner, "Not found folder", "Tera Term", MB_OK | MB_ICONERROR);
		}
		else {
			b.lpfn = BrowseCallback;
			b.lParam = (LPARAM)def;
		}
	}
	LPITEMIDLIST pidl = SHBrowseForFolderW(&b);
	if (pidl == NULL) {
		*folder = NULL;
		return FALSE;
	}

	// PIDL�`���̖߂�l�̃t�@�C���V�X�e���̃p�X�ɕϊ�
#if _MSC_VER > 1400
	// VS2005�Ŏg����SDK�ɂ�SHGetPathFromIDListEx()�������Ă��Ȃ�
	if (true) {
		size_t len = MAX_PATH / 2;
		wchar_t *path = NULL;
		do {
			wchar_t *p;
			len *= 2;
			if (len >= SHRT_MAX) {
				free(path);
				return FALSE;
			}
			p = (wchar_t *)realloc(path, len);
			if (p == NULL) {
				free(path);
				return FALSE;
			}
			path = p;
		} while (!SHGetPathFromIDListEx(pidl, path, (DWORD)len, GPFIDL_DEFAULT));
		*folder = path;
	}
	else
#endif
	{
		wchar_t buf[MAX_PATH];
		if (!SHGetPathFromIDListW(pidl, buf)) {
			return FALSE;
		}
		*folder = _wcsdup(buf);
	}
	CoTaskMemFree(pidl);
	return TRUE;
}

/**
 *	SHBrowseForFolderW() �قڌ݊��֐�
 *
 *	@param	TTBROWSEINFOW
 *		- BROWSEINFOW �̑���� TTBROWSEINFOW ���g��
 *		- ���̃����o���Ȃ�
 *  	  - BROWSEINFOW.lpfn
 *		  - BROWSEINFOW.lParam
 *		  - BROWSEINFOW.iImage
 *		- folder �����ɑI���t�H���_������
 *	@param	def		�f�t�H���g�t�H���_
 *					�w�肵�Ȃ��Ƃ���NULL
 *	@param	folder	�w�肳�ꂽ�t�H���_
 *					�s�v�ɂȂ����� free() ���邱��
 *					MAX_PATH����Ȃ�
 */
BOOL TTSHBrowseForFolderW(TTBROWSEINFOW *bi, const wchar_t *def, wchar_t **folder)
{
#if _MSC_VER == 1400 // VS2005�̏ꍇ
	// 2005�Ŏg����SDK�ɂ�IFileOpenDialog���Ȃ�
	return TTSHBrowseForFolderWAPI(bi, def, folder);
#else
	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void **)&pDialog);
	if (FAILED(hr)) {
		// IFileOpenDialog ���g���Ȃ�OS, Vista�ȑO
		return TTSHBrowseForFolderWAPI(bi, def, folder);
	}

	DWORD options;
	pDialog->GetOptions(&options);
	pDialog->SetOptions(options | FOS_PICKFOLDERS);
	pDialog->SetTitle(bi->lpszTitle);
	{
		IShellItem *psi;
		hr = SHCreateItemFromParsingName(def, NULL, IID_IShellItem, (void **)&psi);
		if (SUCCEEDED(hr)) {
			hr = pDialog->SetFolder(psi);
			psi->Release();
		}
	}
	hr = pDialog->Show(bi->hwndOwner);

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
#endif
}

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
	TTBROWSEINFOW bi = {};
	bi.hwndOwner = hWnd;
	bi.lpszTitle = msg;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_NEWDIALOGSTYLE;

	return TTSHBrowseForFolderW(&bi, def, folder);
}

static BOOL GetOpenSaveFileNameW(const TTOPENFILENAMEW *ofn, bool save, wchar_t **filename)
{
	// GetSaveFileNameW(), GetOpenFileNameW() ���J�����g�t�H���_��ύX���Ă��܂�����
	wchar_t *curdir;
	hGetCurrentDirectoryW(&curdir);

	// �����t�H���_
	wchar_t *init_dir = NULL;
	if (ofn->lpstrFile != NULL) {
		// �����t�@�C���w�肠��
		if (!IsRelativePathW(ofn->lpstrFile)) {
			// �����t�@�C������΃p�X�Ȃ�p�X�����o���ď����t�H���_�ɂ���
			init_dir = _wcsdup(ofn->lpstrFile);
			wchar_t *p = wcsrchr(init_dir, L'\\');
			if (p != NULL) {
				*p = L'\0';
			}
		}
	}
	else {
		if (ofn->lpstrInitialDir != NULL) {
			// �����t�H���_�w�肠��
			init_dir = _wcsdup(ofn->lpstrInitialDir);
		}
	}

	wchar_t File[MAX_PATH];
	if (GetFileAttributesW(ofn->lpstrFile) != INVALID_FILE_ATTRIBUTES) {
		wcsncpy_s(File, _countof(File), ofn->lpstrFile, _TRUNCATE);
	}
	else {
		File[0] = 0;
	}

	OPENFILENAMEW o = {};
	o.lStructSize = get_OPENFILENAME_SIZEW();
	o.hwndOwner = ofn->hwndOwner;
	o.lpstrFilter = ofn->lpstrFilter;
	o.lpstrFile = File;
	o.nMaxFile = _countof(File);
	o.lpstrTitle = ofn->lpstrTitle;
	o.lpstrInitialDir = init_dir;
	o.Flags = ofn->Flags;
	BOOL ok = save ? GetSaveFileNameW(&o) : GetOpenFileNameW(&o);
#if defined(_DEBUG)
	if (!ok) {
		DWORD Err = GetLastError();
		DWORD DlgErr = CommDlgExtendedError();
		assert(Err == 0 && DlgErr == 0);
	}
#endif
	*filename = ok ? _wcsdup(File) : NULL;

	free(init_dir);
	SetCurrentDirectoryW(curdir);
	free(curdir);

	return ok;
}

/**
 *	GetOpenFileNameW() �݊��֐�
 *	�قȂ�_
 *		- �t�H���_���ύX����Ȃ�
 *		- �����t�H���_�ݒ�
 *			- �����t�@�C���̃t���p�X�������t�H���_�ɂ���
 *
 *	@param	filename	�I�����ꂽ�t�@�C����(�߂�l�� TRUE�̎�)
 *						MAX_PATH�����Ȃ��A�s�v�ɂȂ�����free()���邱��
 *	@retval	TRUE		ok�������ꂽ
 *	@retval	FALSE		cancel�������ꂽ
 */
BOOL TTGetOpenFileNameW(const TTOPENFILENAMEW *ofn, wchar_t **filename)
{
	return GetOpenSaveFileNameW(ofn, false, filename);
}

BOOL TTGetSaveFileNameW(const TTOPENFILENAMEW *ofn, wchar_t **filename)
{
	return GetOpenSaveFileNameW(ofn, true, filename);
}
