/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2020 TeraTerm Project
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
#include <io.h>
#include <process.h>
#include <windows.h>
#include <htmlhelp.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include "ftdlg.h"
#include "protodlg.h"
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttdde.h"
#include "ttlib.h"
#include "dlglib.h"
#include "vtterm.h"
#include "win16api.h"
#include "ftlib.h"
#include "buffer.h"
#include "helpid.h"
#include "layer_for_unicode.h"

#include "filesys.h"
#include "tt_res.h"
#include "filesys_log_res.h"

#define FS_BRACKET_NONE  0
#define FS_BRACKET_START 1
#define FS_BRACKET_END   2

static PFileVar LogVar = NULL;
PFileVar SendVar = NULL;
PFileVar FileVar = NULL;
static PCHAR ProtoVar = NULL;
static int ProtoId;

BOOL FileLog = FALSE;
BOOL BinLog = FALSE;
BOOL DDELog = FALSE;
static BOOL FileRetrySend, FileRetryEcho, FileCRSend, FileReadEOF, BinaryMode;
static BYTE FileByte;

#define FILE_SEND_BUF_SIZE  8192
struct FileSendHandler {
	CHAR buf[FILE_SEND_BUF_SIZE];
	int pos;
	int end;
};
static struct FileSendHandler FileSendHandler;
static int FileDlgRefresh;

static int FileBracketMode = FS_BRACKET_NONE;
static int FileBracketPtr = 0;
static char BracketStartStr[] = "\033[200~";
static char BracketEndStr[] = "\033[201~";

static BOOL FSend = FALSE;

HWND HWndLog = NULL; //steven add

static HMODULE HTTFILE = NULL;
static int TTFILECount = 0;

PGetSetupFname GetSetupFname;
static PGetTransFname GetTransFname;
PGetMultiFname GetMultiFname;
PGetGetFname GetGetFname;
PSetFileVar SetFileVar;
PGetXFname GetXFname;
PProtoInit ProtoInit;
PProtoParse ProtoParse;
PProtoTimeOutProc ProtoTimeOutProc;
PProtoCancel ProtoCancel;
PTTFILESetUILanguageFile TTFILESetUILanguageFile;
PTTFILESetFileSendFilter TTFILESetFileSendFilter;

#define IdGetSetupFname  1
#define IdGetTransFname  2
#define IdGetMultiFname  3
#define IdGetGetFname	 4
#define IdSetFileVar	 5
#define IdGetXFname	 6

#define IdProtoInit	 7
#define IdProtoParse	 8
#define IdProtoTimeOutProc 9
#define IdProtoCancel	 10

#define IdTTFILESetUILanguageFile 11
#define IdTTFILESetFileSendFilter 12

/*
   Line Head flag for timestamping
   2007.05.24 Gentaro
*/
enum enumLineEnd {
	Line_Other = 0,
	Line_LineHead = 1,
	Line_FileHead = 2,
};

enum enumLineEnd eLineEnd = Line_LineHead;


// �x���������ݗp�X���b�h�̃��b�Z�[�W
#define WM_DPC_LOGTHREAD_SEND (WM_APP + 1)

static void CloseFileSync(PFileVar ptr);


BOOL LoadTTFILE()
{
	BOOL Err;

	if (HTTFILE != NULL)
	{
		TTFILECount++;
		return TRUE;
	}
	else
		TTFILECount = 0;

	HTTFILE = LoadHomeDLL("TTPFILE.DLL");
	if (HTTFILE == NULL)
		return FALSE;

	Err = FALSE;

	GetSetupFname = (PGetSetupFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetSetupFname));
	if (GetSetupFname==NULL)
		Err = TRUE;

	GetTransFname = (PGetTransFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetTransFname));
	if (GetTransFname==NULL)
		Err = TRUE;

	GetMultiFname = (PGetMultiFname)GetProcAddress(HTTFILE,
	                                               MAKEINTRESOURCE(IdGetMultiFname));
	if (GetMultiFname==NULL)
		Err = TRUE;

	GetGetFname = (PGetGetFname)GetProcAddress(HTTFILE,
	                                           MAKEINTRESOURCE(IdGetGetFname));
	if (GetGetFname==NULL)
		Err = TRUE;

	SetFileVar = (PSetFileVar)GetProcAddress(HTTFILE,
	                                         MAKEINTRESOURCE(IdSetFileVar));
	if (SetFileVar==NULL)
		Err = TRUE;

	GetXFname = (PGetXFname)GetProcAddress(HTTFILE,
	                                       MAKEINTRESOURCE(IdGetXFname));
	if (GetXFname==NULL)
		Err = TRUE;

	ProtoInit = (PProtoInit)GetProcAddress(HTTFILE,
	                                       MAKEINTRESOURCE(IdProtoInit));
	if (ProtoInit==NULL)
		Err = TRUE;

	ProtoParse = (PProtoParse)GetProcAddress(HTTFILE,
	                                         MAKEINTRESOURCE(IdProtoParse));
	if (ProtoParse==NULL)
		Err = TRUE;

	ProtoTimeOutProc = (PProtoTimeOutProc)GetProcAddress(HTTFILE,
	                                                     MAKEINTRESOURCE(IdProtoTimeOutProc));
	if (ProtoTimeOutProc==NULL)
		Err = TRUE;

	ProtoCancel = (PProtoCancel)GetProcAddress(HTTFILE,
	                                           MAKEINTRESOURCE(IdProtoCancel));
	if (ProtoCancel==NULL)
		Err = TRUE;

	TTFILESetUILanguageFile = (PTTFILESetUILanguageFile)GetProcAddress(HTTFILE,
	                                                                   MAKEINTRESOURCE(IdTTFILESetUILanguageFile));
	if (TTFILESetUILanguageFile==NULL) {
		Err = TRUE;
	}
	else {
		TTFILESetUILanguageFile(ts.UILanguageFile);
	}

	TTFILESetFileSendFilter = (PTTFILESetFileSendFilter)GetProcAddress(HTTFILE,
	                                                                   MAKEINTRESOURCE(IdTTFILESetFileSendFilter));
	if (TTFILESetFileSendFilter==NULL) {
		Err = TRUE;
	}
	else {
		TTFILESetFileSendFilter(ts.FileSendFilter);
	}

	if (Err)
	{
		FreeLibrary(HTTFILE);
		HTTFILE = NULL;
		return FALSE;
	}
	else {
		TTFILECount = 1;
		return TRUE;
	}
}

BOOL FreeTTFILE()
{
	if (TTFILECount==0)
		return FALSE;
	TTFILECount--;
	if (TTFILECount>0)
		return TRUE;
	if (HTTFILE!=NULL)
	{
		FreeLibrary(HTTFILE);
		HTTFILE = NULL;
	}
	return TRUE;
}

static PFileTransDlg FLogDlg = NULL;
static PFileTransDlg SendDlg = NULL;
static PProtoDlg PtDlg = NULL;

static BOOL OpenFTDlg(PFileVar fv)
{
	PFileTransDlg FTDlg;

	FTDlg = new CFileTransDlg();

	fv->StartTime = 0;
	fv->ProgStat = 0;

	if (fv->OpId != OpLog) {
		fv->HideDialog = ts.FTHideDialog;
	}

	if (FTDlg!=NULL)
	{
		FTDlg->Create(hInst, HVTWin, fv, &cv, &ts);
		FTDlg->RefreshNum();
		if (fv->OpId == OpLog) {
			HWndLog = FTDlg->m_hWnd; // steven add
		}
	}

	if (fv->OpId==OpLog)
		FLogDlg = FTDlg; /* Log */
	else
		SendDlg = FTDlg; /* File send */

	fv->StartTime = GetTickCount();
	if (fv->OpId == OpSendFile) {
		HWND HFTDlg = FTDlg->GetSafeHwnd();
		InitDlgProgress(HFTDlg, IDC_TRANSPROGRESS, &fv->ProgStat);
		ShowWindow(GetDlgItem(HFTDlg, IDC_TRANS_ELAPSED), SW_SHOW);
	}

	return (FTDlg!=NULL);
}

void ShowFTDlg(WORD OpId)
{
	if (OpId == OpLog) {
		if (FLogDlg != NULL) {
			FLogDlg->ShowWindow(SW_SHOWNORMAL);
			SetForegroundWindow(FLogDlg->GetSafeHwnd());
		}
	}
	else {
		if (SendDlg != NULL) {
			SendDlg->ShowWindow(SW_SHOWNORMAL);
			SetForegroundWindow(SendDlg->GetSafeHwnd());
		}
	}
}

