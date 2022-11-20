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

/* TTCMN.DLL, notify icon */

// SDK7.0�̏ꍇ�AWIN32_IE���K�؂ɒ�`����Ȃ�
#if _MSC_VER == 1400	// VS2005�̏ꍇ�̂�
#if !defined(_WIN32_IE)
#define	_WIN32_IE 0x0501
//#define _WIN32_IE 0x0600
#endif
#endif

#include <string.h>
#include <windows.h>
#include <wchar.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "codeconv.h"
#include "compat_win.h"
#include "dlglib.h"

#define	TTCMN_NOTIFY_INTERNAL 1
#include "ttcmn_notify.h"

typedef struct {
	BOOL enable;					// FALSE �ʒmAPI���g���Ȃ�OS or �g���Ȃ����
	int NotifyIconShowCount;
	HWND parent_wnd;
	BOOL created;
	UINT callback_msg;
	BOOL no_sound;
	HINSTANCE IconInstance;
	WORD IconID;
	HINSTANCE CustomIconInstance;
	WORD CustomIconID;
} NotifyIcon;

/**
 *	Shell_NotifyIconW() wrapper
 *		- TT_NOTIFYICONDATAW_V2 �� Windows 2000 �ȍ~�Ŏg�p�\
 *		- �^�X�N�g���C�Ƀo���[�����o�����Ƃ��ł���̂� 2000 �ȍ~
 *			- NT4�͎g���Ȃ�
 *			- HasBalloonTipSupport() �� 2000�ȏ�Ƃ��Ă���
 *		- �K�� Unicode �ł����p�\ �� 9x�T�|�[�g�s�v?
 *			- Windows 98,ME �ŗ��p�\?
 *			- TT_NOTIFYICONDATAA_V2
 */
static BOOL Shell_NotifyIconW(DWORD dwMessage, TT_NOTIFYICONDATAW_V2 *lpData)
{
	BOOL r = Shell_NotifyIconW(dwMessage, (NOTIFYICONDATAW*)lpData);
	assert(r != FALSE);
	return r;
}

static HICON LoadIcon(NotifyIcon *ni)
{
	HINSTANCE IconInstance = ni->CustomIconInstance == NULL ? ni->IconInstance : ni->CustomIconInstance;
	WORD IconID = ni->CustomIconID == 0 ? ni->IconID : ni->CustomIconID;
	const int primary_monitor_dpi = GetMonitorDpiFromWindow(NULL);
	return TTLoadIcon(IconInstance, MAKEINTRESOURCEW(IconID), 16 ,16, primary_monitor_dpi, TRUE);
}

static void CreateNotifyIconData(NotifyIcon *ni, TT_NOTIFYICONDATAW_V2 *p)
{
	assert(ni->parent_wnd != NULL);
	memset(p, 0, sizeof(*p));
	p->cbSize = sizeof(*p);
	p->hWnd = ni->parent_wnd;
	p->uID = 1;
	p->uCallbackMessage = ni->callback_msg;
	p->hIcon = LoadIcon(ni);

	p->uFlags =
		((p->uCallbackMessage != 0) ? NIF_MESSAGE : 0) |
		((p->hIcon != NULL) ? NIF_ICON : 0);
}

static void NotifyHide(NotifyIcon *ni)
{
	if (ni->NotifyIconShowCount > 1) {
		ni->NotifyIconShowCount -= 1;
	}
	else {
		TT_NOTIFYICONDATAW_V2 notify_icon;
		CreateNotifyIconData(ni, &notify_icon);
		notify_icon.uFlags |= NIF_STATE;
		notify_icon.dwState = NIS_HIDDEN;
		notify_icon.dwStateMask = NIS_HIDDEN;
		Shell_NotifyIconW(NIM_MODIFY, &notify_icon);
		ni->NotifyIconShowCount = 0;
		DestroyIcon(notify_icon.hIcon);
	}
}

/**
 *
 *	@param	flag	NOTIFYICONDATA.dwInfoFlags
 *					1		information icon (NIIF_INFO)
 *					2		warning icon (NIIF_WARNING)
 *					3		error icon (NIIF_ERROR)
 *					0x10	(NIIF_NOSOUND) XP+
 */
