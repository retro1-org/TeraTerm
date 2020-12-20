/*
 * (C) 2020 TeraTerm Project
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

/* TERATERM.EXE, file transfer routines */
#include <stdio.h>
#include <windows.h>
#include <htmlhelp.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "protodlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttdde.h"
#include "ttlib.h"
#include "dlglib.h"
#include "vtterm.h"
#include "ftlib.h"
#include "buffer.h"
#include "helpid.h"
#include "layer_for_unicode.h"
#include "codeconv.h"

#include "filesys_log_res.h"

#include "filesys.h"
#include "filesys_proto.h"
#include "ttfile_proto.h"
#include "tt_res.h"
#include "kermit.h"
#include "xmodem.h"
#include "ymodem.h"
#include "zmodem.h"
#include "bplus.h"
#include "quickvan.h"

static PFileVarProto FileVar = NULL;
static int ProtoId;

extern BOOL FSend;

static PProtoDlg PtDlg = NULL;

static size_t _ReadFile(TFileVarProto *fv, void *buf, size_t bytes)
{
	HANDLE hFile = fv->FileHandle;
	DWORD NumberOfBytesRead;
	BOOL Result = ReadFile(hFile, buf, (UINT)bytes, &NumberOfBytesRead, NULL);
	if (Result == FALSE) {
		return 0;
	}
	return NumberOfBytesRead;
}

static size_t _WriteFile(TFileVarProto *fv, const void *buf, size_t bytes)
{
	HANDLE hFile = fv->FileHandle;
	DWORD NumberOfBytesWritten;
	UINT length = (UINT)bytes;
	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
	if (result == FALSE) {
		return 0;
	}
	return NumberOfBytesWritten;
}

static void _Close(TFileVarProto *fv)
{
	HANDLE hFile = fv->FileHandle;
	CloseHandle(hFile);
	fv->FileHandle = 0;
}

/**
 *	�t�@�C���̃t�@�C���T�C�Y���擾
 *	@param[in]	filenameU8		�t�@�C����(UTF-8)
 *	@retval		�t�@�C���T�C�Y
 */
static size_t _GetFSize(struct FileVarProto *fv, const char *filenameU8)
{
	size_t file_size = GetFSize64W(wc::fromUtf8(filenameU8));
	return file_size;
}

static void _SetDlgTime(TFileVarProto *fv, DWORD elapsed, int bytes)
{
	SetDlgTime(fv->HWin, IDC_PROTOELAPSEDTIME, fv->StartTime, fv->ByteCount);
}

static void _SetDlgPaketNum(struct FileVarProto *fv, LONG Num)
{
	SetDlgNum(fv->HWin, IDC_PROTOPKTNUM, Num);
}

static void _SetDlgByteCount(struct FileVarProto *fv, LONG Num)
{
	SetDlgNum(fv->HWin, IDC_PROTOBYTECOUNT, Num);
}

static void _SetDlgPercent(struct FileVarProto *fv, LONG a, LONG b, int *p)
{
	SetDlgPercent(fv->HWin, IDC_PROTOPERCENT, IDC_PROTOPROGRESS, a, b, p);
}

static void _SetDlgProtoText(struct FileVarProto *fv, const char *text)
{
	SetDlgItemText(fv->HWin, IDC_PROTOPROT, text);
}

static void _SetDlgProtoFileName(struct FileVarProto *fv, const char *text)
{
	SetDlgItemText(fv->HWin, IDC_PROTOFNAME, text);
}

static void _InitDlgProgress(struct FileVarProto *fv, int *CurProgStat)
{
	InitDlgProgress(fv->HWin, IDC_PROTOPROGRESS, CurProgStat);
}

static BOOL NewFileVar_(PFileVarProto *pfv)
{
	TFileVarProto *fv = (TFileVarProto *)malloc(sizeof(TFileVarProto));
	if (fv == NULL)
		return FALSE;
	memset(fv, 0, sizeof(*fv));

	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
	strncpy_s(fv->FullName, sizeof(fv->FullName), FileDirExpanded, _TRUNCATE);
	AppendSlash(fv->FullName,sizeof(fv->FullName));

	fv->DirLen = strlen(fv->FullName);
	fv->FileOpen = FALSE;
	fv->OverWrite = ((ts.FTFlag & FT_RENAME) == 0);
	fv->HMainWin = HVTWin;
	fv->Success = FALSE;
	fv->NoMsg = FALSE;
	fv->HideDialog = FALSE;

	fv->ReadFile = _ReadFile;
	fv->WriteFile = _WriteFile;
	fv->Close = _Close;
	fv->GetFSize = _GetFSize;

	fv->InitDlgProgress = _InitDlgProgress;
	fv->SetDlgTime = _SetDlgTime;
	fv->SetDlgPaketNum = _SetDlgPaketNum;
	fv->SetDlgByteCount = _SetDlgByteCount;
	fv->SetDlgPercent = _SetDlgPercent;
	fv->SetDlgProtoText = _SetDlgProtoText;
	fv->SetDlgProtoFileName = _SetDlgProtoFileName;

	*pfv = fv;
	return TRUE;
}

