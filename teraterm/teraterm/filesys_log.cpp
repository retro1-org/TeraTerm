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

/* TERATERM.EXE, log routines */
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
#include "ttwinman.h"
#include "commlib.h"
#include "ttcommon.h"
#include "ttlib.h"
#include "dlglib.h"
#include "vtterm.h"
#include "ftlib.h"
#include "buffer.h"
#include "helpid.h"
#include "layer_for_unicode.h"
#include "layer_for_unicode_crt.h"
#include "codeconv.h"

#include "filesys_log_res.h"
#include "filesys.h"

typedef struct {
  char FullName[MAX_PATH];
  int DirLen;

  BOOL FileOpen;
  HANDLE FileHandle;
  LONG FileSize, ByteCount;
  BOOL OverWrite;

  BOOL Success;
  BOOL NoMsg;

  char LogDefaultPath[MAX_PATH];
  BOOL HideDialog;

  DWORD StartTime;

  // log rotate
  int RotateMode;  //  enum rotate_mode RotateMode;
  LONG RotateSize;
  int RotateStep;

  HANDLE LogThread;
  DWORD LogThreadId;
  HANDLE LogThreadEvent;
} TFileVar_;
typedef TFileVar_ *PFileVar_;

#define PFileVar PFileVar_
#define TFileVar TFileVar_

static PFileVar LogVar = NULL;

BOOL FileLog = FALSE;
BOOL BinLog = FALSE;

/*
   Line Head flag for timestamping
   2007.05.24 Gentaro
*/
enum enumLineEnd {
	Line_Other = 0,
	Line_LineHead = 1,
	Line_FileHead = 2,
};

static enum enumLineEnd eLineEnd = Line_LineHead;


// �x���������ݗp�X���b�h�̃��b�Z�[�W
#define WM_DPC_LOGTHREAD_SEND (WM_APP + 1)

static void FileTransEnd_(void);
static void Log1Bin(BYTE b);
static void LogBinSkip(int add);
static BOOL CreateLogBuf(void);
static BOOL CreateBinBuf(void);

static PFileTransDlg FLogDlg = NULL;

static BOOL OpenFTDlg_(PFileVar fv)
{
	PFileTransDlg FTDlg;

	FTDlg = new CFileTransDlg();

	fv->StartTime = 0;
	cv.FilePause &= ~OpLog;

	if (ts.LogHideDialog)
		fv->HideDialog = TRUE;

	if (FTDlg!=NULL)
	{
		char DlgCaption[40];
		strncpy_s(DlgCaption, _countof(DlgCaption),"Tera Term: ", _TRUNCATE);
		char uimsg[MAX_UIMSG];
		get_lang_msg("FILEDLG_TRANS_TITLE_LOG", uimsg, sizeof(uimsg), TitLog, ts.UILanguageFile);
		strncat_s(DlgCaption, _countof(DlgCaption), uimsg, _TRUNCATE);

		CFileTransDlgInfo info;
		info.UILanguageFile = ts.UILanguageFile;
		info.OpId = OpLog;
		info.DlgCaption = DlgCaption;
		info.FileName = &fv->FullName[fv->DirLen];
		info.FullName = fv->FullName;
		info.HideDialog = fv->HideDialog;
		info.HMainWin = HVTWin;
		FTDlg->Create(hInst, &info);
		FTDlg->RefreshNum(fv->StartTime, fv->FileSize, fv->ByteCount);
	}

	FLogDlg = FTDlg; /* Log */

	fv->StartTime = GetTickCount();

	return (FTDlg!=NULL);
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

static void FixLogOption(void)
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

	GetRB(Dialog, &val, IDC_APPEND, IDC_APPEND);
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
static void CheckLogFile(const wchar_t *filename, BOOL *exist, int *bom)
{
	*exist = FALSE;
	*bom = 0;

	// �t�@�C�������݂���?
	DWORD logdir = _GetFileAttributesW(filename);
	if ((logdir != INVALID_FILE_ATTRIBUTES) && ((logdir & FILE_ATTRIBUTE_DIRECTORY) == 0)) {
		// �t�@�C����������
		*exist = TRUE;

		// BOM�L��/�����`�F�b�N
		FILE *fp = __wfopen(filename, L"rb");
		if (fp != NULL) {
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
	}
}

typedef struct {
	FLogDlgInfo_t *info;
	// work
	BOOL file_exist;
	int current_bom;
	TTTSet *pts;
} LogDlgWork_t;

static void ArrangeControls(HWND Dialog, LogDlgWork_t *work)
{
	if (work->file_exist) {
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), TRUE);
		if (work->pts->Append > 0) {
			CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_APPEND);
		}
	}
	else {
		// �t�@�C�����Ȃ� -> �V�K
		EnableWindow(GetDlgItem(Dialog, IDC_APPEND), FALSE);
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_NEW_OVERWRITE);
	}

	if (work->file_exist && IsDlgButtonChecked(Dialog, IDC_APPEND) == BST_CHECKED) {
		// �t�@�C�������݂��� && append
		int bom = work->current_bom;
		if (bom != 0) {
			// BOM�L��
			CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
			int cur =
				bom == 1 ? 0 :
				bom == 2 ? 1 :
				bom == 3 ? 2 : 0;
			SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, cur, 0);
		}
		else {
			// BOM�Ȃ�
			CheckDlgButton(Dialog, IDC_BOM, BST_UNCHECKED);
			SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);
		}
		if (IsDlgButtonChecked(Dialog, IDC_FOPTTEXT) == BST_CHECKED) {
			EnableWindow(GetDlgItem(Dialog, IDC_BOM), FALSE);
			if (bom != 0) {
				// BOM�L��
				EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), FALSE);
			}
			else {
				// BOM�Ȃ�
				EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), TRUE);
			}
		}
	}
	else {
		// �t�@�C�����Ȃ� ���� append�ł͂Ȃ�(�㏑��)
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_NEW_OVERWRITE);
		CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);
		if (IsDlgButtonChecked(Dialog, IDC_FOPTTEXT) == BST_CHECKED) {
			EnableWindow(GetDlgItem(Dialog, IDC_BOM), TRUE);
		}
	}
}