BOOL NewFileVar(PFileVar *fv)
{
	if ((*fv)==NULL)
	{
		*fv = (PFileVar)malloc(sizeof(TFileVar));
		if ((*fv)!=NULL)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			memset(*fv, 0, sizeof(TFileVar));
			strncpy_s((*fv)->FullName, sizeof((*fv)->FullName), FileDirExpanded, _TRUNCATE);
			AppendSlash((*fv)->FullName,sizeof((*fv)->FullName));
			(*fv)->DirLen = strlen((*fv)->FullName);
			(*fv)->FileOpen = FALSE;
			(*fv)->OverWrite = ((ts.FTFlag & FT_RENAME) == 0);
			(*fv)->HMainWin = HVTWin;
			(*fv)->Success = FALSE;
			(*fv)->NoMsg = FALSE;
			(*fv)->HideDialog = FALSE;
		}
	}

	return ((*fv)!=NULL);
}

void FreeFileVar(PFileVar *fv)
{
	if ((*fv)!=NULL)
	{
		CloseFileSync(*fv);
		//if ((*fv)->FileOpen) _lclose((*fv)->FileHandle);
		if ((*fv)->FnStrMemHandle != 0)
		{
			GlobalUnlock((*fv)->FnStrMemHandle);
			GlobalFree((*fv)->FnStrMemHandle);
		}
		free(*fv);
		*fv = NULL;
	}
}

/**
 *	�t�@�C����������̒u������
 *		&h	�z�X�g���ɒu�� (2007.5.14)
 *		&p	TCP�|�[�g�ԍ��ɒu�� (2009.6.12)
 *		&u	���O�I�����̃��[�U��
 */
static void ConvertLogname(char *c, int destlen)
{
	char buf[MAXPATHLEN], buf2[MAXPATHLEN], *p = c;
	char tmphost[1024];
	char tmpuser[256+1];
	DWORD len_user = sizeof(tmpuser);

	memset(buf, 0, sizeof(buf));

	while(*p != '\0') {
		if (*p == '&' && *(p+1) != '\0') {
			switch (*(p+1)) {
			  case 'h':
				if (cv.Open) {
					if (cv.PortType == IdTCPIP) {
						// �z�X�g����IPv6�A�h���X���ƁA�t�@�C�����Ɏg�p�ł��Ȃ����������邽�߁A
						// �]�v�ȕ����͍폜����B
						// (2013.3.9 yutaka)
						strncpy_s(tmphost, sizeof(tmphost), ts.HostName, _TRUNCATE);
						//strncpy_s(tmphost, sizeof(tmphost), "2001:0db8:bd05:01d2:288a:1fc0:0001:10ee", _TRUNCATE);
						replaceInvalidFileNameChar(tmphost, '_');
						strncat_s(buf,sizeof(buf), tmphost, _TRUNCATE);
					}
					else if (cv.PortType == IdSerial) {
						strncpy_s(buf2,sizeof(buf2),buf,_TRUNCATE);
						_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%sCOM%d", buf2, ts.ComPort);
					}
				}
				break;
			  case 'p':
				if (cv.Open) {
					if (cv.PortType == IdTCPIP) {
						char port[6];
						_snprintf_s(port, sizeof(port), _TRUNCATE, "%d", ts.TCPPort);
						strncat_s(buf,sizeof(buf),port,_TRUNCATE);
					}
				}
				break;
			  case 'u':
				if (GetUserName(tmpuser, &len_user) != 0) {
					strncat_s(buf,sizeof(buf),tmpuser,_TRUNCATE);
				}
				break;
			  default:
				strncpy_s(buf2,sizeof(buf2),p,2);
				strncat_s(buf,sizeof(buf),buf2,_TRUNCATE);
			}
			p++;
		}
		else {
			strncpy_s(buf2,sizeof(buf2),p,1);
			strncat_s(buf,sizeof(buf),buf2,_TRUNCATE);
		}
		p++;
	}
	strncpy_s(c, destlen, buf, _TRUNCATE);
}

static void FixLogOption()
{
	if (ts.LogBinary) {
		ts.LogTypePlainText = false;
		ts.LogTimestamp = false;
	}
}


// �X���b�h�̏I���ƃt�@�C���̃N���[�Y
static void CloseFileSync(PFileVar ptr)
{
	BOOL ret;

	if (!ptr->FileOpen)
		return;

	if (ptr->LogThread != INVALID_HANDLE_VALUE) {
		// �X���b�h�̏I���҂�
		ret = PostThreadMessage(ptr->LogThreadId, WM_QUIT, 0, 0);
		if (ret != 0) {
			// �X���b�h�L���[�ɃG���L���[�ł����ꍇ�̂ݑ҂����킹���s���B
			WaitForSingleObject(ptr->LogThread, INFINITE);
		}
		else {
			//DWORD code = GetLastError();
		}
		CloseHandle(ptr->LogThread);
		ptr->LogThread = INVALID_HANDLE_VALUE;
	}
	CloseHandle(ptr->FileHandle);
}

// �x���������ݗp�X���b�h
static unsigned _stdcall DeferredLogWriteThread(void *arg)
{
	MSG msg;
	PFileVar fv = (PFileVar)arg;
	PCHAR buf;
	DWORD buflen;
	DWORD wrote;

	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// �X���b�h�L���[�̍쐬���I��������Ƃ��X���b�h�������֒ʒm����B
	if (fv->LogThreadEvent != NULL) {
		SetEvent(fv->LogThreadEvent);
	}

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		switch (msg.message) {
			case WM_DPC_LOGTHREAD_SEND:
				buf = (PCHAR)msg.wParam;
				buflen = (DWORD)msg.lParam;
				WriteFile(LogVar->FileHandle, buf, buflen, &wrote, NULL);
				free(buf);   // �����Ń��������
				break;

			case WM_QUIT:
				goto end;
				break;
		}
	}

end:
	_endthreadex(0);
	return (0);
}

/**
 *	�_�C�A���O�̓��e�� ts �ɏ����߂�
 *
 *	TODO
 *		�_�C�A���O�Őݒ肵���l�͈ꎞ�I�Ȃ��̂�
 *		�ݒ���㏑������̂͗ǂ��Ȃ��̂ł͂Ȃ����낤��?
 */
static void SetLogFlags(HWND Dialog)
{
	WORD BinFlag, val;

	GetRB(Dialog, &BinFlag, IDC_FOPTBIN, IDC_FOPTBIN);
	ts.LogBinary = BinFlag;

	GetRB(Dialog, &val, IDC_FOPTAPPEND, IDC_FOPTAPPEND);
	ts.Append = val;

	if (!BinFlag) {
		GetRB(Dialog, &val, IDC_PLAINTEXT, IDC_PLAINTEXT);
		ts.LogTypePlainText = val;

		GetRB(Dialog, &val, IDC_TIMESTAMP, IDC_TIMESTAMP);
		ts.LogTimestamp = val;
	}

	GetRB(Dialog, &val, IDC_HIDEDIALOG, IDC_HIDEDIALOG);
	ts.LogHideDialog = val;

	GetRB(Dialog, &val, IDC_ALLBUFF_INFIRST, IDC_ALLBUFF_INFIRST);
	ts.LogAllBuffIncludedInFirst = val;

	ts.LogTimestampType = (GetCurSel(Dialog, IDC_TIMESTAMPTYPE) - 1);
}

/**
 *	���O�t�@�C���`�F�b�N
 *
 *	@param[in]	filename
 *	@param[out]	exist	TURE/FALSE
 *	@param[out]	bom		0	no BOM (or file not exist)
 *						1	UTF-8
 *						2	UTF-16LE
 *						3	UTF-16BE
 */
static void CheckLogFile(const char *filename, BOOL *exist, int *bom)
{
	// �t�@�C�������݂���?
	DWORD logdir = GetFileAttributes(filename);
	if ((logdir != INVALID_FILE_ATTRIBUTES) && ((logdir & FILE_ATTRIBUTE_DIRECTORY) == 0)) {
		// �t�@�C���������� , �A�y���h�������
		*exist = TRUE;

		// BOM�L��/�����`�F�b�N
		FILE *fp = fopen(filename, "rb");
		unsigned char tmp[4];
		size_t l = fread(tmp, 1, sizeof(tmp), fp);
		fclose(fp);
		if (l < 2) {
			*bom = 0;
		} else if (l >= 2 && tmp[0] == 0xff && tmp[1] == 0xfe) {
			// UTF-16LE
			*bom = 2;
		} else if (l >= 2 && tmp[0] == 0xfe && tmp[1] == 0xff) {
			// UTF-16BE
			*bom = 3;
		} else if (l >= 3 && tmp[0] == 0xef && tmp[1] == 0xbb && tmp[2] == 0xbf) {
			// UTF-8
			*bom = 1;
		} else {
			*bom = 0;
		}
	}
	else {
		// �t�@�C�����Ȃ��A�V�K
		*exist = FALSE;
		*bom = 0;
	}
}

static void CheckLogFile(HWND Dialog, const char *filename)
{
	BOOL exist;
	int bom;
	CheckLogFile(filename, &exist, &bom);
	if (exist) {
		// �t�@�C�������݂��� -> �A�y���h�������?
		CheckDlgButton(Dialog, IDC_FOPTAPPEND, BST_CHECKED);
		if (bom != 0) {
			// BOM�L��
			CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
		}
		else {
			// BOM�Ȃ�
			CheckDlgButton(Dialog, IDC_BOM, BST_UNCHECKED);
		}
	}
	else {
		// �t�@�C�����Ȃ��A�V�K
		CheckDlgButton(Dialog, IDC_FOPTAPPEND, BST_UNCHECKED);
		CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
	}
}