static void FreeFileVar_(PFileVarProto *pfv)
{
	PFileVarProto fv = *pfv;
	if (fv == NULL) {
		return;
	}

	if (fv->Destroy != NULL) {
		fv->Destroy(fv);
	}

	if (fv->FileOpen) CloseHandle(fv->FileHandle);
	if (fv->FnStrMemHandle != 0)
	{
		GlobalUnlock(fv->FnStrMemHandle);
		GlobalFree(fv->FnStrMemHandle);
	}
	free(fv);

	*pfv = NULL;
}

static BOOL OpenProtoDlg(PFileVarProto fv, int IdProto, int Mode, WORD Opt1, WORD Opt2)
{
	PProtoDlg pd;

	ProtoId = IdProto;

	switch (ProtoId) {
		case PROTO_KMT:
			KmtCreate(fv);
			break;
		case PROTO_XM:
			XCreate(fv);
			break;
		case PROTO_YM:
			YCreate(fv);
			break;
		case PROTO_ZM:
			ZCreate(fv);
			break;
		case PROTO_BP:
			BPCreate(fv);
			break;
		case PROTO_QV:
			QVCreate(fv);
			break;
		default:
			assert(FALSE);
			return FALSE;
			break;
	}

	switch (ProtoId) {
		case PROTO_KMT:
			_ProtoSetOpt(fv, KMT_MODE, Mode);
			break;
		case PROTO_XM:
			_ProtoSetOpt(fv, XMODEM_MODE, Mode);
			_ProtoSetOpt(fv, XMODEM_OPT, Opt1);
			_ProtoSetOpt(fv, XMODEM_TEXT_FLAG, 1 - (Opt2 & 1));
			break;
		case PROTO_YM:
			_ProtoSetOpt(fv, YMODEM_MODE, Mode);
			_ProtoSetOpt(fv, YMODEM_OPT, Opt1);
			break;
		case PROTO_ZM:
			_ProtoSetOpt(fv, ZMODEM_MODE, Mode);
			_ProtoSetOpt(fv, ZMODEM_BINFLAG, (Opt1 & 1) != 0);
			break;
		case PROTO_BP:
			_ProtoSetOpt(fv, BPLUS_MODE, Mode);
			break;
		case PROTO_QV:
			_ProtoSetOpt(fv, QUICKVAN_MODE, Mode);
			break;
	}

	pd = new CProtoDlg();
	if (pd==NULL)
	{
		return FALSE;
	}
	CProtoDlgInfo info;
	info.UILanguageFile = ts.UILanguageFile;
	info.HMainWin = fv->HMainWin;
	pd->Create(hInst, HVTWin, &info);
	fv->HWin = pd->m_hWnd;

	BOOL r = fv->Init(fv, &cv, &ts);
	if (r == FALSE) {
		fv->Destroy(fv);
		return FALSE;
	}
	SetWindowText(fv->HWin, fv->DlgCaption);

	PtDlg = pd;
	return TRUE;
}

static void CloseProtoDlg(void)
{
	if (PtDlg!=NULL)
	{
		PtDlg->DestroyWindow();
		PtDlg = NULL;

		::KillTimer(FileVar->HMainWin,IdProtoTimer);
		{	// Quick-VAN special code
			//if ((ProtoId==PROTO_QV) &&
			//    (((PQVVar)ProtoVar)->QVMode==IdQVSend))
			if (FileVar->OpId == OpQVSend)
				CommTextOut(&cv,"\015",1);
		}
	}
}

static BOOL ProtoStart(void)
{
	if (cv.ProtoFlag)
		return FALSE;
	if (FSend)
	{
		FreeFileVar_(&FileVar);
		return FALSE;
	}

	NewFileVar_(&FileVar);

	if (FileVar==NULL)
	{
		return FALSE;
	}
	cv.ProtoFlag = TRUE;
	return TRUE;
}

void ProtoEnd(void)
{
	if (! cv.ProtoFlag)
		return;
	cv.ProtoFlag = FALSE;

	/* Enable transmit delay (serial port) */
	cv.DelayFlag = TRUE;
	TalkStatus = IdTalkKeyb;

	CloseProtoDlg();

	if ((FileVar!=NULL) && FileVar->Success)
		EndDdeCmnd(1);
	else
		EndDdeCmnd(0);

	FreeFileVar_(&FileVar);
}

/* �_�C�A���O�𒆉��Ɉړ����� */
static void CenterCommonDialog(HWND hDlg)
{
	/* hDlg�̐e���_�C�A���O�̃E�B���h�E�n���h�� */
	HWND hWndDlgRoot = GetParent(hDlg);
	CenterWindow(hWndDlgRoot, GetParent(hWndDlgRoot));
}

static HFONT DlgXoptFont;

