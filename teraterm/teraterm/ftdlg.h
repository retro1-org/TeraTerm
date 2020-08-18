/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007-2020 TeraTerm Project
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

/* TERATERM.EXE, file transfer dialog box */

#include "tmfc.h"

/////////////////////////////////////////////////////////////////////////////
// CFileTransDlg dialog

typedef struct {
	const char *UILanguageFile;
	WORD OpId;
	wchar_t *DlgCaption;
	wchar_t *FullName;
	wchar_t *FileName;		// NULL�̂Ƃ��AFullName����t�@�C�������쐬����
	BOOL HideDialog;
	HWND HMainWin;
} CFileTransDlgInfo;

class CFileTransDlg : public TTCDialog
{
public:
	CFileTransDlg();
	~CFileTransDlg();

	BOOL Create(HINSTANCE hInstance, CFileTransDlgInfo *info);
	BOOL Create(HINSTANCE hInstance, HWND hParent, PFileVar pfv, PComVar pcv, PTTSet pts);
	void ChangeButton(BOOL PauseFlag);
	void RefreshNum(DWORD StartTime, LONG FileSize, LONG ByteCount);
	void RefreshNum(TFileVar *fv);

private:
	virtual BOOL OnCancel();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PostNcDestroy();
	virtual BOOL OnInitDialog();

private:
	BOOL Pause;
	HANDLE SmallIcon;
	HANDLE BigIcon;
	const char *UILanguageFile;
	WORD OpId;
	int ProgStat;	// �v���O���X�o�[�̐i�����߂�Ȃ��悤�L�����Ă���
	BOOL HideDialog;
	wchar_t *DlgCaption;
	wchar_t *FileName;
	wchar_t *FullName;
	HWND HMainWin;
};

typedef CFileTransDlg *PFileTransDlg;