typedef struct {
	char *filename;
	BOOL append;
	BOOL bom;
} LogDlgData_t;

static INT_PTR CALLBACK LogFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_TABSHEET_TITLE_LOG" },
		{ IDC_FOPTBIN, "DLG_FOPT_BINARY" },
		{ IDC_FOPTAPPEND, "DLG_FOPT_APPEND" },
		{ IDC_PLAINTEXT, "DLG_FOPT_PLAIN" },
		{ IDC_HIDEDIALOG, "DLG_FOPT_HIDEDIALOG" },
		{ IDC_ALLBUFF_INFIRST, "DLG_FOPT_ALLBUFFINFIRST" },
		{ IDC_TIMESTAMP, "DLG_FOPT_TIMESTAMP" },
	};
	static const I18nTextInfo timestamp_list[] = {
		{ "DLG_FOPT_TIMESTAMP_LOCAL", L"Local Time" },
		{ "DLG_FOPT_TIMESTAMP_UTC", L"UTC" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_LOGGING", L"Elapsed Time (Logging)" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_CONNECTION", L"Elapsed Time (Connection)" },
	};
	const char *UILanguageFile = ts.UILanguageFile;
	LogDlgData_t *data = (LogDlgData_t *)GetWindowLongPtr(Dialog, DWLP_USER);

	if (Message == 	RegisterWindowMessage(HELPMSGSTRING)) {
		// �R�����_�C�A���O����̃w���v���b�Z�[�W��t���ւ���
		Message = WM_COMMAND;
		wParam = IDHELP;
	}
	switch (Message) {
	case WM_INITDIALOG: {
		data = (LogDlgData_t *)lParam;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)data);
		::DragAcceptFiles(Dialog, TRUE);

		SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
		SetI18nList("Tera Term", Dialog, IDC_TIMESTAMPTYPE, timestamp_list, _countof(timestamp_list),
					UILanguageFile, 0);

		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-8");
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);

		SetDlgItemTextA(Dialog, IDC_FOPT_FILENAME_EDIT, data->filename);
		free(data->filename);
		data->filename = NULL;

		// Binary/Text �`�F�b�N�{�b�N�X
		if (ts.LogBinary) {
			SendDlgItemMessage(Dialog, IDC_FOPTBIN, BM_SETCHECK, BST_CHECKED, 0);
		}
		else {
			SendDlgItemMessage(Dialog, IDC_FOPTTEXT, BM_SETCHECK, BST_CHECKED, 0);
		}

		// Append �`�F�b�N�{�b�N�X
		if (ts.Append) {
			SetRB(Dialog, 1, IDC_FOPTAPPEND, IDC_FOPTAPPEND);
		}

		// Plain Text �`�F�b�N�{�b�N�X
		if (ts.LogBinary) {
			// Binary�t���O���L���ȂƂ��̓`�F�b�N�ł��Ȃ�
			DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_PLAINTEXT);
		}
		else if (ts.LogTypePlainText) {
			SetRB(Dialog, 1, IDC_PLAINTEXT, IDC_PLAINTEXT);
		}

		// Hide dialog�`�F�b�N�{�b�N�X (2008.1.30 maya)
		if (ts.LogHideDialog) {
			SetRB(Dialog, 1, IDC_HIDEDIALOG, IDC_HIDEDIALOG);
		}

		// Include screen buffer�`�F�b�N�{�b�N�X (2013.9.29 yutaka)
		if (ts.LogAllBuffIncludedInFirst) {
			SetRB(Dialog, 1, IDC_ALLBUFF_INFIRST, IDC_ALLBUFF_INFIRST);
		}

		// timestamp�`�F�b�N�{�b�N�X (2006.7.23 maya)
		if (ts.LogBinary) {
			// Binary�t���O���L���ȂƂ��̓`�F�b�N�ł��Ȃ�
			DisableDlgItem(Dialog, IDC_TIMESTAMP, IDC_TIMESTAMP);
		}
		else if (ts.LogTimestamp) {
			SetRB(Dialog, 1, IDC_TIMESTAMP, IDC_TIMESTAMP);
		}

		// timestamp ���
		int tstype = ts.LogTimestampType == TIMESTAMP_LOCAL ? 0 :
				ts.LogTimestampType == TIMESTAMP_UTC ? 1 :
				ts.LogTimestampType == TIMESTAMP_ELAPSED_LOGSTART ? 2 :
				ts.LogTimestampType == TIMESTAMP_ELAPSED_CONNECTED ? 3 : 0;
		SendDlgItemMessage(Dialog, IDC_TIMESTAMPTYPE, CB_SETCURSEL, tstype, 0);
		if (ts.LogBinary || !ts.LogTimestamp) {
			DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
		}

		CenterWindow(Dialog, GetParent(Dialog));

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			char filename[MAX_PATH];
			GetDlgItemTextA(Dialog, IDC_FOPT_FILENAME_EDIT, filename, _countof(filename));
			data->filename = _strdup(filename);
			data->append = IsDlgButtonChecked(Dialog, IDC_FOPTAPPEND) == BST_CHECKED;
			data->bom = IsDlgButtonChecked(Dialog, IDC_BOM) == BST_CHECKED;
			SetLogFlags(Dialog);
			EndDialog(Dialog, IDOK);
			break;
		}
		case IDCANCEL:
			EndDialog(Dialog, IDCANCEL);
			break;
		case IDHELP:
			OpenHelp(HH_HELP_CONTEXT, HlpFileLog, ts.UILanguageFile);
			break;
		case IDC_FOPT_FILENAME_BUTTON: {
			/* save current dir */
			wchar_t curdir[MAXPATHLEN];
			_GetCurrentDirectoryW(_countof(curdir), curdir);

			char fname[MAX_PATH];
			GetDlgItemTextA(Dialog, IDC_FOPT_FILENAME_EDIT, fname, _countof(fname));

			char FNFilter[128*3];
			get_lang_msg("FILEDLG_ALL_FILTER", FNFilter, sizeof(FNFilter), "All(*.*)\\0*.*\\0\\0", UILanguageFile);

			char caption[MAX_PATH];
			char uimsg[MAX_UIMSG];
			get_lang_msg("FILEDLG_TRANS_TITLE_LOG", uimsg, sizeof(uimsg), TitLog, UILanguageFile);
			strncpy_s(caption, sizeof(caption),"Tera Term: ", _TRUNCATE);
			strncat_s(caption, sizeof(caption), uimsg, _TRUNCATE);

			OPENFILENAME ofn = {};
			ofn.lStructSize = get_OPENFILENAME_SIZEA();
			//ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			ofn.Flags |= OFN_EXPLORER | OFN_ENABLESIZING;
			ofn.Flags |= OFN_SHOWHELP;
			ofn.Flags |= OFN_NOCHANGEDIR;		// ���܂����삵�Ȃ���������悤��
			ofn.hwndOwner = Dialog;
			ofn.lpstrFilter = FNFilter;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = fname;
			ofn.nMaxFile = sizeof(fname);
			ofn.lpstrTitle = caption;
			BOOL Ok = GetSaveFileName(&ofn);
			if (Ok) {
				SetDlgItemTextA(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
			}

			/* restore dir */
			_SetCurrentDirectoryW(curdir);

			break;
		}
		case IDC_FOPTBIN:
			DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_TIMESTAMP);
			DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
			EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), FALSE);
			break;
		case IDC_FOPTTEXT:
			EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), TRUE);
			EnableDlgItem(Dialog, IDC_PLAINTEXT, IDC_TIMESTAMP);
			// FALLTHROUGH -- BinFlag �� off �̎��� Timestamp ��ʂ̗L��/������ݒ肷��
		case IDC_TIMESTAMP:
			if (IsDlgButtonChecked(Dialog, IDC_TIMESTAMP) == BST_CHECKED) {
				EnableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
			}
			else {
				DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
			}
			break;
		case IDC_FOPT_FILENAME_EDIT:
			if (HIWORD(wParam) == EN_CHANGE){
				char filename[MAX_PATH];
				GetDlgItemTextA(Dialog, IDC_FOPT_FILENAME_EDIT, filename, _countof(filename));
				CheckLogFile(Dialog, filename);
			}
			break;
		}
		break;
	case WM_DROPFILES: {
		// �����h���b�v����Ă��ŏ���1����������
		HDROP hDrop = (HDROP)wParam;
		const UINT len = _DragQueryFileW(hDrop, 0, NULL, 0);
		if (len == 0) {
			DragFinish(hDrop);
			return TRUE;
		}
		wchar_t *filename = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
		_DragQueryFileW(hDrop, 0, filename, len + 1);
		filename[len] = '\0';
		_SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename);
		SendDlgItemMessage(Dialog, IDC_FOPT_FILENAME_EDIT, EM_SETSEL, len, len);
		free(filename);
		DragFinish(hDrop);
		return TRUE;
	}
	}
	return FALSE;
}