/* Hook function for XMODEM file name dialog box */
static UINT_PTR CALLBACK XFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ IDC_XOPT, "DLG_XOPT" },
		{ IDC_XOPTCHECK, "DLG_XOPT_CHECKSUM" },
		{ IDC_XOPTCRC, "DLG_XOPT_CRC" },
		{ IDC_XOPT1K, "DLG_XOPT_1K" },
		{ IDC_XOPTBIN, "DLG_XOPT_BINARY" },
	};
	LPOPENFILENAME ofn;
	WORD Hi, Lo;
	LPLONG pl;
	LPOFNOTIFY notify;
	LOGFONT logfont;
	HFONT font;
	const char *UILanguageFile = ts.UILanguageFile;

	switch (Message) {
	case WM_INITDIALOG:
		ofn = (LPOPENFILENAME)lParam;
		pl = (LPLONG)ofn->lCustData;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)pl);

		font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (get_lang_font("DLG_TAHOMA_FONT", Dialog, &logfont, &DlgXoptFont, UILanguageFile)) {
			SendDlgItemMessage(Dialog, IDC_XOPT, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTCHECK, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTCRC, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPT1K, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_XOPTBIN, WM_SETFONT, (WPARAM)DlgXoptFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgXoptFont = NULL;
		}

		SetI18nDlgStrs("Tera Term", Dialog, text_info, _countof(text_info), UILanguageFile);

		if (LOWORD(*pl)==0xFFFF) { // Send
			ShowDlgItem(Dialog, IDC_XOPT1K, IDC_XOPT1K);
			Hi = 0;
			if (HIWORD(*pl) == Xopt1kCRC || HIWORD(*pl) == Xopt1kCksum) {
				Hi = 1;
			}
			SetRB(Dialog, Hi, IDC_XOPT1K, IDC_XOPT1K);
		}
		else { // Recv
			ShowDlgItem(Dialog, IDC_XOPTCHECK, IDC_XOPTCRC);
			Hi = HIWORD(*pl);
			if (Hi == Xopt1kCRC) {
				Hi = XoptCRC;
			}
			else if (Hi == Xopt1kCksum) {
				Hi = XoptCheck;
			}
			SetRB(Dialog, Hi, IDC_XOPTCHECK, IDC_XOPTCRC);

			ShowDlgItem(Dialog,IDC_XOPTBIN,IDC_XOPTBIN);
			SetRB(Dialog,LOWORD(*pl),IDC_XOPTBIN,IDC_XOPTBIN);
		}
		CenterCommonDialog(Dialog);
		return TRUE;
	case WM_COMMAND: // for old style dialog
		switch (LOWORD(wParam)) {
		case IDOK:
			pl = (LPLONG)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pl!=NULL)
			{
				if (LOWORD(*pl)==0xFFFF) { // Send
					Lo = 0xFFFF;

					GetRB(Dialog, &Hi, IDC_XOPT1K, IDC_XOPT1K);
					if (Hi > 0) { // force CRC if 1K
						Hi = Xopt1kCRC;
					}
					else {
						Hi = XoptCRC;
					}
				}
				else { // Recv
					GetRB(Dialog, &Lo, IDC_XOPTBIN, IDC_XOPTBIN);
					GetRB(Dialog, &Hi, IDC_XOPTCHECK, IDC_XOPTCRC);
				}
				*pl = MAKELONG(Lo,Hi);
			}
			if (DlgXoptFont != NULL) {
				DeleteObject(DlgXoptFont);
			}
			break;
		case IDCANCEL:
			if (DlgXoptFont != NULL) {
				DeleteObject(DlgXoptFont);
			}
			break;
		}
		break;
	case WM_NOTIFY:	// for Explorer-style dialog
		notify = (LPOFNOTIFY)lParam;
		switch (notify->hdr.code) {
		case CDN_FILEOK:
			pl = (LPLONG)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pl!=NULL) {
				if (LOWORD(*pl) == 0xFFFF) { // Send
					Lo = 0xFFFF;

					GetRB(Dialog, &Hi, IDC_XOPT1K, IDC_XOPT1K);
					if (Hi > 0) { // force CRC if 1K
						Hi = Xopt1kCRC;
					}
					else {
						Hi = XoptCRC;
					}
				}
				else { // Recv
					GetRB(Dialog, &Lo, IDC_XOPTBIN, IDC_XOPTBIN);
					GetRB(Dialog, &Hi, IDC_XOPTCHECK, IDC_XOPTCRC);
				}
				*pl = MAKELONG(Lo, Hi);
			}
			if (DlgXoptFont != NULL) {
				DeleteObject(DlgXoptFont);
			}
			break;
		}
		break;
	}
	return FALSE;
}

/**
 *	�_�C�A���O�̃f�t�H���g�t�@�C������Ԃ�
 *		�t�B���^(ts.FileSendFilter)�����C���h�J�[�h�ł͂Ȃ��A
 *		���̃t�@�C�������݂���ꍇ
 *		�f�t�H���g�̃t�@�C�����Ƃ��ĕԂ�
 *
 * @param[in]	path		�t�@�C�������݂��邩���ׂ�p�X
 *							(lpstrInitialDir �ɐݒ肳���p�X)
 * @retval		NULL		�f�t�H���g�t�@�C�����Ȃ�
 * @retval		NULL�ȊO	�f�t�H���g�t�@�C��(�s�v�ɂȂ�����free()���邱��)
 */
