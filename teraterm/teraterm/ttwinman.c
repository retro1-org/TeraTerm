/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005- TeraTerm Project
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

/* TERATERM.EXE, variables, flags related to VT win and TEK win */

#include "teraterm.h"
#include "tttypes.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "ttlib.h"
#include "helpid.h"
#include "i18n.h"
#include "commlib.h"
#include "codeconv.h"
#include "asprintf.h"

HWND HVTWin = NULL;
HWND HTEKWin = NULL;

int ActiveWin = IdVT; /* IdVT, IdTEK */
IdTalk TalkStatus = IdTalkKeyb; /* IdTalkKeyb, IdTalkCB, IdTalkTextFile */
BOOL KeybEnabled = TRUE; /* keyboard switch */
BOOL Connecting = FALSE;

/* 'help' button on dialog box */
WORD MsgDlgHelp;

TTTSet ts;
TComVar cv;

/* pointers to window objects */
void* pTEKWin = NULL;
/* instance handle */
HINSTANCE hInst;

int SerialNo;

void VTActivate()
{
  ActiveWin = IdVT;
  ShowWindow(HVTWin, SW_SHOWNORMAL);
  SetFocus(HVTWin);
}

// �L���v�V�����̕ύX
//
// (2005.2.19 yutaka) format ID=13�̐V�K�ǉ��ACOM5�ȏ�̕\���ɑΉ�
// (2005.3.13 yutaka) �^�C�g����SJIS�ւ̕ϊ��i���{��j��ǉ�
// (2006.6.15 maya)   ts.KanjiCode��EUC���ƁASJIS�ł�EUC�Ƃ���
//                    �ϊ����Ă��܂��̂ŁA�����ł͕ϊ����Ȃ�
// (2007.7.19 maya)   TCP �|�[�g�ԍ� �� �V���A���|�[�g�̃{�[���[�g�̕\���ɑΉ�
/*
 *  TitleFormat
 *    0 0 0 0 0 0 (2)
 *    | | | | | +----- displays TCP host/serial port
 *    | | | | +------- displays session no
 *    | | | +--------- displays VT/TEK
 *    | | +----------- displays TCP host/serial port first
 *    | +------------- displays TCP port number
 *    +--------------- displays speed of serial port
 */