static BOOL LogStart()
{
	unsigned tid;

	if ((FileLog) || (BinLog)) return FALSE;

	assert(LogVar != NULL);

	if (strlen(&(LogVar->FullName[LogVar->DirLen]))==0) {
		// �t�@�C�������ݒ肳��Ă��Ȃ�
		return FALSE;
	}

	if (! LoadTTFILE()) return FALSE;

	LogVar->OpId = OpLog;
	(*SetFileVar)(LogVar);
	FixLogOption();

	if (ts.LogBinary > 0)
	{
		BinLog = TRUE;
		FileLog = FALSE;
		if (! CreateBinBuf())
		{
			FileTransEnd(OpLog);
			return FALSE;
		}
	}
	else {
		BinLog = FALSE;
		FileLog = TRUE;
		if (! CreateLogBuf())
		{
			FileTransEnd(OpLog);
			return FALSE;
		}
	}
	cv.LStart = cv.LogPtr;
	cv.LCount = 0;
	if (ts.LogHideDialog)
		LogVar->HideDialog = 1;

	/* 2007.05.24 Gentaro */
	eLineEnd = Line_LineHead;

	if (ts.Append > 0)
	{
		int dwShareMode = FILE_SHARE_READ;
		if (!ts.LogLockExclusive) {
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		}
		LogVar->FileHandle = CreateFile(LogVar->FullName, GENERIC_WRITE, dwShareMode, NULL,
		                                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (LogVar->FileHandle != INVALID_HANDLE_VALUE){
			SetFilePointer(LogVar->FileHandle, 0, NULL, FILE_END);
			/* 2007.05.24 Gentaro
				If log file already exists,
				a newline is inserted before the first timestamp.
			*/
			eLineEnd = Line_FileHead;
		}
	}
	else {
		int dwShareMode = FILE_SHARE_READ;
		if (!ts.LogLockExclusive) {
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		}
		LogVar->FileHandle = CreateFile(LogVar->FullName, GENERIC_WRITE, dwShareMode, NULL,
		                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	LogVar->FileOpen = (LogVar->FileHandle != INVALID_HANDLE_VALUE);
	if (! LogVar->FileOpen)
	{
		char msg[128];

		// �t�@�C���I�[�v���G���[���̃��b�Z�[�W�\����ǉ������B(2008.7.9 yutaka)
		if (LogVar->NoMsg == FALSE) {
			_snprintf_s(msg, sizeof(msg), _TRUNCATE, "Can not create a `%s' file. (%d)", LogVar->FullName, GetLastError());
			MessageBox(NULL, msg, "Tera Term: File open error", MB_OK | MB_ICONERROR);
		}

		FileTransEnd(OpLog);
		return FALSE;
	}
	LogVar->ByteCount = 0;

	// Log rotate configuration
	LogVar->RotateMode = ts.LogRotate;
	LogVar->RotateSize = ts.LogRotateSize;
	LogVar->RotateStep = ts.LogRotateStep;

	// Log rotate���L���̏ꍇ�A�����t�@�C���T�C�Y��ݒ肷��B
	// �ŏ��̃t�@�C�����ݒ肵���T�C�Y�Ń��[�e�[�g���Ȃ����̏C���B
	// (2016.4.9 yutaka)
	if (LogVar->RotateMode != ROTATE_NONE) {
		DWORD size = GetFileSize(LogVar->FileHandle, NULL);
		if (size != -1)
			LogVar->ByteCount = size;
	}

	if (! OpenFTDlg(LogVar)) {
		FileTransEnd(OpLog);
		return FALSE;
	}

	// �x���������ݗp�X���b�h���N�����B
	// (2013.4.19 yutaka)
	// DeferredLogWriteThread �X���b�h���N�����āA�X���b�h�L���[���쐬�������O�ɁA
	// ���O�t�@�C���̃N���[�Y(CloseFileSync)���s����ƁA�G���L���[�����s���A�f�b�h���b�N
	// ����Ƃ��������C�������B
	// �X���b�h�Ԃ̓������s�����߁A���O�Ȃ��C�x���g�I�u�W�F�N�g���g���āA�X���b�h�L���[��
	// �쐬�܂ő҂����킹����悤�ɂ����B���O�t���C�x���g�I�u�W�F�N�g���g���ꍇ�́A
	// �V�X�e��(Windows OS)��Ń��j�[�N�Ȗ��O�ɂ���K�v������B
	// (2016.9.23 yutaka)
	LogVar->LogThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	LogVar->LogThread = (HANDLE)_beginthreadex(NULL, 0, DeferredLogWriteThread, LogVar, 0, &tid);
	LogVar->LogThreadId = tid;
	if (LogVar->LogThreadEvent != NULL) {
		WaitForSingleObject(LogVar->LogThreadEvent, INFINITE);
		CloseHandle(LogVar->LogThreadEvent);
	}

	// ���݃o�b�t�@�ɂ���f�[�^�����ׂď����o���Ă���A
	// ���O�̎���J�n����B
	// (2013.9.29 yutaka)
	if (ts.LogAllBuffIncludedInFirst) {
		DWORD ofs, size, written_size;
		char buf[512];
		const char *crlf = "\r\n";
		DWORD crlf_len = 2;
		for (ofs = 0 ;  ; ofs++ ) {
			// 1�̍s���擾����B���������Ȃ̂ŁA�G�X�P�[�v�V�[�P���X�͊܂܂�Ȃ��B
			size = BuffGetAnyLineData(ofs, buf, sizeof(buf));
			if (size == -1)
				break;

#if 0
			if (ts.DeferredLogWriteMode) { // �x����������
				char *pbuf = (char *)malloc(size + 2);
				memcpy(pbuf, buf, size);
				pbuf[size] = '\r';
				pbuf[size+1] = '\n';
				Sleep(1);  // �X���b�h�L���[�������悤�ɁA�R���e�L�X�g�X�C�b�`�𑣂��B
				PostThreadMessage(LogVar->LogThreadId, WM_DPC_LOGTHREAD_SEND, (WPARAM)pbuf, size + 2);
			} else { // �������B�l�b�g���[�N�o�R���ƒx���B
#endif
				WriteFile(LogVar->FileHandle, buf, size, &written_size, NULL);
				WriteFile(LogVar->FileHandle, crlf, crlf_len, &written_size, NULL);
#if 0
			}
#endif
		}
	}

	return TRUE;
}

void LogPut1(BYTE b)
{
	cv.LogBuf[cv.LogPtr] = b;
	cv.LogPtr++;
	if (cv.LogPtr>=InBuffSize)
		cv.LogPtr = cv.LogPtr-InBuffSize;

	if (FileLog)
	{
		if (cv.LCount>=InBuffSize)
		{
			cv.LCount = InBuffSize;
			cv.LStart = cv.LogPtr;
		}
		else
			cv.LCount++;
	}
	else
		cv.LCount = 0;

	if (DDELog)
	{
		if (cv.DCount>=InBuffSize)
		{
			cv.DCount = InBuffSize;
			cv.DStart = cv.LogPtr;
		}
		else
			cv.DCount++;
	}
	else {
		cv.DCount = 0;
		// ���O�̎撆�Ƀ}�N�����X�g�[��������ւ̏C���B
		// ���O�̎撆�Ɉ�x�}�N�����~�߂�ƁA�o�b�t�@�̃C���f�b�N�X���������Ȃ��Ȃ�A
		// �ēx�}�N���𗬂��Ă��������f�[�^������Ȃ��̂������B
		// �}�N�����~��������Ԃł��C���f�b�N�X�̓��������悤�ɂ����B
		// (2006.12.26 yutaka)
		cv.DStart = cv.LogPtr;
	}
}

static BOOL Get1(PCHAR Buf, int *Start, int *Count, PBYTE b)
{
	if (*Count<=0) return FALSE;
	*b = Buf[*Start];
	(*Start)++;
	if (*Start>=InBuffSize)
		*Start = *Start-InBuffSize;
	(*Count)--;
	return TRUE;
}



static CRITICAL_SECTION g_filelog_lock;   /* ���b�N�p�ϐ� */

void logfile_lock_initialize(void)
{
	InitializeCriticalSection(&g_filelog_lock);
}

static inline void logfile_lock(void)
{
	EnterCriticalSection(&g_filelog_lock);
}

static inline void logfile_unlock(void)
{
	LeaveCriticalSection(&g_filelog_lock);
}

 // �R�����g�����O�֒ǉ�����
static void CommentLogToFile(char *buf, int size)
{
	DWORD wrote;

	if (LogVar == NULL || !LogVar->FileOpen) {
		char uimsg[MAX_UIMSG];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
		get_lang_msg("MSG_COMMENT_LOG_OPEN_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
		             "It is not opened by the log file yet.", ts.UILanguageFile);
		::MessageBox(NULL, ts.UIMsg, uimsg, MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	logfile_lock();
	WriteFile(LogVar->FileHandle, buf, size, &wrote, NULL);
	WriteFile(LogVar->FileHandle, "\r\n", 2, &wrote, NULL); // ���s
	/* Set Line End Flag
		2007.05.24 Gentaro
	*/
	eLineEnd = Line_LineHead;
	logfile_unlock();
}

// ���O�����[�e�[�g����B
// (2013.3.21 yutaka)
static void LogRotate(void)
{
	int loopmax = 10000;  // XXX
	char filename[1024];
	char newfile[1024], oldfile[1024];
	int i, k;
	int dwShareMode = FILE_SHARE_READ;
	unsigned tid;

	if (! LogVar->FileOpen) return;

	if (LogVar->RotateMode == ROTATE_NONE)
		return;

	if (LogVar->RotateMode == ROTATE_SIZE) {
		if (LogVar->ByteCount <= LogVar->RotateSize)
			return;
		//OutputDebugPrintf("%s: mode %d size %ld\n", __FUNCTION__, LogVar->RotateMode, LogVar->ByteCount);
	} else {
		return;
	}

	logfile_lock();
	// ���O�T�C�Y���ď���������B
	LogVar->ByteCount = 0;

	// �������񍡂̃t�@�C�����N���[�Y���āA�ʖ��̃t�@�C�����I�[�v������B
	CloseFileSync(LogVar);
	//_lclose(LogVar->FileHandle);

	// ���ネ�[�e�[�V�����̃X�e�b�v���̎w�肪���邩
	if (LogVar->RotateStep > 0)
		loopmax = LogVar->RotateStep;

	for (i = 1 ; i <= loopmax ; i++) {
		_snprintf_s(filename, sizeof(filename), _TRUNCATE, "%s.%d", LogVar->FullName, i);
		if (_access_s(filename, 0) != 0)
			break;
	}
	if (i > loopmax) {
		// ���オ�����ς��ɂȂ�����A�ŌẪt�@�C������p������B
		i = loopmax;
	}

	// �ʃt�@�C���Ƀ��l�[���B
	for (k = i-1 ; k >= 0 ; k--) {
		if (k == 0)
			strncpy_s(oldfile, sizeof(oldfile), LogVar->FullName, _TRUNCATE);
		else
			_snprintf_s(oldfile, sizeof(oldfile), _TRUNCATE, "%s.%d", LogVar->FullName, k);
		_snprintf_s(newfile, sizeof(newfile), _TRUNCATE, "%s.%d", LogVar->FullName, k+1);
		remove(newfile);
		if (rename(oldfile, newfile) != 0) {
			OutputDebugPrintf("%s: rename %d\n", __FUNCTION__, errno);
		}
	}

	// �ăI�[�v��
	if (!ts.LogLockExclusive) {
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}
	LogVar->FileHandle = CreateFile(LogVar->FullName, GENERIC_WRITE, dwShareMode, NULL,
	                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// �x���������ݗp�X���b�h���N�����B
	// (2013.4.19 yutaka)
	// DeferredLogWriteThread �X���b�h���N�����āA�X���b�h�L���[���쐬�������O�ɁA
	// ���O�t�@�C���̃N���[�Y(CloseFileSync)���s����ƁA�G���L���[�����s���A�f�b�h���b�N
	// ����Ƃ��������C�������B
	// �X���b�h�Ԃ̓������s�����߁A���O�Ȃ��C�x���g�I�u�W�F�N�g���g���āA�X���b�h�L���[��
	// �쐬�܂ő҂����킹����悤�ɂ����B���O�t���C�x���g�I�u�W�F�N�g���g���ꍇ�́A
	// �V�X�e��(Windows OS)��Ń��j�[�N�Ȗ��O�ɂ���K�v������B
	// (2016.9.26 yutaka)
	LogVar->LogThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	LogVar->LogThread = (HANDLE)_beginthreadex(NULL, 0, DeferredLogWriteThread, LogVar, 0, &tid);
	LogVar->LogThreadId = tid;
	if (LogVar->LogThreadEvent != NULL) {
		WaitForSingleObject(LogVar->LogThreadEvent, INFINITE);
		CloseHandle(LogVar->LogThreadEvent);
	}

	logfile_unlock();

}

void LogToFile()
{
	PCHAR Buf;
	int Start, Count;
	BYTE b;
	PCHAR WriteBuf;
	DWORD WriteBufMax, WriteBufLen;
	CHAR tmp[128];
	DWORD wrote;

	if (! LogVar->FileOpen) return;
	if (FileLog)
	{
		Buf = cv.LogBuf;
		Start = cv.LStart;
		Count = cv.LCount;
	}
	else if (BinLog)
	{
		Buf = cv.BinBuf;
		Start = cv.BStart;
		Count = cv.BCount;
	}
	else
		return;

	if (Buf==NULL) return;
	if (Count==0) return;

	// ���b�N�����(2004.8.6 yutaka)
	logfile_lock();

	if (ts.DeferredLogWriteMode) {
		WriteBufMax = 8192;
		WriteBufLen = 0;
		WriteBuf = (PCHAR)malloc(WriteBufMax);
		while (Get1(Buf,&Start,&Count,&b)) {
			if (((cv.FilePause & OpLog)==0) && (! cv.ProtoFlag))
			{
				tmp[0] = 0;
				if ( ts.LogTimestamp && eLineEnd ) {
					char *strtime = NULL;

					switch (ts.LogTimestampType) {
					case TIMESTAMP_LOCAL:
						strtime = mctimelocal(ts.LogTimestampFormat, FALSE);
						break;
					case TIMESTAMP_UTC:
						strtime = mctimelocal(ts.LogTimestampFormat, TRUE);
						break;
					case TIMESTAMP_ELAPSED_LOGSTART:
						strtime = strelapsed(LogVar->StartTime);
						break;
					case TIMESTAMP_ELAPSED_CONNECTED:
						strtime = strelapsed(cv.ConnectedTime);
						break;
					}

					/* 2007.05.24 Gentaro */
					if( eLineEnd == Line_FileHead ){
						strncat_s(tmp, sizeof(tmp), "\r\n", _TRUNCATE);
					}
					strncat_s(tmp, sizeof(tmp), "[", _TRUNCATE);
					strncat_s(tmp, sizeof(tmp), strtime, _TRUNCATE);
					strncat_s(tmp, sizeof(tmp), "] ", _TRUNCATE);
				}

				/* 2007.05.24 Gentaro */
				if( b == 0x0a ){
					eLineEnd = Line_LineHead; /* set endmark*/
				}
				else {
					eLineEnd = Line_Other; /* clear endmark*/
				}

				if (WriteBufLen >= (WriteBufMax*4/5)) {
					WriteBufMax *= 2;
					WriteBuf = (PCHAR)realloc(WriteBuf, WriteBufMax);
				}
				memcpy(&WriteBuf[WriteBufLen], tmp, strlen(tmp));
				WriteBufLen += strlen(tmp);
				WriteBuf[WriteBufLen++] = b;

				(LogVar->ByteCount)++;
			}
		}

		PostThreadMessage(LogVar->LogThreadId, WM_DPC_LOGTHREAD_SEND, (WPARAM)WriteBuf, WriteBufLen);

	} else {

		while (Get1(Buf,&Start,&Count,&b))
		{
			if (((cv.FilePause & OpLog)==0) && (! cv.ProtoFlag))
			{
				if ( ts.LogTimestamp && eLineEnd ) {
					char *strtime = NULL;

					switch (ts.LogTimestampType) {
					case TIMESTAMP_LOCAL:
						strtime = mctimelocal(ts.LogTimestampFormat, FALSE);
						break;
					case TIMESTAMP_UTC:
						strtime = mctimelocal(ts.LogTimestampFormat, TRUE);
						break;
					case TIMESTAMP_ELAPSED_LOGSTART:
						strtime = strelapsed(LogVar->StartTime);
						break;
					case TIMESTAMP_ELAPSED_CONNECTED:
						strtime = strelapsed(cv.ConnectedTime);
						break;
					}
					WriteFile(LogVar->FileHandle, "[", 1, &wrote, NULL);
					WriteFile(LogVar->FileHandle, strtime, strlen(strtime), &wrote, NULL);
					WriteFile(LogVar->FileHandle, "] ", 2, &wrote, NULL);
				}

				/* 2007.05.24 Gentaro */
				if( b == 0x0a ){
					eLineEnd = Line_LineHead; /* set endmark*/
				}
				else {
					eLineEnd = Line_Other; /* clear endmark*/
				}

				WriteFile(LogVar->FileHandle, (PCHAR)&b, 1, &wrote, NULL);
				(LogVar->ByteCount)++;
			}
		}

	}

	logfile_unlock();

	if (FileLog)
	{
		cv.LStart = Start;
		cv.LCount = Count;
	}
	else {
		cv.BStart = Start;
		cv.BCount = Count;
	}
	if (((cv.FilePause & OpLog) !=0) || cv.ProtoFlag) return;
	if (FLogDlg!=NULL)
		FLogDlg->RefreshNum();

	// ���O�E���[�e�[�g
	LogRotate();

}

BOOL CreateLogBuf()
{
	if (cv.HLogBuf==NULL)
	{
		cv.HLogBuf = GlobalAlloc(GMEM_MOVEABLE,InBuffSize);
		cv.LogBuf = NULL;
		cv.LogPtr = 0;
		cv.LStart = 0;
		cv.LCount = 0;
		cv.DStart = 0;
		cv.DCount = 0;
	}
	return (cv.HLogBuf!=NULL);
}

void FreeLogBuf()
{
	if ((cv.HLogBuf==NULL) || FileLog || DDELog)
		return;
	if (cv.LogBuf!=NULL)
		GlobalUnlock(cv.HLogBuf);
	GlobalFree(cv.HLogBuf);
	cv.HLogBuf = NULL;
	cv.LogBuf = NULL;
	cv.LogPtr = 0;
	cv.LStart = 0;
	cv.LCount = 0;
	cv.DStart = 0;
	cv.DCount = 0;
}

BOOL CreateBinBuf()
{
	if (cv.HBinBuf==NULL)
	{
		cv.HBinBuf = GlobalAlloc(GMEM_MOVEABLE,InBuffSize);
		cv.BinBuf = NULL;
		cv.BinPtr = 0;
		cv.BStart = 0;
		cv.BCount = 0;
	}
	return (cv.HBinBuf!=NULL);
}

void FreeBinBuf()
{
	if ((cv.HBinBuf==NULL) || BinLog)
		return;
	if (cv.BinBuf!=NULL)
		GlobalUnlock(cv.HBinBuf);
	GlobalFree(cv.HBinBuf);
	cv.HBinBuf = NULL;
	cv.BinBuf = NULL;
	cv.BinPtr = 0;
	cv.BStart = 0;
	cv.BCount = 0;
}

void FileSendStart()
{
	LONG Option = 0;

	if (! cv.Ready || FSend) return;
	if (cv.ProtoFlag)
	{
		FreeFileVar(&SendVar);
		return;
	}

	if (! LoadTTFILE())
		return;
	if (! NewFileVar(&SendVar))
	{
		FreeTTFILE();
		return;
	}
	SendVar->OpId = OpSendFile;

	FSend = TRUE;

	if (strlen(&(SendVar->FullName[SendVar->DirLen])) == 0) {
		char FileDirExpanded[MAX_PATH];
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
		if (ts.TransBin)
			Option |= LOGDLG_BINARY;
		SendVar->FullName[0] = 0;
		if (! (*GetTransFname)(SendVar, FileDirExpanded, GTF_SEND, &Option)) {
			FileTransEnd(OpSendFile);
			return;
		}
		ts.TransBin = CheckFlag(Option, LOGDLG_BINARY);
	}
	else
		(*SetFileVar)(SendVar);

	SendVar->FileHandle = CreateFile(SendVar->FullName, GENERIC_READ, FILE_SHARE_READ, NULL,
	                                 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	SendVar->FileOpen = (SendVar->FileHandle != INVALID_HANDLE_VALUE);
	if (! SendVar->FileOpen)
	{
		FileTransEnd(OpSendFile);
		return;
	}
	SendVar->ByteCount = 0;
	SendVar->FileSize = GetFSize(SendVar->FullName);

	TalkStatus = IdTalkFile;
	FileRetrySend = FALSE;
	FileRetryEcho = FALSE;
	FileCRSend = FALSE;
	FileReadEOF = FALSE;
	FileSendHandler.pos = 0;
	FileSendHandler.end = 0;
	FileDlgRefresh = 0;

	if (BracketedPasteMode()) {
		FileBracketMode = FS_BRACKET_START;
		FileBracketPtr = 0;
		BinaryMode = TRUE;
	}
	else {
		FileBracketMode = FS_BRACKET_NONE;
		BinaryMode = ts.TransBin;
	}

	if (! OpenFTDlg(SendVar))
		FileTransEnd(OpSendFile);
}

void FileTransEnd(WORD OpId)
/* OpId = 0: close Log and FileSend
      OpLog: close Log
 OpSendFile: close FileSend */
{
	if (((OpId==0) || (OpId==OpLog)) && (FileLog || BinLog))
	{
		FileLog = FALSE;
		BinLog = FALSE;
		if (FLogDlg!=NULL)
		{
			FLogDlg->DestroyWindow();
			FLogDlg = NULL;
			HWndLog = NULL; // steven add
		}
		FreeFileVar(&LogVar);
		FreeLogBuf();
		FreeBinBuf();
		FreeTTFILE();
	}

	if (((OpId==0) || (OpId==OpSendFile)) && FSend)
	{
		FSend = FALSE;
		TalkStatus = IdTalkKeyb;
		if (SendDlg!=NULL)
		{
			SendDlg->DestroyWindow();
			SendDlg = NULL;
		}
		FreeFileVar(&SendVar);
		FreeTTFILE();
	}

	EndDdeCmnd(0);
}

int FSOut1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryOut(&cv,(PCHAR)&b,1);
	else if ((b>=0x20) || (b==0x09) || (b==0x0A) || (b==0x0D))
		return CommTextOut(&cv,(PCHAR)&b,1);
	else
		return 1;
}

int FSEcho1(BYTE b)
{
	if (BinaryMode)
		return CommBinaryEcho(&cv,(PCHAR)&b,1);
	else
		return CommTextEcho(&cv,(PCHAR)&b,1);
}

// �ȉ��̎��͂�����̊֐����g��
// - BinaryMode == true
// - FileBracketMode == false
// - cv.TelFlag == false
// - ts.LocalEcho == 0
void FileSendBinayBoost()
{
	WORD c, fc;
	LONG BCOld;
	DWORD read_bytes;

	if ((SendDlg == NULL) ||
		((cv.FilePause & OpSendFile) != 0))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
			FileSendHandler.end - FileSendHandler.pos);
		FileSendHandler.pos += c;
		FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
		if (FileRetrySend)
			return;
	}

	do {
		if (FileSendHandler.pos == FileSendHandler.end) {
			ReadFile(SendVar->FileHandle, &(FileSendHandler.buf[0]), sizeof(FileSendHandler.buf), &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			FileSendHandler.pos = 0;
			FileSendHandler.end = fc;
		} else {
			fc = FileSendHandler.end - FileSendHandler.end;
		}

		if (fc != 0)
		{
			c = CommRawOut(&cv, &(FileSendHandler.buf[FileSendHandler.pos]),
				FileSendHandler.end - FileSendHandler.pos);
			FileSendHandler.pos += c;
			FileRetrySend = (FileSendHandler.end != FileSendHandler.pos);
			SendVar->ByteCount = SendVar->ByteCount + c;
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld)
					SendDlg->RefreshNum();
				return;
			}
		}
		FileDlgRefresh = SendVar->ByteCount;
		SendDlg->RefreshNum();
		BCOld = SendVar->ByteCount;
		if (fc != 0)
			return;
	} while (fc != 0);

	FileTransEnd(OpSendFile);
}

void FileSend()
{
	WORD c, fc;
	LONG BCOld;
	DWORD read_bytes;

	if (cv.PortType == IdSerial && ts.FileSendHighSpeedMode &&
	    BinaryMode && !FileBracketMode && !cv.TelFlag &&
	    (ts.LocalEcho == 0) && (ts.Baud >= 115200)) {
		return FileSendBinayBoost();
	}

	if ((SendDlg==NULL) ||
	    ((cv.FilePause & OpSendFile) !=0))
		return;

	BCOld = SendVar->ByteCount;

	if (FileRetrySend)
	{
		FileRetryEcho = (ts.LocalEcho>0);
		c = FSOut1(FileByte);
		FileRetrySend = (c==0);
		if (FileRetrySend)
			return;
	}

	if (FileRetryEcho)
	{
		c = FSEcho1(FileByte);
		FileRetryEcho = (c==0);
		if (FileRetryEcho)
			return;
	}

	do {
		if (FileBracketMode == FS_BRACKET_START) {
			FileByte = BracketStartStr[FileBracketPtr++];
			fc = 1;

			if (FileBracketPtr >= sizeof(BracketStartStr) - 1) {
				FileBracketMode = FS_BRACKET_END;
				FileBracketPtr = 0;
				BinaryMode = ts.TransBin;
			}
		}
		else if (! FileReadEOF) {
			ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
			fc = LOWORD(read_bytes);
			SendVar->ByteCount = SendVar->ByteCount + fc;

			if (FileCRSend && (fc==1) && (FileByte==0x0A)) {
				ReadFile(SendVar->FileHandle, &FileByte, 1, &read_bytes, NULL);
				fc = LOWORD(read_bytes);
				SendVar->ByteCount = SendVar->ByteCount + fc;
			}
		}
		else {
			fc = 0;
		}

		if (fc == 0 && FileBracketMode == FS_BRACKET_END) {
			FileReadEOF = TRUE;
			FileByte = BracketEndStr[FileBracketPtr++];
			fc = 1;
			BinaryMode = TRUE;

			if (FileBracketPtr >= sizeof(BracketEndStr) - 1) {
				FileBracketMode = FS_BRACKET_NONE;
				FileBracketPtr = 0;
			}
		}


		if (fc!=0)
		{
			c = FSOut1(FileByte);
			FileCRSend = (ts.TransBin==0) && (FileByte==0x0D);
			FileRetrySend = (c==0);
			if (FileRetrySend)
			{
				if (SendVar->ByteCount != BCOld)
					SendDlg->RefreshNum();
				return;
			}
			if (ts.LocalEcho>0)
			{
				c = FSEcho1(FileByte);
				FileRetryEcho = (c==0);
				if (FileRetryEcho)
					return;
			}
		}
		if ((fc==0) || ((SendVar->ByteCount % 100 == 0) && (FileBracketPtr == 0))) {
			SendDlg->RefreshNum();
			BCOld = SendVar->ByteCount;
			if (fc!=0)
				return;
		}
	} while (fc!=0);

	FileTransEnd(OpSendFile);
}

void FLogChangeButton(BOOL Pause)
{
	if (FLogDlg!=NULL)
		FLogDlg->ChangeButton(Pause);
}

void FLogRefreshNum()
{
	if (FLogDlg!=NULL)
		FLogDlg->RefreshNum();
}

BOOL OpenProtoDlg(PFileVar fv, int IdProto, int Mode, WORD Opt1, WORD Opt2)
{
	int vsize;
	PProtoDlg pd;

	ProtoId = IdProto;

	switch (ProtoId) {
		case PROTO_KMT:
			vsize = sizeof(TKmtVar);
			break;
		case PROTO_XM:
			vsize = sizeof(TXVar);
			break;
		case PROTO_YM:
			vsize = sizeof(TYVar);
			break;
		case PROTO_ZM:
			vsize = sizeof(TZVar);
			break;
		case PROTO_BP:
			vsize = sizeof(TBPVar);
			break;
		case PROTO_QV:
			vsize = sizeof(TQVVar);
			break;
		default:
			vsize = 0;
			assert(FALSE);
			break;
	}
	ProtoVar = (PCHAR)malloc(vsize);
	if (ProtoVar==NULL)
		return FALSE;

	switch (ProtoId) {
		case PROTO_KMT:
			((PKmtVar)ProtoVar)->KmtMode = Mode;
			break;
		case PROTO_XM:
			((PXVar)ProtoVar)->XMode = Mode;
			((PXVar)ProtoVar)->XOpt = Opt1;
			((PXVar)ProtoVar)->TextFlag = 1 - (Opt2 & 1);
			break;
		case PROTO_YM:
			((PYVar)ProtoVar)->YMode = Mode;
			((PYVar)ProtoVar)->YOpt = Opt1;
			break;
		case PROTO_ZM:
			((PZVar)ProtoVar)->BinFlag = (Opt1 & 1) != 0;
			((PZVar)ProtoVar)->ZMode = Mode;
			break;
		case PROTO_BP:
			((PBPVar)ProtoVar)->BPMode = Mode;
			break;
		case PROTO_QV:
			((PQVVar)ProtoVar)->QVMode = Mode;
			break;
	}

	pd = new CProtoDlg();
	if (pd==NULL)
	{
		free(ProtoVar);
		ProtoVar = NULL;
		return FALSE;
	}
	pd->Create(hInst, HVTWin, fv, &ts);

	(*ProtoInit)(ProtoId,FileVar,ProtoVar,&cv,&ts);

	PtDlg = pd;
	return TRUE;
}

void CloseProtoDlg()
{
	if (PtDlg!=NULL)
	{
		PtDlg->DestroyWindow();
		PtDlg = NULL;

		::KillTimer(FileVar->HMainWin,IdProtoTimer);
		if ((ProtoId==PROTO_QV) &&
		    (((PQVVar)ProtoVar)->QVMode==IdQVSend))
			CommTextOut(&cv,"\015",1);
		if (FileVar->LogFlag)
			CloseHandle(FileVar->LogFile);
		FileVar->LogFile = 0;
		if (ProtoVar!=NULL)
		{
			free(ProtoVar);
			ProtoVar = NULL;
		}
	}
}

BOOL ProtoStart()
{
	if (cv.ProtoFlag)
		return FALSE;
	if (FSend)
	{
		FreeFileVar(&FileVar);
		return FALSE;
	}

	if (! LoadTTFILE())
		return FALSE;
	NewFileVar(&FileVar);

	if (FileVar==NULL)
	{
		FreeTTFILE();
		return FALSE;
	}
	cv.ProtoFlag = TRUE;
	return TRUE;
}

void ProtoEnd()
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

	FreeTTFILE();
	FreeFileVar(&FileVar);
}

int ProtoDlgParse()
{
	int P;

	P = ActiveWin;
	if (PtDlg==NULL)
		return P;

	if ((*ProtoParse)(ProtoId,FileVar,ProtoVar,&cv))
		P = 0; /* continue */
	else {
		CommSend(&cv);
		ProtoEnd();
	}
	return P;
}

void ProtoDlgTimeOut()
{
	if (PtDlg!=NULL)
		(*ProtoTimeOutProc)(ProtoId,FileVar,ProtoVar,&cv);
}

void ProtoDlgCancel()
{
	if ((PtDlg!=NULL) &&
	    (*ProtoCancel)(ProtoId,FileVar,ProtoVar,&cv))
		ProtoEnd();
}

void KermitStart(int mode)
{
	WORD w;

	if (! ProtoStart())
		return;

	switch (mode) {
		case IdKmtSend:
			FileVar->OpId = OpKmtSend;
			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				char FileDirExpanded[MAX_PATH];
				ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
				if (!(*GetMultiFname)(FileVar, FileDirExpanded, GMF_KERMIT, &w) ||
				    (FileVar->NumFname==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				(*SetFileVar)(FileVar);
			break;
		case IdKmtReceive:
			FileVar->OpId = OpKmtRcv;
			break;
		case IdKmtGet:
			FileVar->OpId = OpKmtSend;
			if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
			{
				if (! (*GetGetFname)(FileVar->HMainWin,FileVar) ||
				    (strlen(FileVar->FullName)==0))
				{
					ProtoEnd();
					return;
				}
			}
			else
				(*SetFileVar)(FileVar);
			break;
		case IdKmtFinish:
			FileVar->OpId = OpKmtFin;
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

void XMODEMStart(int mode)
{
	LONG Option;
	int tmp;

	if (! ProtoStart())
		return;

	if (mode==IdXReceive)
		FileVar->OpId = OpXRcv;
	else
		FileVar->OpId = OpXSend;

	if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
	{
		char FileDirExpanded[MAX_PATH];
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
		Option = MAKELONG(ts.XmodemBin,ts.XmodemOpt);
		if (! (*GetXFname)(FileVar->HMainWin,
		                   mode==IdXReceive,&Option,FileVar,FileDirExpanded))
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
		(*SetFileVar)(FileVar);

	if (mode==IdXReceive)
		FileVar->FileHandle = _lcreat(FileVar->FullName,0);
	else
		FileVar->FileHandle = _lopen(FileVar->FullName,OF_READ);

	FileVar->FileOpen = FileVar->FileHandle != INVALID_HANDLE_VALUE;
	if (! FileVar->FileOpen)
	{
		ProtoEnd();
		return;
	}
	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_XM,mode,
	                   ts.XmodemOpt,ts.XmodemBin))
		ProtoEnd();
}

void YMODEMStart(int mode)
{
	WORD Opt;

	if (! ProtoStart())
		return;

	if (mode==IdYSend)
	{
		char FileDirExpanded[MAX_PATH];
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));

		// �t�@�C���]�����̃I�v�V������"Yopt1K"�Ɍ��ߑł��B
		// TODO: "Yopt1K", "YoptG", "YoptSingle"����ʂ������Ȃ�΁AIDD_FOPT���g������K�v����B
		Opt = Yopt1K;
		FileVar->OpId = OpYSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			if (! (*GetMultiFname)(FileVar,FileDirExpanded,GMF_Y,&Opt) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
			//ts.XmodemBin = Opt;
		}
		else
		(*SetFileVar)(FileVar);
	}
	else {
		FileVar->OpId = OpYRcv;
		// �t�@�C���]�����̃I�v�V������"Yopt1K"�Ɍ��ߑł��B
		Opt = Yopt1K;
		(*SetFileVar)(FileVar);
	}

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_YM,mode,Opt,0))
		ProtoEnd();
}

void ZMODEMStart(int mode)
{
	WORD Opt;

	if (! ProtoStart())
		return;

	if (mode == IdZSend || mode == IdZAutoS)
	{
		Opt = ts.XmodemBin;
		FileVar->OpId = OpZSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			if (! (*GetMultiFname)(FileVar,FileDirExpanded,GMF_Z,&Opt) ||
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
		(*SetFileVar)(FileVar);
	}
	else /* IdZReceive or IdZAutoR */
		FileVar->OpId = OpZRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_ZM,mode,Opt,0))
		ProtoEnd();
}