static wchar_t *GetCommonDialogDefaultFilenameW(const wchar_t *path)
{
	const char *FileSendFilterA = ts.FileSendFilter;
	if (strlen(FileSendFilterA) == 0) {
		return NULL;
	}

	// �t�B���^�����C���h�J�[�h�ł͂Ȃ��A���̃t�@�C�������݂���ꍇ
	// ���炩���߃f�t�H���g�̃t�@�C���������Ă��� (2008.5.18 maya)
	wchar_t *filename = NULL;
	if (!isInvalidFileNameChar(FileSendFilterA)) {
		wchar_t file[MAX_PATH];
		wcsncpy_s(file, _countof(file), path, _TRUNCATE);
		AppendSlashW(file, _countof(file));
		wchar_t *FileSendFilterW = ToWcharA(FileSendFilterA);
		wcsncat_s(file, _countof(file), FileSendFilterW, _TRUNCATE);
		DWORD attr = _GetFileAttributesW(file);
		if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			// �t�@�C�������݂���
			filename = _wcsdup(file);
		}
		free(FileSendFilterW);
	}

	return filename;
}

static char *GetCommonDialogDefaultFilenameA(const char *path)
{
	wchar_t *pathW = ToWcharA(path);
	wchar_t *fileW = GetCommonDialogDefaultFilenameW(pathW);
	char *fileA = ToCharW(fileW);
	free(pathW);
	free(fileW);
	return fileA;
}

static BOOL _GetXFname(HWND HWin, BOOL Receive, const char *caption, LPLONG Option, PFileVarProto fv)
{
	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
	PCHAR CurDir = FileDirExpanded;

	char *FNFilter = GetCommonDialogFilterA(!Receive ? ts.FileSendFilter : NULL, ts.UILanguageFile);

	fv->FullName[0] = 0;
	if (!Receive) {
		char *default_filename = GetCommonDialogDefaultFilenameA(CurDir);
		if (default_filename != NULL) {
			strncpy_s(fv->FullName, _countof(fv->FullName), default_filename, _TRUNCATE);
			free(default_filename);
		}
	}

	OPENFILENAME ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = HWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fv->FullName;
	ofn.nMaxFile = _countof(fv->FullName);
	ofn.lpstrInitialDir = CurDir;
	LONG opt = *Option;
	if (! Receive)
	{
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		opt = opt | 0xFFFF;
	}
	else {
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	}
	ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
	ofn.Flags |= OFN_SHOWHELP;
	ofn.lCustData = (LPARAM)&opt;
	ofn.lpstrTitle = caption;
	ofn.lpfnHook = XFnHook;
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_XOPT);
	ofn.hInstance = hInst;

	/* save current dir */
	wchar_t TempDir[MAX_PATH];
	_GetCurrentDirectoryW(_countof(TempDir), TempDir);
	BOOL Ok;
	if (!Receive)
	{
		Ok = GetOpenFileName(&ofn);
	}
	else {
		Ok = GetSaveFileName(&ofn);
	}
	free(FNFilter);
	_SetCurrentDirectoryW(TempDir);

	if (Ok) {
		fv->DirLen = ofn.nFileOffset;
		fv->FnPtr = ofn.nFileOffset;

		if (Receive)
			*Option = opt;
		else
			*Option = MAKELONG(LOWORD(*Option),HIWORD(opt));
	}

	return Ok;
}

/**
 *	OnIdle()#teraterm.cpp����R�[�������
 *		cv.ProtoFlag �� 0 �ȊO�̂Ƃ�
 *	@retval		0		continue
 *				1/2		ActiveWin(�O���[�o���ϐ�)�̒l(IdVT=1/IdTek=2)
 *				�� ���̂Ƃ���̂Ă��Ă���
 */
int ProtoDlgParse(void)
{
	int P;

	P = ActiveWin;
	if (PtDlg==NULL)
		return P;

	if (_ProtoParse(ProtoId,FileVar,&cv))
		P = 0; /* continue */
	else {
		CommSend(&cv);
		ProtoEnd();
	}
	return P;
}

void ProtoDlgTimeOut(void)
{
	if (PtDlg!=NULL)
		_ProtoTimeOutProc(ProtoId,FileVar,&cv);
}

void ProtoDlgCancel(void)
{
	if ((PtDlg!=NULL) &&
	    _ProtoCancel(ProtoId,FileVar,&cv))
		ProtoEnd();
}