static void CheckLogFile(HWND Dialog, const wchar_t *filename, LogDlgWork_t *work)
{
	BOOL exist;
	int bom;
	CheckLogFile(filename, &exist, &bom);
	work->file_exist = exist;
	work->current_bom = bom;
	ArrangeControls(Dialog, work);
}

static INT_PTR CALLBACK LogFnHook(HWND Dialog, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static const DlgTextInfo TextInfos[] = {
		{ 0, "DLG_TABSHEET_TITLE_LOG" },
		{ IDC_FOPTBIN, "DLG_FOPT_BINARY" },
		{ IDC_APPEND, "DLG_FOPT_APPEND" },
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
	LogDlgWork_t *work = (LogDlgWork_t *)GetWindowLongPtr(Dialog, DWLP_USER);

	if (Message == 	RegisterWindowMessage(HELPMSGSTRING)) {
		// �R�����_�C�A���O����̃w���v���b�Z�[�W��t���ւ���
		Message = WM_COMMAND;
		wParam = IDHELP;
	}
	switch (Message) {
	case WM_INITDIALOG: {
		work = (LogDlgWork_t *)lParam;
		TTTSet *pts = work->pts;
		const char *UILanguageFile = pts->UILanguageFile;
		SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)work);
		::DragAcceptFiles(Dialog, TRUE);

		SetDlgTexts(Dialog, TextInfos, _countof(TextInfos), UILanguageFile);
		SetI18nList("Tera Term", Dialog, IDC_TIMESTAMPTYPE, timestamp_list, _countof(timestamp_list),
					UILanguageFile, 0);

		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-8");
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-16LE");
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_ADDSTRING, 0, (LPARAM)"UTF-16BE");
		SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);

		_SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, work->info->filename);
		work->info->filename = NULL;

		// Binary/Text �`�F�b�N�{�b�N�X
		if (pts->LogBinary) {
			CheckRadioButton(Dialog, IDC_FOPTBIN, IDC_FOPTTEXT, IDC_FOPTBIN);
		}
		else {
			CheckRadioButton(Dialog, IDC_FOPTBIN, IDC_FOPTTEXT, IDC_FOPTTEXT);
		}

		// Plain Text �`�F�b�N�{�b�N�X
		if (pts->LogBinary) {
			// Binary�t���O���L���ȂƂ��̓`�F�b�N�ł��Ȃ�
			DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_PLAINTEXT);
		}
		else if (pts->LogTypePlainText) {
			SetRB(Dialog, 1, IDC_PLAINTEXT, IDC_PLAINTEXT);
		}

		// Hide dialog�`�F�b�N�{�b�N�X (2008.1.30 maya)
		if (pts->LogHideDialog) {
			SetRB(Dialog, 1, IDC_HIDEDIALOG, IDC_HIDEDIALOG);
		}

		// Include screen buffer�`�F�b�N�{�b�N�X (2013.9.29 yutaka)
		if (pts->LogAllBuffIncludedInFirst) {
			SetRB(Dialog, 1, IDC_ALLBUFF_INFIRST, IDC_ALLBUFF_INFIRST);
		}

		// timestamp�`�F�b�N�{�b�N�X (2006.7.23 maya)
		if (pts->LogBinary) {
			// Binary�t���O���L���ȂƂ��̓`�F�b�N�ł��Ȃ�
			DisableDlgItem(Dialog, IDC_TIMESTAMP, IDC_TIMESTAMP);
		}
		else if (pts->LogTimestamp) {
			SetRB(Dialog, 1, IDC_TIMESTAMP, IDC_TIMESTAMP);
		}

		// timestamp ���
		int tstype = pts->LogTimestampType == TIMESTAMP_LOCAL ? 0 :
				pts->LogTimestampType == TIMESTAMP_UTC ? 1 :
				pts->LogTimestampType == TIMESTAMP_ELAPSED_LOGSTART ? 2 :
				pts->LogTimestampType == TIMESTAMP_ELAPSED_CONNECTED ? 3 : 0;
		SendDlgItemMessage(Dialog, IDC_TIMESTAMPTYPE, CB_SETCURSEL, tstype, 0);
		if (pts->LogBinary || !pts->LogTimestamp) {
			DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
		}

		CenterWindow(Dialog, GetParent(Dialog));

		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK: {
			wchar_t filename[MAX_PATH];
			_GetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename, _countof(filename));
			work->info->filename = _wcsdup(filename);
			work->info->append = IsDlgButtonChecked(Dialog, IDC_APPEND) == BST_CHECKED;
			work->info->bom = IsDlgButtonChecked(Dialog, IDC_BOM) == BST_CHECKED;
			work->info->code = (int)SendDlgItemMessageA(Dialog, IDC_TEXTCODING_DROPDOWN, CB_GETCURSEL, 0, 0);
			SetLogFlags(Dialog);
			EndDialog(Dialog, IDOK);
			break;
		}
		case IDCANCEL:
			EndDialog(Dialog, IDCANCEL);
			break;
		case IDHELP:
			OpenHelp(HH_HELP_CONTEXT, HlpFileLog, work->pts->UILanguageFile);
			break;
		case IDC_FOPT_FILENAME_BUTTON: {
			/* save current dir */
			const char *UILanguageFile = work->pts->UILanguageFile;
			wchar_t curdir[MAXPATHLEN];
			_GetCurrentDirectoryW(_countof(curdir), curdir);

			wchar_t fname[MAX_PATH];
			GetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname, _countof(fname));

			wchar_t FNFilter[128*3];
			get_lang_msgW("FILEDLG_ALL_FILTER", FNFilter, sizeof(FNFilter), L"All(*.*)\\0*.*\\0\\0", UILanguageFile);

			wchar_t caption[MAX_PATH];
			wchar_t uimsg[MAX_UIMSG];