void BPStart(int mode)
{
	LONG Option = 0;

	if (! ProtoStart())
		return;
	if (mode==IdBPSend)
	{
		FileVar->OpId = OpBPSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			FileVar->FullName[0] = 0;
			if (! (*GetTransFname)(FileVar, FileDirExpanded, GTF_BP, &Option))
			{
				ProtoEnd();
				return;
			}
		}
		else
			(*SetFileVar)(FileVar);
	}
	else /* IdBPReceive or IdBPAuto */
		FileVar->OpId = OpBPRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_BP,mode,0,0))
		ProtoEnd();
}

void QVStart(int mode)
{
	WORD W;

	if (! ProtoStart())
		return;

	if (mode==IdQVSend)
	{
		FileVar->OpId = OpQVSend;
		if (strlen(&(FileVar->FullName[FileVar->DirLen]))==0)
		{
			char FileDirExpanded[MAX_PATH];
			ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
			if (! (*GetMultiFname)(FileVar,FileDirExpanded,GMF_QV, &W) ||
			    (FileVar->NumFname==0))
			{
				ProtoEnd();
				return;
			}
		}
		else
			(*SetFileVar)(FileVar);
	}
	else
		FileVar->OpId = OpQVRcv;

	TalkStatus = IdTalkQuiet;

	/* disable transmit delay (serial port) */
	cv.DelayFlag = FALSE;

	if (! OpenProtoDlg(FileVar,PROTO_QV,mode,0,0))
		ProtoEnd();
}