static INT_PTR CALLBACK GetFnDlg(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ 0, "DLG_GETFN_TITLE" },
		{ IDC_FILENAME, "DLG_GETFN_FILENAME" },
		{ IDOK, "BTN_OK" },
		{ IDCANCEL, "BTN_CANCEL" },
		{ IDC_GETFNHELP, "BTN_HELP" },
	};
	PFileVarProto fv;
	char TempFull[MAX_PATH];
	int i, j;
	const char *UILanguageFile = ts.UILanguageFile;

	switch (Message) {
	case WM_INITDIALOG:
		fv = (PFileVarProto)lParam;
		SetWindowLongPtr(Dialog, DWLP_USER, lParam);
		SendDlgItemMessage(Dialog, IDC_GETFN, EM_LIMITTEXT, sizeof(TempFull)-1,0);
		SetI18nDlgStrs("Tera Term", Dialog, text_info, _countof(text_info), UILanguageFile);
		return TRUE;

	case WM_COMMAND:
		fv = (PFileVarProto)GetWindowLongPtr(Dialog,DWLP_USER);
		switch (LOWORD(wParam)) {
		case IDOK:
			if (fv!=NULL) {
				GetDlgItemText(Dialog, IDC_GETFN, TempFull, sizeof(TempFull));
				if (strlen(TempFull)==0) return TRUE;
				GetFileNamePos(TempFull,&i,&j);
				FitFileName(&(TempFull[j]),sizeof(TempFull) - j, NULL);
				strncat_s(fv->FullName,sizeof(fv->FullName),&(TempFull[j]),_TRUNCATE);
			}
			EndDialog(Dialog, 1);
			return TRUE;
		case IDCANCEL:
			EndDialog(Dialog, 0);
			return TRUE;
		case IDC_GETFNHELP:
			if (fv!=NULL) {
				// �Ăяo�������w���vID����������
				PostMessage(fv->HMainWin,WM_USER_DLGHELP2,0,0);
			}
			break;
		}
	}
	return FALSE;
}

static BOOL _GetGetFname(HWND HWin, PFileVarProto fv, PTTSet ts)
{
	SetDialogFont(ts->DialogFontName, ts->DialogFontPoint, ts->DialogFontCharSet,
				  ts->UILanguageFile, "Tera Term", "DLG_SYSTEM_FONT");
	return (BOOL)TTDialogBoxParam(hInst,
								  MAKEINTRESOURCE(IDD_GETFNDLG),
								  HWin, GetFnDlg, (LPARAM)fv);
}

static HFONT DlgFoptFont;

static UINT_PTR CALLBACK TransFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo text_info[] = {
		{ IDC_FOPT, "DLG_FOPT" },
		{ IDC_FOPTBIN, "DLG_FOPT_BINARY" },
	};
	LPOPENFILENAME ofn;
	LPWORD pw;
	LPOFNOTIFY notify;
	LOGFONT logfont;
	HFONT font;
	const char *UILanguageFile = ts.UILanguageFile;

	switch (Message) {
	case WM_INITDIALOG:
		ofn = (LPOPENFILENAME)lParam;
		pw = (LPWORD)ofn->lCustData;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)pw);

		font = (HFONT)SendMessage(Dialog, WM_GETFONT, 0, 0);
		GetObject(font, sizeof(LOGFONT), &logfont);
		if (get_lang_font("DLG_TAHOMA_FONT", Dialog, &logfont, &DlgFoptFont, UILanguageFile)) {
			SendDlgItemMessage(Dialog, IDC_FOPT, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_FOPTBIN, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_FOPTAPPEND, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_PLAINTEXT, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
			SendDlgItemMessage(Dialog, IDC_TIMESTAMP, WM_SETFONT, (WPARAM)DlgFoptFont, MAKELPARAM(TRUE,0));
		}
		else {
			DlgFoptFont = NULL;
		}

		SetI18nDlgStrs("Tera Term", Dialog, text_info, _countof(text_info), UILanguageFile);

		SetRB(Dialog,*pw & 1,IDC_FOPTBIN,IDC_FOPTBIN);

		CenterCommonDialog(Dialog);

		return TRUE;
	case WM_COMMAND: // for old style dialog
		switch (LOWORD(wParam)) {
		case IDOK:
			pw = (LPWORD)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pw!=NULL)
				GetRB(Dialog,pw,IDC_FOPTBIN,IDC_FOPTBIN);
			if (DlgFoptFont != NULL) {
				DeleteObject(DlgFoptFont);
			}
			break;
		case IDCANCEL:
			if (DlgFoptFont != NULL) {
				DeleteObject(DlgFoptFont);
			}
			break;
		}
		break;
	case WM_NOTIFY: // for Explorer-style dialog
		notify = (LPOFNOTIFY)lParam;
		switch (notify->hdr.code) {
		case CDN_FILEOK:
			pw = (LPWORD)GetWindowLongPtr(Dialog,DWLP_USER);
			if (pw!=NULL)
				GetRB(Dialog,pw,IDC_FOPTBIN,IDC_FOPTBIN);
			if (DlgFoptFont != NULL) {
				DeleteObject(DlgFoptFont);
			}
			break;
		}
		break;
	}
	return FALSE;
}

/* GetMultiFname function id */
#define GMF_KERMIT 0 /* Kermit Send */
#define GMF_Z  1     /* ZMODEM Send */
#define GMF_QV 2     /* Quick-VAN Send */
#define GMF_Y  3     /* YMODEM Send */

#define FnStrMemSize 4096