static void NotifySetMessageW(NotifyIcon *ni, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	if (!ni->enable || msg == NULL) {
		return;
	}
	if (ni->parent_wnd == NULL) {
		// �܂��E�B���h�E���Z�b�g���Ă��Ȃ�
		return;
	}
	if (title == NULL) {
		flag = NIIF_NONE;
	}

	if (ni->no_sound) {
		// shell32.dll 6.00(XP)+
		flag |= NIIF_NOSOUND;
	}

	TT_NOTIFYICONDATAW_V2 notify_icon;
	CreateNotifyIconData(ni, &notify_icon);
	notify_icon.uFlags |= NIF_INFO;
	notify_icon.uFlags |= NIF_STATE;
	notify_icon.dwState = 0;
	notify_icon.dwStateMask = NIS_HIDDEN;

	if (title) {
		wcsncpy_s(notify_icon.szInfoTitle, _countof(notify_icon.szInfoTitle), title, _TRUNCATE);
	}
	else {
		notify_icon.szInfoTitle[0] = 0;
	}
	notify_icon.dwInfoFlags = flag;

	wcsncpy_s(notify_icon.szInfo, _countof(notify_icon.szInfo), msg, _TRUNCATE);

	if (!ni->created) {
		if (Shell_NotifyIconW(NIM_ADD, &notify_icon) != FALSE) {
			ni->created = TRUE;
		}
	}
	else {
		Shell_NotifyIconW(NIM_MODIFY, &notify_icon);
	}

	// �ʒm�A�C�R���͓n������R�s�[�����̂ŕێ����Ȃ��Ă悢
	// �����ɔj������ok
	// https://docs.microsoft.com/ja-jp/windows/win32/shell/taskbar
	// https://stackoverflow.com/questions/23897103/how-to-properly-update-tray-notification-icon
	DestroyIcon(notify_icon.hIcon);
	ni->NotifyIconShowCount += 1;
}

static void NotifySetIconID(NotifyIcon *ni, HINSTANCE hInstance, WORD IconID)
{
	ni->CustomIconInstance = hInstance;
	ni->CustomIconID = IconID;
}

static NotifyIcon *NotifyInitialize(void)
{
	NotifyIcon *ni = (NotifyIcon *)calloc(sizeof(NotifyIcon) ,1);
	return ni;
}

/**
 *	�e�E�B���h�E�ƒʒm�̈�̃A�C�R�����Z�b�g����
 *	����API���R�[������ƒʒm�̈悪�g����悤�ɂȂ�
 *
 *	@param hWnd		�ʒm�̈�Ɋ֘A�t����E�B���h�E(�����ł͐e�E�B���h�E�ƌĂ�)
 */
static void NotifySetWindow(NotifyIcon *ni, HWND hWnd, UINT msg, HINSTANCE hInstance, WORD IconID)
{
	assert(hWnd != NULL);
	if (! HasBalloonTipSupport()) {
		ni->enable = FALSE;
		return;
	}
	ni->enable = TRUE;
	ni->parent_wnd = hWnd;
	ni->callback_msg = msg;
	ni->no_sound = FALSE;
	ni->IconInstance = hInstance;
	ni->IconID = IconID;
}

/**
 *	�e�E�B���h�E��j������O�ɒʒm�̈�̃A�C�R�����폜����
 *	����API���R�[������ƒʒm�̈�͎g���Ȃ��Ȃ�
 */
static void NotifyUnsetWindow(NotifyIcon *ni)
{
	if (ni->enable && ni->created) {
		TT_NOTIFYICONDATAW_V2 notify_icon;
		CreateNotifyIconData(ni, &notify_icon);
		Shell_NotifyIconW(NIM_DELETE, &notify_icon);
		DestroyIcon(notify_icon.hIcon);
		ni->created = FALSE;
		ni->enable = FALSE;
	}
}

static void NotifyUninitialize(NotifyIcon *ni)
{
	NotifyUnsetWindow(ni);
	free(ni);
}

static void NotifySetSound(NotifyIcon *ni, BOOL sound)
{
	ni->no_sound = sound == FALSE ? TRUE : FALSE;
}

static BOOL NotifyGetSound(NotifyIcon *ni)
{
	return ni->no_sound;
}