/**
 *	���O���[�e�[�g�̐ݒ�
 *	���O�̃T�C�Y��<size>�o�C�g�𒴂��Ă���΁A���[�e�[�V��������悤�ݒ肷��
 */
void LogRotateSize(size_t size)
{
	if (LogVar == NULL) {
		return;
	}
	LogVar->RotateMode = ROTATE_SIZE;
	LogVar->RotateSize = size;
}

/**
 *	���O���[�e�[�g�̐ݒ�
 *	���O�t�@�C���̐����ݒ肷��
 */
void LogRotateRotate(int step)
{
	if (LogVar == NULL) {
		return;
	}
	LogVar->RotateStep = step;
}

/**
 *	���O���[�e�[�g�̐ݒ�
 *	���[�e�[�V�������~
 */
void LogRotateHalt(void)
{
	if (LogVar == NULL) {
		return;
	}
	LogVar->RotateMode = ROTATE_NONE;
	LogVar->RotateSize = 0;
	LogVar->RotateStep = 0;
}

static INT_PTR CALLBACK OnCommentDlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_COMMENT_TITLE" },
		{ IDOK, "BTN_OK" }
	};
	char buf[256];
	UINT ret;

	switch (msg) {
		case WM_INITDIALOG:
			//SetDlgItemText(hDlgWnd, IDC_EDIT_COMMENT, "�T���v��");
			// �G�f�B�b�g�R���g���[���Ƀt�H�[�J�X�����Ă�
			SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_COMMENT));
			SetDlgTexts(hDlgWnd, TextInfos, _countof(TextInfos), ts.UILanguageFile);
			return FALSE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					memset(buf, 0, sizeof(buf));
					ret = GetDlgItemTextA(hDlgWnd, IDC_EDIT_COMMENT, buf, sizeof(buf) - 1);
					if (ret > 0) { // �e�L�X�g�擾����
						//buf[sizeof(buf) - 1] = '\0';  // null-terminate
						CommentLogToFile(buf, ret);
					}
					TTEndDialog(hDlgWnd, IDOK);
					break;
				default:
					return FALSE;
			}
			break;
		case WM_CLOSE:
			TTEndDialog(hDlgWnd, 0);
			return TRUE;

		default:
			return FALSE;
	}
	return TRUE;
}