static BOOL _GetMultiFname(PFileVarProto fv, WORD FuncId, LPWORD Option)
{
	OPENFILENAME ofn;
	wchar_t TempDir[MAX_PATH];
	BOOL Ok;
	const char *FileSendFilter = ts.FileSendFilter;
	const char *UILanguageFile = ts.UILanguageFile;

	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
	PCHAR CurDir = FileDirExpanded;

	/* save current dir */
	_GetCurrentDirectoryW(_countof(TempDir), TempDir);

	fv->NumFname = 0;

	/* moemory should be zero-initialized */
	fv->FnStrMemHandle = GlobalAlloc(GHND, FnStrMemSize);
	if (fv->FnStrMemHandle == NULL) {
		MessageBeep(0);
		return FALSE;
	}
	else {
		fv->FnStrMem = (char *)GlobalLock(fv->FnStrMemHandle);
		if (fv->FnStrMem == NULL) {
			GlobalFree(fv->FnStrMemHandle);
			fv->FnStrMemHandle = 0;
			MessageBeep(0);
			return FALSE;
		}
	}

	char *FNFilter = GetCommonDialogFilterA(FileSendFilter, UILanguageFile);

	char *default_filename = GetCommonDialogDefaultFilenameA(CurDir);
	if (default_filename != NULL) {
		strncpy_s(fv->FnStrMem, _countof(fv->FullName), default_filename, _TRUNCATE);
		free(default_filename);
	}

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = fv->HMainWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fv->FnStrMem;
	ofn.nMaxFile = FnStrMemSize;
	ofn.lpstrTitle= fv->DlgCaption;
	ofn.lpstrInitialDir = CurDir;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	ofn.Flags |= OFN_SHOWHELP;
	ofn.lCustData = 0;
	if (FuncId==GMF_Z) {
		ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
		ofn.lCustData = (LPARAM)Option;
		ofn.lpfnHook = TransFnHook;
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FOPT);
	} else if (FuncId==GMF_Y) {
		// TODO: YMODEM

	}

	ofn.hInstance = hInst;

	Ok = GetOpenFileName(&ofn);
	free(FNFilter);

	if (Ok) {
		int i, len;
		/* count number of file names */
		len = strlen(fv->FnStrMem);
		i = 0;
		while (len>0) {
			i = i + len + 1;
			fv->NumFname++;
			len = strlen(&fv->FnStrMem[i]);
		}

		fv->NumFname--;

		if (fv->NumFname<1) { // single selection
			fv->NumFname = 1;
			fv->DirLen = ofn.nFileOffset;
			strncpy_s(fv->FullName, sizeof(fv->FullName),fv->FnStrMem, _TRUNCATE);
			fv->FnPtr = 0;
		}
		else { // multiple selection
			strncpy_s(fv->FullName, sizeof(fv->FullName),fv->FnStrMem, _TRUNCATE);
			AppendSlash(fv->FullName,sizeof(fv->FullName));
			fv->DirLen = strlen(fv->FullName);
			fv->FnPtr = strlen(fv->FnStrMem)+1;
		}

		fv->FNCount = 0;
	}

	GlobalUnlock(fv->FnStrMemHandle);
	if (! Ok) {
		GlobalFree(fv->FnStrMemHandle);
		fv->FnStrMemHandle = NULL;
	}

	/* restore dir */
	_SetCurrentDirectoryW(TempDir);

	return Ok;
}

static void _SetFileVar(PFileVarProto fv)
{
	int i;
	char c;

	GetFileNamePos(fv->FullName,&(fv->DirLen),&i);
	c = fv->FullName[fv->DirLen];
	if (c=='\\'||c=='/') fv->DirLen++;
}

void KermitStart(int mode)
{
	WORD w;
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	switch (mode) {
		case IdKmtSend:
			FileVar->OpId = OpKmtSend;

			strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
			get_lang_msg("FILEDLG_TRANS_TITLE_KMTSEND", uimsg, sizeof(uimsg), TitKmtSend, UILanguageFile);
			strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				if (!_GetMultiFname(FileVar, GMF_KERMIT, &w) ||
				    (FileVar->NumFname==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				_SetFileVar(FileVar);
			break;
		case IdKmtReceive:
			FileVar->OpId = OpKmtRcv;

			strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
			get_lang_msg("FILEDLG_TRANS_TITLE_KMTRCV", uimsg, sizeof(uimsg), TitKmtRcv, UILanguageFile);
			strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

			break;
		case IdKmtGet:
			FileVar->OpId = OpKmtSend;

			strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
			get_lang_msg("FILEDLG_TRANS_TITLE_KMTGET", uimsg, sizeof(uimsg), TitKmtGet, UILanguageFile);
			strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				if (! _GetGetFname(FileVar->HMainWin,FileVar, &ts) ||
				    (strlen(FileVar->FullName)==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				_SetFileVar(FileVar);
			break;
		case IdKmtFinish:
			FileVar->OpId = OpKmtFin;

			strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
			get_lang_msg("FILEDLG_TRANS_TITLE_KMTFIN", uimsg, sizeof(uimsg), TitKmtFin, UILanguageFile);
			strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

			break;
		default:
			ProtoEnd();
			return;
	}
	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_KMT,mode,0,0))
		ProtoEnd();
}

BOOL KermitStartSend(const char *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtSend);

	return TRUE;
}

BOOL KermitGet(const char *filename)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtGet);

	return TRUE;
}

BOOL KermitStartRecive(void)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtReceive);

	return TRUE;
}

BOOL KermitFinish(void)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->NoMsg = TRUE;
	KermitStart(IdKmtFinish);

	return TRUE;
}