#define TitLogW      L"Log"
			get_lang_msgW("FILEDLG_TRANS_TITLE_LOG", uimsg, _countof(uimsg), TitLogW, UILanguageFile);
			wcsncpy_s(caption, _countof(caption), L"Tera Term: ", _TRUNCATE);
			wcsncat_s(caption, _countof(caption), uimsg, _TRUNCATE);

			OPENFILENAMEW ofn = {};
			ofn.lStructSize = get_OPENFILENAME_SIZEW();
			//ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			ofn.Flags |= OFN_EXPLORER | OFN_ENABLESIZING;
			ofn.Flags |= OFN_SHOWHELP;
			ofn.Flags |= OFN_NOCHANGEDIR;		// ���܂����삵�Ȃ���������悤��
			ofn.hwndOwner = Dialog;
			ofn.lpstrFilter = FNFilter;
			ofn.nFilterIndex = 1;
			ofn.lpstrFile = fname;
			ofn.nMaxFile = _countof(fname);
			ofn.lpstrTitle = caption;
			BOOL Ok = GetSaveFileNameW(&ofn);
			if (Ok) {
				SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, fname);
			}

			/* restore dir */
			_SetCurrentDirectoryW(curdir);

			break;
		}
		case IDC_FOPTBIN:
			EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), FALSE);
			EnableWindow(GetDlgItem(Dialog, IDC_BOM), FALSE);
			DisableDlgItem(Dialog, IDC_PLAINTEXT, IDC_TIMESTAMP);
			DisableDlgItem(Dialog, IDC_TIMESTAMPTYPE, IDC_TIMESTAMPTYPE);
			break;
		case IDC_FOPTTEXT:
			ArrangeControls(Dialog, work);
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
				wchar_t filename[MAX_PATH];
				GetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename, _countof(filename));
				CheckLogFile(Dialog, filename, work);
			}
			break;
		case IDC_NEW_OVERWRITE:
			if (IsDlgButtonChecked(Dialog, IDC_FOPTTEXT) == BST_CHECKED) {
				EnableWindow(GetDlgItem(Dialog, IDC_BOM), TRUE);
				EnableWindow(GetDlgItem(Dialog, IDC_TEXTCODING_DROPDOWN), TRUE);
				CheckDlgButton(Dialog, IDC_BOM, BST_CHECKED);
				SendDlgItemMessage(Dialog, IDC_TEXTCODING_DROPDOWN, CB_SETCURSEL, 0, 0);
			}
			break;
		case IDC_APPEND:
			ArrangeControls(Dialog, work);
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
		CheckRadioButton(Dialog, IDC_NEW_OVERWRITE, IDC_APPEND, IDC_APPEND);
		_SetDlgItemTextW(Dialog, IDC_FOPT_FILENAME_EDIT, filename);
		SendDlgItemMessage(Dialog, IDC_FOPT_FILENAME_EDIT, EM_SETSEL, len, len);
		free(filename);
		DragFinish(hDrop);
		return TRUE;
	}
	}
	return FALSE;
}