static NotifyIcon *GetNotifyData(PComVar cv)
{
	assert(cv != NULL);
	NotifyIcon *p = (NotifyIcon *)cv->NotifyIcon;
	assert(p != NULL);
	return p;
}

/*
 *	EXPORT API
 */
void WINAPI NotifyHideIcon(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifyHide(ni);
}

void WINAPI NotifyMessageW(PComVar cv, const wchar_t *msg, const wchar_t *title, DWORD flag)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifySetMessageW(ni, msg, title, flag);
}

void WINAPI NotifyMessage(PComVar cv, const char *msg, const char *title, DWORD flag)
{
	wchar_t *titleW = ToWcharA(title);
	wchar_t *msgW = ToWcharA(msg);
	NotifyMessageW(cv, msgW, titleW, flag);
	free(titleW);
	free(msgW);
}

DllExport void NotifySetSound(PComVar cv, BOOL sound)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifySetSound(ni, sound);
}

DllExport BOOL NotifyGetSound(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	return NotifyGetSound(ni);
}

/**
 *	�ʒm�̈揉����
 *
 *	@param	hWnd		�ʒm�̈�̐e�E�B���h�E
 *	@param	msg			�e�E�B���h�E�֑����郁�b�Z�[�WID
 *	@param	hInstance	�A�C�R���������W���[����instance
 *	@param	IconID		�A�C�R���̃��\�[�XID
 */
void WINAPI NotifyInitialize(PComVar cv)
{
	assert(cv->NotifyIcon == NULL);	 // 2�d������
	NotifyIcon *ni = NotifyInitialize();
	cv->NotifyIcon = ni;
}

/**
 *	�ʒm�̈抮��
 */
void WINAPI NotifyUninitialize(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifyUninitialize(ni);
	cv->NotifyIcon = NULL;
}

/**
 *	�E�B���h�E�ƃA�C�R�����Z�b�g����
 *
 *	@param	hWnd		�ʒm�̈�̐e�E�B���h�E
 *	@param	msg			�e�E�B���h�E�֑����郁�b�Z�[�WID
 *	@param	hInstance	�A�C�R���������W���[����instance
 *	@param	IconID		�A�C�R���̃��\�[�XID
 *
 *	�����ŃZ�b�g�����A�C�R�����f�t�H���g�̃A�C�R���ƂȂ�
 */
void WINAPI NotifySetWindow(PComVar cv, HWND hWnd, UINT msg, HINSTANCE hInstance, WORD IconID)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifySetWindow(ni, hWnd, msg, hInstance, IconID);
}

/**
 *	�ʒm�̈�̐e�E�B���h�E��j������O�ɃR�[������
 *	����API���R�[������ƒʒm�̈�͎g���Ȃ��Ȃ�
 */
void WINAPI NotifyUnsetWindow(PComVar cv)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifyUnsetWindow(ni);
}

/**
 *	�ʒm�̈�Ŏg�p����A�C�R�����Z�b�g����
 *
 *	@param	hInstance	�A�C�R���������W���[����instance
 *	@param	IconID		�A�C�R���̃��\�[�XID
 *
 *	�e�X��NULL, 0 �ɂ���� NotifySetWindow() �Őݒ肵���f�t�H���g�̃A�C�R���ɖ߂�
 */
void WINAPI NotifySetIconID(PComVar cv, HINSTANCE hInstance, WORD IconID)
{
	NotifyIcon *ni = GetNotifyData(cv);
	NotifySetIconID(ni, hInstance, IconID);
}

/**
 * Tera Term�̒ʒm�̈�̎g����
 * - �ʒm����Ƃ��ɁA�A�C�R���ƃo���[����\��
 *   - ���߂Ă̒ʒm���ɁA�ʒm�A�C�R���쐬
 * - �ʒm�^�C���A�E�g
 *   - �ʒm�A�C�R�����B��
 *   - vtwin.cpp �� CVTWindow::OnNotifyIcon() ���Q��
 * - �ʒm���N���b�N
 *   - �ʒm�A�C�R�����B�� + vtwin��Z�I�[�_�[���őO�ʂ�
 *   - vtwin.cpp �� CVTWindow::OnNotifyIcon() ���Q��
 * - �I����
 *   - �A�C�R�����폜
 */