void XMODEMStart(int mode)
{
	LONG Option;
	int tmp;
	const char *UILanguageFile = ts.UILanguageFile;
	char uimsg[MAX_UIMSG];

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode==IdXReceive) {
		FileVar->OpId = OpXRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_XRCV", uimsg, sizeof(uimsg), TitXRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}
	else {
		FileVar->OpId = OpXSend;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_XSEND", uimsg, sizeof(uimsg), TitXSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

	if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
	{
		Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		if (! _GetXFname(FileVar->HMainWin,
						 mode==IdXReceive, fv->DlgCaption, &Option,FileVar))
		{
			ProtoEnd();
			return;
		}
		tmp = HIWORD(Option);
		if (mode == IdXReceive) {
			if (IsXoptCRC(tmp)) {
				if (IsXopt1k(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCRC;
				}
				else {
					ts.XmodemOpt = XoptCRC;
				}
			}
			else {
				if (IsXopt1k(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCksum;
				}
				else {
					ts.XmodemOpt = XoptCheck;
				}
			}
			ts.XmodemBin = LOWORD(Option);
		}
		else {
			if (IsXopt1k(tmp)) {
				if (IsXoptCRC(ts.XmodemOpt)) {
					ts.XmodemOpt = Xopt1kCRC;
				}
				else {
					ts.XmodemOpt = Xopt1kCksum;
				}
			}
			else {
				if (IsXoptCRC(ts.XmodemOpt)) {
					ts.XmodemOpt = XoptCRC;
				}
				else {
					ts.XmodemOpt = XoptCheck;
				}
			}
		}
	}
	else
		_SetFileVar(FileVar);

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_XM,mode, ts.XmodemOpt,ts.XmodemBin)) {
		ProtoEnd();
	}
}

BOOL XMODEMStartReceive(const char *fiename, WORD ParamBinaryFlag, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),fiename, _TRUNCATE);
	if (IsXopt1k(ts.XmodemOpt)) {
		if (IsXoptCRC(ParamXmodemOpt)) {
			// CRC
			ts.XmodemOpt = Xopt1kCRC;
		}
		else {	// Checksum
			ts.XmodemOpt = Xopt1kCksum;
		}
	}
	else {
		if (IsXoptCRC(ParamXmodemOpt)) {
			ts.XmodemOpt = XoptCRC;
		}
		else {
			ts.XmodemOpt = XoptCheck;
		}
	}
	ts.XmodemBin = ParamBinaryFlag;
	FileVar->NoMsg = TRUE;
	XMODEMStart(IdXReceive);

	return TRUE;
}

BOOL XMODEMStartSend(const char *fiename, WORD ParamXmodemOpt)
{
	if (FileVar !=NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName), fiename, _TRUNCATE);
	if (IsXoptCRC(ts.XmodemOpt)) {
		if (IsXopt1k(ParamXmodemOpt)) {
			ts.XmodemOpt = Xopt1kCRC;
		}
		else {
			ts.XmodemOpt = XoptCRC;
		}
	}
	else {
		if (IsXopt1k(ParamXmodemOpt)) {
			ts.XmodemOpt = Xopt1kCksum;
		}
		else {
			ts.XmodemOpt = XoptCheck;
		}
	}
	FileVar->NoMsg = TRUE;
	XMODEMStart(IdXSend);

	return TRUE;
}

void YMODEMStart(int mode)
{
	WORD Opt;
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode==IdYSend)
	{
		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_YSEND", uimsg, sizeof(uimsg), TitYSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		// �t�@�C���]�����̃I�v�V������"Yopt1K"�Ɍ��ߑł��B
		// TODO: "Yopt1K", "YoptG", "YoptSingle"����ʂ������Ȃ�΁AIDD_FOPT���g������K�v����B
		Opt = Yopt1K;
		FileVar->OpId = OpYSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! _GetMultiFname(FileVar, GMF_Y,&Opt) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
			//ts.XmodemBin = Opt;
		}
		else
		_SetFileVar(FileVar);
	}
	else {
		FileVar->OpId = OpYRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_YRCV", uimsg, sizeof(uimsg), TitYRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		// �t�@�C���]�����̃I�v�V������"Yopt1K"�Ɍ��ߑł��B
		Opt = Yopt1K;
		_SetFileVar(FileVar);
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_YM,mode,Opt,0))
		ProtoEnd();
}

BOOL YMODEMStartReceive()
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}
	FileVar->NoMsg = TRUE;
	YMODEMStart(IdYReceive);
	return TRUE;
}

BOOL YMODEMStartSend(const char *fiename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),fiename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	YMODEMStart(IdYSend);
	return TRUE;
}

void ZMODEMStart(int mode)
{
	WORD Opt = 0; // TODO �g���Ă��Ȃ�
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode == IdZSend || mode == IdZAutoS)
	{
		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_ZSEND", uimsg, sizeof(uimsg), TitZSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		Opt = ts.XmodemBin;
		FileVar->OpId = OpZSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! _GetMultiFname(FileVar, GMF_Z,&Opt) ||
			    (FileVar->NumFname==0))
			{
				if (mode == IdZAutoS) {
					CommRawOut(&cv, "\030\030\030\030\030\030\030\030\b\b\b\b\b\b\b\b\b\b", 18);
				}
				ProtoEnd();
				return;
			}
			ts.XmodemBin = Opt;
		}
		else
		_SetFileVar(FileVar);
	}
	else {
		/* IdZReceive or IdZAutoR */
		FileVar->OpId = OpZRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_ZRCV", uimsg, sizeof(uimsg), TitZRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_ZM,mode,Opt,0))
		ProtoEnd();
}