void LogAddCommentDlg(HINSTANCE hInst, HWND hWnd)
{
	// ���O�t�@�C���փR�����g��ǉ����� (2004.8.6 yutaka)
	TTDialogBox(hInst, MAKEINTRESOURCE(IDD_COMMENT_DIALOG),
				HVTWin, OnCommentDlgProc);
}

void LogClose()
{
	if (LogVar != NULL)
		FileTransEnd(OpLog);
}

BOOL LogOpen(const char *fname)
{
	BOOL ret;

	if ((LogVar==NULL) && !NewFileVar(&LogVar)) {
		return FALSE;
	}

	LogVar->DirLen = 0;
	LogVar->NoMsg = TRUE;
	strncpy_s(LogVar->FullName, sizeof(LogVar->FullName), fname, _TRUNCATE);
	ret = LogStart();
	return ret;
}

BOOL LogIsOpend()
{
	// LogVar->FileOpen
	return LogVar != NULL;
}

void LogWriteStr(const char *str)
{
	if (LogVar != NULL)
	{
		DWORD wrote;
		size_t len = strlen(str);
		WriteFile(LogVar->FileHandle, str, len, &wrote, NULL);
		LogVar->ByteCount =
			LogVar->ByteCount + len;
		FLogRefreshNum();
	}
}