static BOOL LogStart(const char *fname)
{
	unsigned tid;

	strncpy_s(LogVar->FullName, sizeof(LogVar->FullName), fname, _TRUNCATE);

	if (! LoadTTFILE()) return FALSE;

	//(*SetFileVar)(LogVar);
	{
		int i;
		char c;
		PFileVar fv = LogVar;
		GetFileNamePos(fv->FullName,&(fv->DirLen),&i);
		c = fv->FullName[fv->DirLen];
		if (c=='\\'||c=='/') fv->DirLen++;
	}
	FixLogOption();

	if (ts.LogBinary > 0)
	{
		BinLog = TRUE;
		FileLog = FALSE;
		if (! CreateBinBuf())
		{
			FileTransEnd_();
			return FALSE;
		}
	}
	else {
		BinLog = FALSE;
		FileLog = TRUE;
		if (! CreateLogBuf())
		{
			FileTransEnd_();
			return FALSE;
		}
	}
	cv.LStart = cv.LogPtr;
	cv.LCount = 0;

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

		FileTransEnd_();
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

	if (! OpenFTDlg_(LogVar)) {
		FileTransEnd_();
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

	if (FileLog) {
		cv.Log1Byte = LogPut1;
	}
	if (BinLog) {
		cv.Log1Bin = Log1Bin;
		cv.LogBinSkip = LogBinSkip;
	}

	return TRUE;
}

/**
 * ���O��1byte��������
 *		�o�b�t�@�֏������܂��
 *		���ۂ̏������݂� LogToFile() �ōs����
 */
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

/**
 * �o�b�t�@���̃��O���t�@�C���֏�������
 */
static void LogToFile(void)
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
			if (!FLogIsPause() && (! cv.ProtoFlag))
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
			if (!FLogIsPause() && (! cv.ProtoFlag))
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
	if (FLogIsPause() || cv.ProtoFlag) return;
	if (FLogDlg!=NULL)
		//FLogDlg->RefreshNum(LogVar);
		FLogDlg->RefreshNum(LogVar->StartTime, LogVar->FileSize, LogVar->ByteCount);


	// ���O�E���[�e�[�g
	LogRotate();

}