BOOL ZMODEMStartReceive(void)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	FileVar->NoMsg = TRUE;
	ZMODEMStart(IdZReceive);

	return TRUE;
}

BOOL ZMODEMStartSend(const char *fiename, WORD ParamBinaryFlag)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),fiename, _TRUNCATE);
	FileVar->NumFname = 1;
	ts.XmodemBin = ParamBinaryFlag;
	FileVar->NoMsg = TRUE;

	ZMODEMStart(IdZSend);

	return TRUE;
}

static BOOL _GetTransFname(PFileVarProto fv, const char *DlgCaption)
{
	wchar_t TempDir[MAX_PATH];
	char FileName[MAX_PATH];
	const char *UILanguageFile = ts.UILanguageFile;

	char FileDirExpanded[MAX_PATH];
	ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
	PCHAR CurDir = FileDirExpanded;

	/* save current dir */
	_GetCurrentDirectoryW(_countof(TempDir), TempDir);

	char *FNFilter = GetCommonDialogFilterA(ts.FileSendFilter, UILanguageFile);

	ExtractFileName(fv->FullName, FileName ,sizeof(FileName));
	strncpy_s(fv->FullName, sizeof(fv->FullName), FileName, _TRUNCATE);

	OPENFILENAME ofn = {};
	ofn.lStructSize = get_OPENFILENAME_SIZE();
	ofn.hwndOwner   = fv->HMainWin;
	ofn.lpstrFilter = FNFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = fv->FullName;
	ofn.nMaxFile = sizeof(fv->FullName);
	ofn.lpstrInitialDir = CurDir;

	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	ofn.Flags |= OFN_SHOWHELP;

	ofn.lpstrTitle = DlgCaption;

	ofn.hInstance = hInst;

	BOOL Ok = GetOpenFileName(&ofn);
	free(FNFilter);

	if (Ok) {
		fv->DirLen = ofn.nFileOffset;
	}
	/* restore dir */
	_SetCurrentDirectoryW(TempDir);
	return Ok;
}

void BPStart(int mode)
{
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode==IdBPSend)
	{
		FileVar->OpId = OpBPSend;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_BPSEND", uimsg, sizeof(uimsg), TitBPSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			FileVar->FullName[0] = 0;
			if (! _GetTransFname(FileVar, FileVar->DlgCaption))
			{
				ProtoEnd();
				return;
			}
		}
		else
			_SetFileVar(FileVar);
	}
	else {
		/* IdBPReceive or IdBPAuto */
		FileVar->OpId = OpBPRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_BPRCV", uimsg, sizeof(uimsg), TitBPRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_BP,mode,0,0))
		ProtoEnd();
}

BOOL BPSendStart(const char *filename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName), filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	BPStart(IdBPSend);

	return TRUE;
}

BOOL BPStartReceive(void)
{
	if (FileVar != NULL)
		return FALSE;
	if (!NewFileVar_(&FileVar))
		return FALSE;

	FileVar->NoMsg = TRUE;
	BPStart(IdBPReceive);

	return TRUE;
}

void QVStart(int mode)
{
	WORD W;
	char uimsg[MAX_UIMSG];
	const char *UILanguageFile = ts.UILanguageFile;

	if (! ProtoStart())
		return;

	TFileVarProto *fv = FileVar;

	if (mode==IdQVSend)
	{
		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_QVSEND", uimsg, sizeof(uimsg), TitQVSend, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);

		FileVar->OpId = OpQVSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! _GetMultiFname(FileVar, GMF_QV, &W) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
		}
		else
			_SetFileVar(FileVar);
	}
	else {
		FileVar->OpId = OpQVRcv;

		strncpy_s(fv->DlgCaption, sizeof(fv->DlgCaption),"Tera Term: ", _TRUNCATE);
		get_lang_msg("FILEDLG_TRANS_TITLE_QVRCV", uimsg, sizeof(uimsg), TitQVRcv, UILanguageFile);
		strncat_s(fv->DlgCaption, sizeof(fv->DlgCaption), uimsg, _TRUNCATE);
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_QV,mode,0,0))
		ProtoEnd();
}

BOOL QVStartReceive(void)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	FileVar->NoMsg = TRUE;
	QVStart(IdQVReceive);

	return TRUE;
}

BOOL QVStartSend(const char *filename)
{
	if (FileVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar_(&FileVar)) {
		return FALSE;
	}

	FileVar->DirLen = 0;
	strncpy_s(FileVar->FullName, sizeof(FileVar->FullName),filename, _TRUNCATE);
	FileVar->NumFname = 1;
	FileVar->NoMsg = TRUE;
	QVStart(IdQVSend);

	return TRUE;
}

BOOL IsFileVarNULL()
{
	return FileVar == NULL;
}