void LogInfo(char *param_ptr, size_t param_len)
{
	if (LogVar) {
		param_ptr[0] = '0'
			+ (ts.LogBinary != 0)
			+ ((ts.Append != 0) << 1)
			+ ((ts.LogTypePlainText != 0) << 2)
			+ ((ts.LogTimestamp != 0) << 3)
			+ ((ts.LogHideDialog != 0) << 4);
		strncpy_s(param_ptr + 1, param_len - 1, LogVar->FullName, _TRUNCATE);
	}
	else {
		param_ptr[0] = '0' - 1;
		param_ptr[1] = 0;
	}
}

/**
 *	���݂̃��O�t�@�C�������擾
 */
const char *LogGetFilename()
{
	if (LogVar == NULL) {
		return NULL;
	}
	return LogVar->FullName;
}

/**
 *	���O�_�C�A���O���J��
 *	@retval	TRUE	[ok] �������ꂽ
 *	@retval	FALSE	�L�����Z�����ꂽ
 *	@param[in,out]	filename	OK���A�t�@�C�����A�s�v�ɂȂ�����free()���邱��
 */
BOOL LogOpenDialog(char **filename)
{
	LogDlgData_t *data = (LogDlgData_t *)calloc(sizeof(LogDlgData_t), 1);
	data->filename = LogGetLogFilename(NULL);
	INT_PTR ret = TTDialogBoxParam(
		hInst, MAKEINTRESOURCE(IDD_LOGDLG),
		HVTWin, LogFnHook, (LPARAM)data);
	if (ret == IDOK) {
		*filename = data->filename;
	}
	free(data);
	return ret == IDOK ? TRUE : FALSE;
}

/**
 *	���O�t�@�C�������擾
 *	���O�t�@�C�����p�̏C�����s��
 *	- strftime() �Ɠ������t�W�J
 *	- �ݒ肳�ꂽ���O�t�@�C���t�H���_��ǉ�
 *	- �z�X�g��,�|�[�g�ԍ��W�J
 *
 *	@param[in]	log_filename	�t�@�C����(����/��΂ǂ���ł�ok)
 *								NULL�̏ꍇ�f�t�H���g�t�@�C�����ƂȂ�
 *								strftime�`��ok
 *	@return						�t���p�X�t�@�C����
 *								�s�v�ɂȂ����� free() ���邱��
 */
char *LogGetLogFilename(const char *log_filename)
{
	// �t�H���_
	char FileDirExpanded[MAX_PATH];
	char *logdir;
	if (strlen(ts.LogDefaultPath) > 0) {
		logdir = ts.LogDefaultPath;
	}
	else if (strlen(ts.FileDir) > 0) {
		ExpandEnvironmentStrings(ts.FileDir, FileDirExpanded, sizeof(FileDirExpanded));
		logdir = FileDirExpanded;
	}
	else {
		logdir = ts.HomeDir;
	}

	// ���ƂȂ�t�@�C����
	char base_name[MAX_PATH];
	if (log_filename == NULL) {
		strncpy_s(base_name, _countof(base_name), ts.LogDefaultName, _TRUNCATE);
	}
	else {
		strncpy_s(base_name, _countof(base_name), log_filename, _TRUNCATE);
	}

	// �t���p�X��
	char full_path[MAX_PATH];
	ConvFName(logdir, base_name, sizeof(base_name), "", full_path, sizeof(full_path));
	ParseStrftimeFileName(full_path, sizeof(full_path));
	ConvertLogname(full_path, sizeof(full_path));

	return _strdup(full_path);
}