static BOOL CreateLogBuf(void)
{
	if (cv.HLogBuf==NULL)
	{
		cv.HLogBuf = GlobalAlloc(GMEM_MOVEABLE,InBuffSize);
		cv.LogBuf = NULL;
		cv.LogPtr = 0;
		cv.LStart = 0;
		cv.LCount = 0;
	}
	return (cv.HLogBuf!=NULL);
}

static void FreeLogBuf(void)
{
	if ((cv.HLogBuf==NULL) || FileLog)
		return;
	if (cv.LogBuf!=NULL)
		GlobalUnlock(cv.HLogBuf);
	GlobalFree(cv.HLogBuf);
	cv.HLogBuf = NULL;
	cv.LogBuf = NULL;
	cv.LogPtr = 0;
	cv.LStart = 0;
	cv.LCount = 0;
}

static BOOL CreateBinBuf(void)
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

static void FreeBinBuf(void)
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

static void FileTransEnd_(void)
{
	FileLog = FALSE;
	BinLog = FALSE;
	cv.Log1Byte = NULL;
	cv.Log1Bin = NULL;
	cv.LogBinSkip = NULL;
	if (FLogDlg!=NULL)
	{
		FLogDlg->DestroyWindow();
		FLogDlg = NULL;
	}
	if (LogVar != NULL)
	{
		CloseFileSync(LogVar);
		free(LogVar);
		LogVar = NULL;
	}
	FreeLogBuf();
	FreeBinBuf();
	FreeTTFILE();
}


/**
 *	���O���|�[�Y����
 *	���� FLogChangeButton() ������
 */
void FLogPause(BOOL Pause)
{
	if (FLogDlg!=NULL)
		FLogDlg->ChangeButton(Pause);
	FileTransPause(OpLog, Pause);
}

/**
 *	���O���[�e�[�g�̐ݒ�
 *	���O�̃T�C�Y��<size>�o�C�g�𒴂��Ă���΁A���[�e�[�V��������悤�ݒ肷��
 */
void FLogRotateSize(size_t size)
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
void FLogRotateRotate(int step)
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
void FLogRotateHalt(void)
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

void FLogAddCommentDlg(HINSTANCE hInst, HWND hWnd)
{
	// ���O�t�@�C���փR�����g��ǉ����� (2004.8.6 yutaka)
	TTDialogBox(hInst, MAKEINTRESOURCE(IDD_COMMENT_DIALOG),
				HVTWin, OnCommentDlgProc);
}

void FLogClose(void)
{
	if (LogVar == NULL) {
		return;
	}

	FileTransEnd_();
}

/**
 *	���O���I�[�v������
 *	@param[in]	fname	���O�t�@�C����, CreateFile()�ɓn�����
 *
 *	���O�t�@�C������strftime�̓W�J�Ȃǂ͍s���Ȃ��B
 *	FLogGetLogFilename() �� FLogOpenDialog() ��
 *	�t�@�C�������擾�ł���B
 */
BOOL FLogOpen(const char *fname)
{
	BOOL ret;

	if (LogVar != NULL) {
		return FALSE;
	}
	if ((FileLog) || (BinLog)) return FALSE;

	//
	PFileVar fv = (PFileVar)malloc(sizeof(TFileVar));
	if (fv == NULL) {
		return FALSE;
	}
	LogVar = fv;
	memset(fv, 0, sizeof(TFileVar));

	fv->FileOpen = FALSE;
	fv->OverWrite = ((ts.FTFlag & FT_RENAME) == 0);
	fv->Success = FALSE;
	fv->NoMsg = FALSE;
	fv->HideDialog = FALSE;

	LogVar->DirLen = 0;
	LogVar->NoMsg = TRUE;

	ret = LogStart(fname);
	return ret;
}

BOOL FLogIsOpend(void)
{
	// LogVar->FileOpen
	return LogVar != NULL;
}