void ChangeTitle()
{
	wchar_t TempTitle[HostNameMaxLength + TitleBuffSize * 2 + 1]; // �o�b�t�@�g��
	wchar_t TempTitleWithRemote[TitleBuffSize * 2];

	{
		wchar_t *title = ToWcharA(ts.Title);
		wchar_t *title_remote = ToWcharA(cv.TitleRemote);
		if (title_remote == NULL) {
			// TODO cv.TitleRemote �� strncpy_s() �ŕs���ȕ������܂܂ꂽ�Ƃ� NULL ���Ԃ�
			title_remote = _wcsdup(L"-");
		}
		if (Connecting || !cv.Ready || wcslen(title_remote) == 0) {
			wcsncpy_s(TempTitleWithRemote, _countof(TempTitleWithRemote), title, _TRUNCATE);
			wcsncpy_s(TempTitle, _countof(TempTitle), title, _TRUNCATE);
		}
		else {
			// �����[�g����̃^�C�g����ʂɈ��� (2008.11.1 maya)
			if (ts.AcceptTitleChangeRequest == IdTitleChangeRequestOff) {
				wcsncpy_s(TempTitleWithRemote, _countof(TempTitleWithRemote), title, _TRUNCATE);
			}
			else if (ts.AcceptTitleChangeRequest == IdTitleChangeRequestAhead) {
				_snwprintf_s(TempTitleWithRemote, _countof(TempTitleWithRemote), _TRUNCATE,
							 L"%s %s", title_remote, title);
			}
			else if (ts.AcceptTitleChangeRequest == IdTitleChangeRequestLast) {
				_snwprintf_s(TempTitleWithRemote, _countof(TempTitleWithRemote), _TRUNCATE,
							 L"%s %s", title, title_remote);
			}
			else {
				wcsncpy_s(TempTitleWithRemote, _countof(TempTitleWithRemote), title_remote, _TRUNCATE);
			}
			wcsncpy_s(TempTitle, _countof(TempTitle), TempTitleWithRemote, _TRUNCATE);
		}
		free(title);
		free(title_remote);
	}

	if ((ts.TitleFormat & 1)!=0)
	{ // host name
		wchar_t UIMsg[MAX_UIMSG];
		wcsncat_s(TempTitle,_countof(TempTitle), L" - ",_TRUNCATE);
		if (Connecting) {
			get_lang_msgW("DLG_MAIN_TITLE_CONNECTING", UIMsg, _countof(UIMsg), L"[connecting...]", ts.UILanguageFile);
			wcsncat_s(TempTitle,_countof(TempTitle), UIMsg,_TRUNCATE);
		}
		else if (! cv.Ready) {
			get_lang_msgW("DLG_MAIN_TITLE_DISCONNECTED", UIMsg, _countof(UIMsg), L"[disconnected]", ts.UILanguageFile);
			wcsncat_s(TempTitle,_countof(TempTitle), UIMsg,_TRUNCATE);
		}
		else if (cv.PortType==IdSerial)
		{
			// COM5 over�ɑΉ�
			wchar_t str[24]; // COMxxxx:xxxxxxxxxxbps
			if (ts.TitleFormat & 32) {
				_snwprintf_s(str, _countof(str), _TRUNCATE, L"COM%d:%ubps", ts.ComPort, ts.Baud);
			}
			else {
				_snwprintf_s(str, _countof(str), _TRUNCATE, L"COM%d", ts.ComPort);
			}

			if (ts.TitleFormat & 8) {
				_snwprintf_s(TempTitle, _countof(TempTitle), _TRUNCATE, L"%s - %s", str, TempTitleWithRemote);
			} else {
				wcsncat_s(TempTitle, _countof(TempTitle), str, _TRUNCATE);
			}
		}
		else if (cv.PortType == IdNamedPipe)
		{
			wchar_t str[_countof(TempTitle)];
			wchar_t *host_name = ToWcharA(ts.HostName);
			wcsncpy_s(str, _countof(str), host_name, _TRUNCATE);
			free(host_name);

			if (ts.TitleFormat & 8) {
				// format ID = 13(8 + 5): <hots/port> - <title>
				_snwprintf_s(TempTitle, _countof(TempTitle), _TRUNCATE, L"%s - %s", str, TempTitleWithRemote);
			}
			else {
				wcsncat_s(TempTitle, _countof(TempTitle), str, _TRUNCATE);
			}
		}
		else {
			wchar_t str[_countof(TempTitle)];
			wchar_t *host_name = ToWcharA(ts.HostName);
			if (ts.TitleFormat & 16) {
				_snwprintf_s(str, _countof(str), _TRUNCATE, L"%s:%d", host_name, ts.TCPPort);
			}
			else {
				wcsncpy_s(str, _countof(str), host_name, _TRUNCATE);
			}
			free(host_name);

			if (ts.TitleFormat & 8) {
				// format ID = 13(8 + 5): <hots/port> - <title>
				_snwprintf_s(TempTitle, _countof(TempTitle), _TRUNCATE, L"%s - %s", str, TempTitleWithRemote);
			}
			else {
				wcsncat_s(TempTitle, _countof(TempTitle), str, _TRUNCATE);
			}
		}
	}

	if ((ts.TitleFormat & 2)!=0)
	{ // serial no.
		wchar_t Num[11];
		wcsncat_s(TempTitle,_countof(TempTitle),L" (",_TRUNCATE);
		_snwprintf_s(Num,_countof(Num),_TRUNCATE,L"%u",SerialNo);
		wcsncat_s(TempTitle,_countof(TempTitle),Num,_TRUNCATE);
		wcsncat_s(TempTitle,_countof(TempTitle),L")",_TRUNCATE);
	}

	if ((ts.TitleFormat & 4)!=0) // VT
		wcsncat_s(TempTitle,_countof(TempTitle),L" VT",_TRUNCATE);

	SetWindowTextW(HVTWin,TempTitle);

	if (HTEKWin!=0)
	{
		if ((ts.TitleFormat & 4)!=0) // TEK
		{
			wcsncat_s(TempTitle,_countof(TempTitle),L" TEK",_TRUNCATE);
		}
		SetWindowTextW(HTEKWin,TempTitle);
	}
}

void SwitchMenu()
{
  HWND H1, H2;

  if (ActiveWin==IdVT)
  {
    H1 = HTEKWin;
    H2 = HVTWin;
  }
  else {
    H1 = HVTWin;
    H2 = HTEKWin;
  }

  if (H1!=0)
    PostMessage(H1,WM_USER_CHANGEMENU,0,0);
  if (H2!=0)
    PostMessage(H2,WM_USER_CHANGEMENU,0,0);
}

void SwitchTitleBar()
{
  HWND H1, H2;

  if (ActiveWin==IdVT)
  {
    H1 = HTEKWin;
    H2 = HVTWin;
  }
  else {
    H1 = HVTWin;
    H2 = HTEKWin;
  }

  if (H1!=0)
    PostMessage(H1,WM_USER_CHANGETBAR,0,0);
  if (H2!=0)
    PostMessage(H2,WM_USER_CHANGETBAR,0,0);
}

HMODULE LoadHomeDLL(const wchar_t *DLLname)
{
	HMODULE handle;
	wchar_t *DLLpath;
	aswprintf(&DLLpath, L"%s\\%s", ts.HomeDirW, DLLname);
	handle = LoadLibraryW(DLLpath);
	free(DLLpath);
	return handle;
}