void FLogWriteStr(const char *str)
{
	if (LogVar != NULL)
	{
		DWORD wrote;
		size_t len = strlen(str);
		WriteFile(LogVar->FileHandle, str, len, &wrote, NULL);
		LogVar->ByteCount =
			LogVar->ByteCount + len;
		if (FLogDlg!=NULL)
			//FLogDlg->RefreshNum(LogVar);
			FLogDlg->RefreshNum(LogVar->StartTime, LogVar->FileSize, LogVar->ByteCount);
	}
}

void FLogInfo(char *param_ptr, size_t param_len)
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
const char *FLogGetFilename()
{
	if (LogVar == NULL) {
		return NULL;
	}
	return LogVar->FullName;
}

/**
 *	���O�_�C�A���O���J��
 *	@param[in,out]	info.filename	�t�@�C���������l
 *									OK���A�t�@�C�����A�s�v�ɂȂ�����free()���邱��
 *	@retval	TRUE	[ok] �������ꂽ
 *	@retval	FALSE	�L�����Z�����ꂽ
 */
BOOL FLogOpenDialog(HINSTANCE hInst, HWND hWnd, FLogDlgInfo_t *info)
{
	LogDlgWork_t *work = (LogDlgWork_t *)calloc(sizeof(LogDlgWork_t), 1);
	char *filenameA = ToCharW(info->filename);
	char *srcfnameA = FLogGetLogFilename(filenameA);
	wchar_t *srcfnameW = ToWcharA(srcfnameA);
	free(filenameA);
	free(srcfnameA);
	work->info = info;
	work->info->filename = srcfnameW;
	work->pts = &ts;
	INT_PTR ret = TTDialogBoxParam(
		hInst, MAKEINTRESOURCE(IDD_LOGDLG),
		hWnd, LogFnHook, (LPARAM)work);
	free(srcfnameW);
	free(work);
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
char *FLogGetLogFilename(const char *log_filename)
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

BOOL FLogIsPause()
{
	return ((cv.FilePause & OpLog) !=0);
}

void FLogWindow(int nCmdShow)
{
	if (FLogDlg == NULL)
		return;

	HWND HWndLog = FLogDlg->m_hWnd;
	ShowWindow(HWndLog, nCmdShow);
	if (nCmdShow == SW_RESTORE) {
		// �g���X�^�C�� WS_EX_NOACTIVATE ��Ԃ���������
		SetForegroundWindow(HWndLog);
	}
}

void FLogShowDlg(void)
{
	if (FLogDlg != NULL) {
		FLogDlg->ShowWindow(SW_SHOWNORMAL);
		SetForegroundWindow(FLogDlg->GetSafeHwnd());
	}
}

/**
 * ���O��1byte��������
 *		LogPut1() �ƈႤ?
 */
//void Log1Bin(PComVar cv, BYTE b)
static void Log1Bin(BYTE b)
{
	if (((cv.FilePause & OpLog)!=0) || cv.ProtoFlag) {
		return;
	}
	if (cv.BinSkip > 0) {
		cv.BinSkip--;
		return;
	}
	cv.BinBuf[cv.BinPtr] = b;
	cv.BinPtr++;
	if (cv.BinPtr>=InBuffSize) {
		cv.BinPtr = cv.BinPtr-InBuffSize;
	}
	if (cv.BCount>=InBuffSize) {
		cv.BCount = InBuffSize;
		cv.BStart = cv.BinPtr;
	}
	else {
		cv.BCount++;
	}
}

static void LogBinSkip(int add)
{
	if (cv.HBinBuf!=0 ) {
		cv.BinSkip += add;
	}
}

/**
 *	���O�o�b�t�@�ɗ��܂��Ă���f�[�^�̃o�C�g����Ԃ�
 */
int FLogGetCount(void)
{
	if (FileLog) {
		return cv.LCount;
	}
	if (BinLog) {
		return cv.BCount;
	}
	return 0;
}

/**
 * �o�b�t�@���̃��O���t�@�C���֏�������
 */
void FLogWriteFile(void)
{
	if (cv.LogBuf!=NULL)
	{
		if (FileLog) {
			LogToFile();
		}
		GlobalUnlock(cv.HLogBuf);
		cv.LogBuf = NULL;
	}

	if (cv.BinBuf!=NULL)
	{
		if (BinLog) {
			LogToFile();
		}
		GlobalUnlock(cv.HBinBuf);
		cv.BinBuf = NULL;
	}
}
