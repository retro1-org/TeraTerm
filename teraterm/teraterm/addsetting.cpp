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

/*
 * Additional settings dialog
 */

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <time.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "ttwinman.h"	// for ts
#include "ttcommon.h"
#include "dlglib.h"
#include "compat_win.h"
#include "helpid.h"
#include "addsetting.h"
#include "debug_pp.h"
#include "tipwin.h"
#include "i18n.h"
#include "codeconv.h"
#include "coding_pp.h"
#include "font_pp.h"
#include "asprintf.h"
#include "win32helper.h"
#include "themedlg.h"

const mouse_cursor_t MouseCursor[] = {
	{"ARROW", IDC_ARROW},
	{"IBEAM", IDC_IBEAM},
	{"CROSS", IDC_CROSS},
	{"HAND", IDC_HAND},
	{NULL, NULL},
};
#define MOUSE_CURSOR_MAX (sizeof(MouseCursor)/sizeof(MouseCursor[0]) - 1)

void CVisualPropPageDlg::SetupRGBbox(int index)
{
	COLORREF Color = ts.ANSIColor[index];
	BYTE c;

	c = GetRValue(Color);
	SetDlgItemNum(IDC_COLOR_RED, c);

	c = GetGValue(Color);
	SetDlgItemNum(IDC_COLOR_GREEN, c);

	c = GetBValue(Color);
	SetDlgItemNum(IDC_COLOR_BLUE, c);
}

// CGeneralPropPageDlg �_�C�A���O

CGeneralPropPageDlg::CGeneralPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CGeneralPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_GENERAL",
				 L"General", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CGeneralPropPageDlg::~CGeneralPropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CGeneralPropPageDlg ���b�Z�[�W �n���h��

void CGeneralPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_CLICKABLE_URL, "DLG_TAB_GENERAL_CLICKURL" },
		{ IDC_DISABLE_SENDBREAK, "DLG_TAB_GENERAL_DISABLESENDBREAK" },
		{ IDC_ACCEPT_BROADCAST, "DLG_TAB_GENERAL_ACCEPTBROADCAST" },
		{ IDC_MOUSEWHEEL_SCROLL_LINE, "DLG_TAB_GENERAL_MOUSEWHEEL_SCROLL_LINE" },
		{ IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, "DLG_TAB_GENERAL_AUTOSCROLL_ONLY_IN_BOTTOM_LINE" },
		{ IDC_CLEAR_ON_RESIZE, "DLG_TAB_GENERAL_CLEAR_ON_RESIZE" },
		{ IDC_CURSOR_CHANGE_IME, "DLG_TAB_GENERAL_CURSOR_CHANGE_IME" },
		{ IDC_LIST_HIDDEN_FONTS, "DLG_TAB_GENERAL_LIST_HIDDEN_FONTS" },
		{ IDC_TITLEFMT_GROUP, "DLG_TAB_GENERAL_TITLEFMT_GROUP" },
		{ IDC_TITLEFMT_DISPHOSTNAME, "DLG_TAB_GENERAL_TITLEFMT_DISPHOSTNAME" },
		{ IDC_TITLEFMT_DISPSESSION, "DLG_TAB_GENERAL_TITLEFMT_DISPSESSION" },
		{ IDC_TITLEFMT_DISPVTTEK, "DLG_TAB_GENERAL_TITLEFMT_DISPVTTEK" },
		{ IDC_TITLEFMT_SWAPHOSTTITLE, "DLG_TAB_GENERAL_TITLEFMT_SWAPHOSTTITLE" },
		{ IDC_TITLEFMT_DISPTCPPORT, "DLG_TAB_GENERAL_TITLEFMT_DISPTCPPORT" },
		{ IDC_TITLEFMT_DISPSERIALSPEED, "DLG_TAB_GENERAL_TITLEFMT_DISPSERIALSPEED" }
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	// (1)DisableAcceleratorSendBreak
	SetCheck(IDC_DISABLE_SENDBREAK, ts.DisableAcceleratorSendBreak);

	// (2)EnableClickableUrl
	SetCheck(IDC_CLICKABLE_URL, ts.EnableClickableUrl);

	// (3)AcceptBroadcast 337: 2007/03/20
	SetCheck(IDC_ACCEPT_BROADCAST, ts.AcceptBroadcast);

	// (4)IDC_MOUSEWHEEL_SCROLL_LINE
	SetDlgItemNum(IDC_SCROLL_LINE, ts.MouseWheelScrollLine);

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	SetCheck(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE, ts.AutoScrollOnlyInBottomLine);

	// (6)IDC_CLEAR_ON_RESIZE
	SetCheck(IDC_CLEAR_ON_RESIZE, (ts.TermFlag & TF_CLEARONRESIZE) != 0);

	// (7)IDC_CURSOR_CHANGE_IME
	SetCheck(IDC_CURSOR_CHANGE_IME, (ts.WindowFlag & WF_IMECURSORCHANGE) != 0);

	// (8)IDC_LIST_HIDDEN_FONTS
	SetCheck(IDC_LIST_HIDDEN_FONTS, ts.ListHiddenFonts);

	// (9) Title Format
	SetCheck(IDC_TITLEFMT_DISPHOSTNAME, (ts.TitleFormat & 1) != 0);
	SetCheck(IDC_TITLEFMT_DISPSESSION, (ts.TitleFormat & (1<<1)) != 0);
	SetCheck(IDC_TITLEFMT_DISPVTTEK, (ts.TitleFormat & (1<<2)) != 0);
	SetCheck(IDC_TITLEFMT_SWAPHOSTTITLE, (ts.TitleFormat & (1<<3)) != 0);
	SetCheck(IDC_TITLEFMT_DISPTCPPORT, (ts.TitleFormat & (1<<4)) != 0);
	SetCheck(IDC_TITLEFMT_DISPSERIALSPEED, (ts.TitleFormat & (1<<5)) != 0);

	// �_�C�A���O�Ƀt�H�[�J�X�𓖂Ă� (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CLICKABLE_URL));
}

void CGeneralPropPageDlg::OnOK()
{
	char buf[64];
	int val;

	// (1)
	ts.DisableAcceleratorSendBreak = GetCheck(IDC_DISABLE_SENDBREAK);

	// (2)
	ts.EnableClickableUrl = GetCheck(IDC_CLICKABLE_URL);

	// (3) 337: 2007/03/20
	ts.AcceptBroadcast = GetCheck(IDC_ACCEPT_BROADCAST);

	// (4)IDC_MOUSEWHEEL_SCROLL_LINE
	GetDlgItemText(IDC_SCROLL_LINE, buf, _countof(buf));
	val = atoi(buf);
	if (val > 0)
		ts.MouseWheelScrollLine = val;

	// (5)IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE
	ts.AutoScrollOnlyInBottomLine = GetCheck(IDC_AUTOSCROLL_ONLY_IN_BOTTOM_LINE);

	// (6)IDC_CLEAR_ON_RESIZE
	if (((ts.TermFlag & TF_CLEARONRESIZE) != 0) != GetCheck(IDC_CLEAR_ON_RESIZE)) {
		ts.TermFlag ^= TF_CLEARONRESIZE;
	}

	// (7)IDC_CURSOR_CHANGE_IME
	if (((ts.WindowFlag & WF_IMECURSORCHANGE) != 0) != GetCheck(IDC_CURSOR_CHANGE_IME)) {
		ts.WindowFlag ^= WF_IMECURSORCHANGE;
	}

	// (8)IDC_LIST_HIDDEN_FONTS
	ts.ListHiddenFonts = GetCheck(IDC_LIST_HIDDEN_FONTS);

	// (9) Title Format
	ts.TitleFormat = GetCheck(IDC_TITLEFMT_DISPHOSTNAME) == BST_CHECKED;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPSESSION) == BST_CHECKED) << 1;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPVTTEK) == BST_CHECKED) << 2;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_SWAPHOSTTITLE) == BST_CHECKED) << 3;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPTCPPORT) == BST_CHECKED) << 4;
	ts.TitleFormat |= (GetCheck(IDC_TITLEFMT_DISPSERIALSPEED) == BST_CHECKED) << 5;
}

void CGeneralPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditional, 0);
}

// CSequencePropPageDlg �_�C�A���O

CSequencePropPageDlg::CSequencePropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CSequencePropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_SEQUENCE",
				 L"Control Sequence", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CSequencePropPageDlg::~CSequencePropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CSequencePropPageDlg ���b�Z�[�W �n���h��

void CSequencePropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_ACCEPT_MOUSE_EVENT_TRACKING, "DLG_TAB_SEQUENCE_ACCEPT_MOUSE_EVENT_TRACKING" },
		{ IDC_DISABLE_MOUSE_TRACKING_CTRL, "DLG_TAB_SEQUENCE_DISABLE_MOUSE_TRACKING_CTRL" },
		{ IDC_ACCEPT_TITLE_CHANGING_LABEL, "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING" },

		{ IDC_CURSOR_CTRL_SEQ, "DLG_TAB_SEQUENCE_CURSOR_CTRL" },
		{ IDC_WINDOW_CTRL, "DLG_TAB_SEQUENCE_WINDOW_CTRL" },
		{ IDC_WINDOW_REPORT, "DLG_TAB_SEQUENCE_WINDOW_REPORT" },
		{ IDC_TITLE_REPORT_LABEL, "DLG_TAB_SEQUENCE_TITLE_REPORT" },

		{ IDC_CLIPBOARD_ACCESS_LABEL, "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS" },

		{ IDC_CLIPBOARD_NOTIFY, "DLG_TAB_SEQUENCE_CLIPBOARD_NOTIFY" },
		{ IDC_ACCEPT_CLEAR_SBUFF, "DLG_TAB_SEQUENCE_ACCEPT_CLEAR_SBUFF" },
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	const static I18nTextInfo accept_title_changing[] = {
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OFF", L"off" },
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_OVERWRITE", L"overwrite" },
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_AHEAD", L"ahead" },
		{ "DLG_TAB_SEQUENCE_ACCEPT_TITLE_CHANGING_LAST", L"last" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_ACCEPT_TITLE_CHANGING, accept_title_changing, _countof(accept_title_changing),
				 ts.UILanguageFileW, 0);

	const static I18nTextInfo sequence_title_report[] = {
		{ "DLG_TAB_SEQUENCE_TITLE_REPORT_IGNORE", L"ignore" },
		{ "DLG_TAB_SEQUENCE_TITLE_REPORT_ACCEPT", L"accept" },
		{ "DLG_TAB_SEQUENCE_TITLE_REPORT_EMPTY", L"empty" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_TITLE_REPORT, sequence_title_report, _countof(sequence_title_report),
				 ts.UILanguageFileW, 0);

	const static I18nTextInfo sequence_clipboard_access[] = {
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_OFF", L"off" },
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_WRITE", L"write only" },
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_READ", L"read only" },
		{ "DLG_TAB_SEQUENCE_CLIPBOARD_ACCESS_ON", L"read/write" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_CLIPBOARD_ACCESS, sequence_clipboard_access,
				 _countof(sequence_clipboard_access), ts.UILanguageFileW, 0);

	// (1)IDC_ACCEPT_MOUSE_EVENT_TRACKING
	SetCheck(IDC_ACCEPT_MOUSE_EVENT_TRACKING, ts.MouseEventTracking);
	EnableDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL, ts.MouseEventTracking ? TRUE : FALSE);

	// (2)IDC_DISABLE_MOUSE_TRACKING_CTRL
	SetCheck(IDC_DISABLE_MOUSE_TRACKING_CTRL, ts.DisableMouseTrackingByCtrl);

	// (3)IDC_ACCEPT_TITLE_CHANGING
	SetCurSel(IDC_ACCEPT_TITLE_CHANGING, ts.AcceptTitleChangeRequest);

	// (4)IDC_TITLE_REPORT
	SetCurSel(IDC_TITLE_REPORT,
			  (ts.WindowFlag & WF_TITLEREPORT) == IdTitleReportIgnore ? 0 :
			  (ts.WindowFlag & WF_TITLEREPORT) == IdTitleReportAccept ? 1
			  /*(ts.WindowFlag & WF_TITLEREPORT) == IdTitleReportEmptye ? */ : 2);

	// (5)IDC_WINDOW_CTRL
	SetCheck(IDC_WINDOW_CTRL, (ts.WindowFlag & WF_WINDOWCHANGE) != 0);

	// (6)IDC_WINDOW_REPORT
	SetCheck(IDC_WINDOW_REPORT, (ts.WindowFlag & WF_WINDOWREPORT) != 0);

	// (7)IDC_CURSOR_CTRL_SEQ
	SetCheck(IDC_CURSOR_CTRL_SEQ, (ts.WindowFlag & WF_CURSORCHANGE) != 0);

	// (8)IDC_CLIPBOARD_ACCESS
	SetCurSel(IDC_CLIPBOARD_ACCESS,
			  (ts.CtrlFlag & CSF_CBRW) == CSF_CBRW ? 3 :
			  (ts.CtrlFlag & CSF_CBRW) == CSF_CBREAD ? 2 :
			  (ts.CtrlFlag & CSF_CBRW) == CSF_CBWRITE ? 1 :
			  0);	// off

	// (9)IDC_CLIPBOARD_NOTIFY
	SetCheck(IDC_CLIPBOARD_NOTIFY, ts.NotifyClipboardAccess);
	EnableDlgItem(IDC_CLIPBOARD_NOTIFY, HasBalloonTipSupport() ? TRUE : FALSE);

	// (10)IDC_ACCEPT_CLEAR_SBUFF
	SetCheck(IDC_ACCEPT_CLEAR_SBUFF, (ts.TermFlag & TF_REMOTECLEARSBUFF) != 0);

	// �_�C�A���O�Ƀt�H�[�J�X�𓖂Ă� (2004.12.7 yutaka)
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_ACCEPT_MOUSE_EVENT_TRACKING));
}

BOOL CSequencePropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_ACCEPT_MOUSE_EVENT_TRACKING | (BN_CLICKED << 16):
			EnableDlgItem(IDC_DISABLE_MOUSE_TRACKING_CTRL,
						  GetCheck(IDC_ACCEPT_MOUSE_EVENT_TRACKING) ? TRUE : FALSE);
			return TRUE;
	}
	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CSequencePropPageDlg::OnOK()
{
	// (1)IDC_ACCEPT_MOUSE_EVENT_TRACKING
	ts.MouseEventTracking = GetCheck(IDC_ACCEPT_MOUSE_EVENT_TRACKING);

	// (2)IDC_DISABLE_MOUSE_TRACKING_CTRL
	ts.DisableMouseTrackingByCtrl = GetCheck(IDC_DISABLE_MOUSE_TRACKING_CTRL);

	// (3)IDC_ACCEPT_TITLE_CHANGING
	int sel = GetCurSel(IDC_ACCEPT_TITLE_CHANGING);
	if (0 <= sel && sel <= IdTitleChangeRequestMax) {
		ts.AcceptTitleChangeRequest = sel;
	}

	// (4)IDC_TITLE_REPORT
	switch (GetCurSel(IDC_TITLE_REPORT)) {
		case 0:
			ts.WindowFlag &= ~WF_TITLEREPORT;
			break;
		case 1:
			ts.WindowFlag &= ~WF_TITLEREPORT;
			ts.WindowFlag |= IdTitleReportAccept;
			break;
		case 2:
			ts.WindowFlag |= IdTitleReportEmpty;
			break;
		default: // Invalid value.
			break;
	}

	// (5)IDC_WINDOW_CTRL
	if (((ts.WindowFlag & WF_WINDOWCHANGE) != 0) != GetCheck(IDC_WINDOW_CTRL)) {
		ts.WindowFlag ^= WF_WINDOWCHANGE;
	}

	// (6)IDC_WINDOW_REPORT
	if (((ts.WindowFlag & WF_WINDOWREPORT) != 0) != GetCheck(IDC_WINDOW_REPORT)) {
		ts.WindowFlag ^= WF_WINDOWREPORT;
	}

	// (7)IDC_CURSOR_CTRL_SEQ
	if (((ts.WindowFlag & WF_CURSORCHANGE) != 0) != GetCheck(IDC_CURSOR_CTRL_SEQ)) {
		ts.WindowFlag ^= WF_CURSORCHANGE;
	}

	// (8)IDC_CLIPBOARD_ACCESS
	switch (GetCurSel(IDC_CLIPBOARD_ACCESS)) {
		case 0: // off
			ts.CtrlFlag &= ~CSF_CBRW;
			break;
		case 1: // write only
			ts.CtrlFlag &= ~CSF_CBRW;
			ts.CtrlFlag |= CSF_CBWRITE;
			break;
		case 2: // read only
			ts.CtrlFlag &= ~CSF_CBRW;
			ts.CtrlFlag |= CSF_CBREAD;
			break;
		case 3: // read/write
			ts.CtrlFlag |= CSF_CBRW;
			break;
		default: // Invalid value.
			break;
	}

	// (9)IDC_CLIPBOARD_ACCESS
	ts.NotifyClipboardAccess = GetCheck(IDC_CLIPBOARD_NOTIFY);

	// (10)IDC_ACCEPT_CLEAR_SBUFF
	if (((ts.TermFlag & TF_REMOTECLEARSBUFF) != 0) != GetCheck(IDC_ACCEPT_CLEAR_SBUFF)) {
		ts.TermFlag ^= TF_REMOTECLEARSBUFF;
	}
}

void CSequencePropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditional, 0);
}

// CCopypastePropPageDlg �_�C�A���O

CCopypastePropPageDlg::CCopypastePropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CCopypastePropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_COPYPASTE",
				 L"Copy and Paste", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CCopypastePropPageDlg::~CCopypastePropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CCopypastePropPageDlg ���b�Z�[�W �n���h��

void CCopypastePropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_LINECOPY, "DLG_TAB_COPYPASTE_CONTINUE" },
		{ IDC_DISABLE_PASTE_RBUTTON, "DLG_TAB_COPYPASTE_MOUSEPASTE" },
		{ IDC_CONFIRM_PASTE_RBUTTON, "DLG_TAB_COPYPASTE_CONFIRMPASTE" },
		{ IDC_DISABLE_PASTE_MBUTTON, "DLG_TAB_COPYPASTE_MOUSEPASTEM" },
		{ IDC_SELECT_LBUTTON, "DLG_TAB_COPYPASTE_SELECTLBUTTON" },
		{ IDC_TRIMNLCHAR, "DLG_TAB_COPYPASTE_TRIM_TRAILING_NL" },
		{ IDC_CONFIRM_CHANGE_PASTE, "DLG_TAB_COPYPASTE_CONFIRM_CHANGE_PASTE" },
		{ IDC_CONFIRM_STRING_FILE_LABEL, "DLG_TAB_COPYPASTE_STRINGFILE" },
		{ IDC_DELIMITER, "DLG_TAB_COPYPASTE_DELIMITER" },
		{ IDC_PASTEDELAY_LABEL, "DLG_TAB_COPYPASTE_PASTEDELAY" },
		{ IDC_PASTEDELAY_LABEL2, "DLG_TAB_COPYPASTE_PASTEDELAY2" },
		{ IDC_SELECT_ON_ACTIVATE, "DLG_TAB_COPYPASTE_SELECT_ON_ACTIVATE" }
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	// (1)Enable continued-line copy
	SetCheck(IDC_LINECOPY, ts.EnableContinuedLineCopy);

	// (2)DisablePasteMouseRButton
	if (ts.PasteFlag & CPF_DISABLE_RBUTTON) {
		SetCheck(IDC_DISABLE_PASTE_RBUTTON, BST_CHECKED);
		EnableDlgItem(IDC_CONFIRM_PASTE_RBUTTON, FALSE);
	} else {
		SetCheck(IDC_DISABLE_PASTE_RBUTTON, BST_UNCHECKED);
		EnableDlgItem(IDC_CONFIRM_PASTE_RBUTTON, TRUE);
	}

	// (3)ConfirmPasteMouseRButton
	SetCheck(IDC_CONFIRM_PASTE_RBUTTON, (ts.PasteFlag & CPF_CONFIRM_RBUTTON)?BST_CHECKED:BST_UNCHECKED);

	// (4)DisablePasteMouseMButton
	SetCheck(IDC_DISABLE_PASTE_MBUTTON, (ts.PasteFlag & CPF_DISABLE_MBUTTON)?BST_CHECKED:BST_UNCHECKED);

	// (5)SelectOnlyByLButton
	SetCheck(IDC_SELECT_LBUTTON, ts.SelectOnlyByLButton);

	// (6)TrimTrailingNLonPaste
	SetCheck(IDC_TRIMNLCHAR, (ts.PasteFlag & CPF_TRIM_TRAILING_NL)?BST_CHECKED:BST_UNCHECKED);

	// (7)ConfirmChangePaste
	SetCheck(IDC_CONFIRM_CHANGE_PASTE, (ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE)?BST_CHECKED:BST_UNCHECKED);

	// �t�@�C���p�X
	SetDlgItemTextA(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile);
	if (ts.PasteFlag & CPF_CONFIRM_CHANGEPASTE) {
		EnableDlgItem(IDC_CONFIRM_STRING_FILE, TRUE);
		EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, TRUE);
	} else {
		EnableDlgItem(IDC_CONFIRM_STRING_FILE, FALSE);
		EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, FALSE);
	}

	// (8)delimiter characters
	SetDlgItemTextA(IDC_DELIM_LIST, ts.DelimList);

	// (9)PasteDelayPerLine
	char buf[64];
	_snprintf_s(buf, sizeof(buf), "%d", ts.PasteDelayPerLine);
	SetDlgItemNum(IDC_PASTEDELAY_EDIT, ts.PasteDelayPerLine);

	// (10) SelectOnActivate
	SetCheck(IDC_SELECT_ON_ACTIVATE, ts.SelOnActive ? BST_CHECKED : BST_UNCHECKED);

	// �_�C�A���O�Ƀt�H�[�J�X�𓖂Ă�
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_LINECOPY));
}

BOOL CCopypastePropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_DISABLE_PASTE_RBUTTON | (BN_CLICKED << 16):
			EnableDlgItem(IDC_CONFIRM_PASTE_RBUTTON,
						  GetCheck(IDC_DISABLE_PASTE_RBUTTON) ? FALSE : TRUE);
			return TRUE;

		case IDC_CONFIRM_CHANGE_PASTE | (BN_CLICKED << 16):
			if (GetCheck(IDC_CONFIRM_CHANGE_PASTE)) {
				EnableDlgItem(IDC_CONFIRM_STRING_FILE, TRUE);
				EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, TRUE);
			} else {
				EnableDlgItem(IDC_CONFIRM_STRING_FILE, FALSE);
				EnableDlgItem(IDC_CONFIRM_STRING_FILE_PATH, FALSE);
			}
			return TRUE;

		case IDC_CONFIRM_STRING_FILE_PATH | (BN_CLICKED << 16):
			{
				wchar_t fileW[_countof(ts.ConfirmChangePasteStringFile)];
				MultiByteToWideChar(CP_ACP, 0, ts.ConfirmChangePasteStringFile, -1, fileW, _countof(fileW));

				OPENFILENAMEW ofn;

				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = get_OPENFILENAME_SIZEW();
				ofn.hwndOwner = GetSafeHwnd();
				ofn.lpstrFilter = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_CONFIRM_STRING_APP_FILTER", L"txt(*.txt)\\0*.txt\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
				ofn.lpstrFile = fileW;
				ofn.nMaxFile = _countof(fileW);
				ofn.lpstrTitle = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_CONFIRM_STRING_APP_TITLE", L"Choose a file including strings for ConfirmChangePaste", ts.UILanguageFile);
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				BOOL ok = GetOpenFileNameW(&ofn);
				if (ok) {
					char *file = ToCharW(fileW);
					strncpy_s(ts.ConfirmChangePasteStringFile, sizeof(ts.ConfirmChangePasteStringFile), file, _TRUNCATE);
					free(file);
					SetDlgItemTextA(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile);
				}
				free((void *)ofn.lpstrFilter);
				free((void *)ofn.lpstrTitle);
			}
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CCopypastePropPageDlg::OnOK()
{
	char buf[64];
	int val;

	// (1)
	ts.EnableContinuedLineCopy = GetCheck(IDC_LINECOPY);

	// (2)
	if (GetCheck(IDC_DISABLE_PASTE_RBUTTON)) {
		ts.PasteFlag |= CPF_DISABLE_RBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_DISABLE_RBUTTON;
	}

	// (3)
	if (GetCheck(IDC_CONFIRM_PASTE_RBUTTON)) {
		ts.PasteFlag |= CPF_CONFIRM_RBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_CONFIRM_RBUTTON;
	}

	// (4)
	if (GetCheck(IDC_DISABLE_PASTE_MBUTTON)) {
		ts.PasteFlag |= CPF_DISABLE_MBUTTON;
	}
	else {
		ts.PasteFlag &= ~CPF_DISABLE_MBUTTON;
	}

	// (5)
	ts.SelectOnlyByLButton = GetCheck(IDC_SELECT_LBUTTON);

	// (6)
	if (GetCheck(IDC_TRIMNLCHAR)) {
		ts.PasteFlag |= CPF_TRIM_TRAILING_NL;
	}
	else {
		ts.PasteFlag &= ~CPF_TRIM_TRAILING_NL;
	}

	// (7)IDC_CONFIRM_CHANGE_PASTE
	if (GetCheck(IDC_CONFIRM_CHANGE_PASTE)) {
		ts.PasteFlag |= CPF_CONFIRM_CHANGEPASTE;
	}
	else {
		ts.PasteFlag &= ~CPF_CONFIRM_CHANGEPASTE;
	}
	GetDlgItemTextA(IDC_CONFIRM_STRING_FILE, ts.ConfirmChangePasteStringFile, sizeof(ts.ConfirmChangePasteStringFile));

	// (8)
	GetDlgItemTextA(IDC_DELIM_LIST, ts.DelimList, sizeof(ts.DelimList));

	// (9)
	GetDlgItemTextA(IDC_PASTEDELAY_EDIT, buf, sizeof(buf));
	val = atoi(buf);
	ts.PasteDelayPerLine =
		(val < 0) ? 0 :
		(val > 5000) ? 5000 : val;

	// (10) SelectOnActivate
	ts.SelOnActive = (GetCheck(IDC_SELECT_ON_ACTIVATE) == BST_CHECKED);
}

void CCopypastePropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditional, 0);
}

// CVisualPropPageDlg �_�C�A���O

CVisualPropPageDlg::CVisualPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CVisualPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_VISUAL",
				 L"Visual", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
	TipWin = new CTipWin(inst);
}

CVisualPropPageDlg::~CVisualPropPageDlg()
{
	free((void *)m_psp.pszTitle);
	TipWin->Destroy();
	delete TipWin;
	TipWin = NULL;
}

// CVisualPropPageDlg ���b�Z�[�W �n���h��

void CVisualPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_ALPHABLEND, "DLG_TAB_VISUAL_ALPHA" },
		{ IDC_ALPHA_BLEND_ACTIVE_LABEL, "DLG_TAB_VISUAL_ALPHA_ACTIVE" },
		{ IDC_ALPHA_BLEND_INACTIVE_LABEL, "DLG_TAB_VISUAL_ALPHA_INACTIVE" },
		{ IDC_MOUSE, "DLG_TAB_VISUAL_MOUSE" },
		{ IDC_FONT_QUALITY_LABEL, "DLG_TAB_VISUAL_FONT_QUALITY" },
		{ IDC_ANSICOLOR, "DLG_TAB_VISUAL_ANSICOLOR" },
		{ IDC_RED, "DLG_TAB_VISUAL_RED" },
		{ IDC_GREEN, "DLG_TAB_VISUAL_GREEN" },
		{ IDC_BLUE, "DLG_TAB_VISUAL_BLUE" },
		{ IDC_ENABLE_ATTR_COLOR_BOLD, "DLG_TAB_VISUAL_BOLD_COLOR" },		// SGR 1
		{ IDC_ENABLE_ATTR_FONT_BOLD, "DLG_TAB_VISUAL_BOLD_FONT" },
		{ IDC_ENABLE_ATTR_COLOR_UNDERLINE, "DLG_TAB_VISUAL_UNDERLINE_COLOR" },	// SGR 4
		{ IDC_ENABLE_ATTR_FONT_UNDERLINE, "DLG_TAB_VISUAL_UNDERLINE_FONT" },
		{ IDC_ENABLE_ATTR_COLOR_BLINK, "DLG_TAB_VISUAL_BLINK" },			// SGR 5
		{ IDC_ENABLE_ATTR_COLOR_REVERSE, "DLG_TAB_VISUAL_REVERSE" },		// SGR 7
		{ IDC_ENABLE_ATTR_COLOR_URL, "DLG_TAB_VISUAL_URL_COLOR" },			// URL Attribute
		{ IDC_ENABLE_ATTR_FONT_URL, "DLG_TAB_VISUAL_URL_FONT" },
		{ IDC_ENABLE_ANSI_COLOR, "DLG_TAB_VISUAL_ANSI" },
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	const static I18nTextInfo visual_font_quality[] = {
		{ "DLG_TAB_VISUAL_FONT_QUALITY_DEFAULT", L"Default" },
		{ "DLG_TAB_VISUAL_FONT_QUALITY_NONANTIALIASED", L"Non-Antialiased" },
		{ "DLG_TAB_VISUAL_FONT_QUALITY_ANTIALIASED", L"Antialiased" },
		{ "DLG_TAB_VISUAL_FONT_QUALITY_CLEARTYPE", L"ClearType" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_FONT_QUALITY, visual_font_quality, _countof(visual_font_quality),
				 ts.UILanguageFileW, 0);

	// (1)AlphaBlend

	SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, ts.AlphaBlendActive);
	SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
	SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_SETPOS, TRUE, ts.AlphaBlendActive);

	SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, ts.AlphaBlendInactive);
	SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
	SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_SETPOS, TRUE, ts.AlphaBlendInactive);

	BOOL isLayeredWindowSupported = (pSetLayeredWindowAttributes != NULL);
	EnableDlgItem(IDC_ALPHA_BLEND_ACTIVE, isLayeredWindowSupported);
	EnableDlgItem(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, isLayeredWindowSupported);
	EnableDlgItem(IDC_ALPHA_BLEND_INACTIVE, isLayeredWindowSupported);
	EnableDlgItem(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, isLayeredWindowSupported);

	// (2) theme file
	{
		SendDlgItemMessageA(IDC_THEME_FILE, CB_ADDSTRING, 0, (LPARAM)"�g�p���Ȃ�");
		SendDlgItemMessageA(IDC_THEME_FILE, CB_ADDSTRING, 1, (LPARAM)"�Œ�e�[�}(�e�[�}�t�@�C���w��)");
		SendDlgItemMessageA(IDC_THEME_FILE, CB_ADDSTRING, 2, (LPARAM)"�����_���e�[�}");
		int sel = ts.EtermLookfeel.BGEnable;
		if (sel < 0) sel = 0;
		if (sel > 2) sel = 2;
		SendDlgItemMessageA(IDC_THEME_FILE, CB_SETCURSEL, sel, 0);
		BOOL enable = (sel == 1) ? TRUE : FALSE;
		EnableDlgItem(IDC_THEME_EDIT, enable);
		EnableDlgItem(IDC_THEME_BUTTON, enable);

		SetDlgItemTextW(IDC_THEME_EDIT, ts.EtermLookfeel.BGThemeFileW);
	}

	// (3)Mouse cursor type
	int sel = 0;
	for (int i = 0 ; MouseCursor[i].name ; i++) {
		const char *name = MouseCursor[i].name;
		SendDlgItemMessageA(IDC_MOUSE_CURSOR, CB_ADDSTRING, i, (LPARAM)name);
		if (_stricmp(name, ts.MouseCursorName) == 0) {
			sel = i;
		}
	}
	SetCurSel(IDC_MOUSE_CURSOR, sel);

	// (4)Font quality
	switch (ts.FontQuality) {
		case DEFAULT_QUALITY:
			SetCurSel(IDC_FONT_QUALITY, 0);
			break;
		case NONANTIALIASED_QUALITY:
			SetCurSel(IDC_FONT_QUALITY, 1);
			break;
		case ANTIALIASED_QUALITY:
			SetCurSel(IDC_FONT_QUALITY, 2);
			break;
		default: // CLEARTYPE_QUALITY
			SetCurSel(IDC_FONT_QUALITY, 3);
			break;
	}

	// (5)ANSI color
	for (int i = 0 ; i < 16 ; i++) {
		char buf[4];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", i);
		SendDlgItemMessageA(IDC_ANSI_COLOR, LB_INSERTSTRING, i, (LPARAM)buf);
	}
	SetupRGBbox(0);
	SendDlgItemMessage(IDC_ANSI_COLOR, LB_SETCURSEL, 0, 0);
	::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);

	// (6)Bold Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_BOLD, (ts.ColorFlag&CF_BOLDCOLOR) != 0);
	SetCheck(IDC_ENABLE_ATTR_FONT_BOLD, (ts.FontFlag&FF_BOLD) != 0);

	// (7)Blink Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_BLINK, (ts.ColorFlag&CF_BLINKCOLOR) != 0);

	// (8)Reverse Attr Color
	SetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE, (ts.ColorFlag&CF_REVERSECOLOR) != 0);

	// Underline Attr
	SetCheck(IDC_ENABLE_ATTR_COLOR_UNDERLINE, (ts.ColorFlag&CF_UNDERLINE) != 0);
	SetCheck(IDC_ENABLE_ATTR_FONT_UNDERLINE, (ts.FontFlag&FF_UNDERLINE) != 0);

	// URL Underline Attr
	SetCheck(IDC_ENABLE_ATTR_COLOR_URL, (ts.ColorFlag&CF_URLCOLOR) != 0);
	SetCheck(IDC_ENABLE_ATTR_FONT_URL, (ts.FontFlag&FF_URLUNDERLINE) != 0);

	// Color
	SetCheck(IDC_ENABLE_ANSI_COLOR, (ts.ColorFlag&CF_ANSICOLOR) != 0);

	SetCheck(IDC_CHECK_FAST_SIZE_MOVE, ts.EtermLookfeel.BGFastSizeMove != 0);
	SetCheck(IDC_CHECK_FLICKER_LESS_MOVE, ts.EtermLookfeel.BGNoCopyBits != 0);

	// �_�C�A���O�Ƀt�H�[�J�X�𓖂Ă�
	::SetFocus(GetDlgItem(IDC_ALPHA_BLEND_ACTIVE));

	// �c�[���`�b�v�쐬
	TipWin->Create(m_hWnd);
}

void CVisualPropPageDlg::OnHScroll(UINT nSBCode, UINT nPos, HWND pScrollBar)
{
	int pos;
	if ( pScrollBar == GetDlgItem(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR) ) {
		switch (nSBCode) {
			case SB_TOP:
			case SB_BOTTOM:
			case SB_LINEDOWN:
			case SB_LINEUP:
			case SB_PAGEDOWN:
			case SB_PAGEUP:
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				pos = SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_GETPOS, NULL, NULL);
				SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, pos);
				break;
			case SB_ENDSCROLL:
			default:
				return;
		}
	}
	else if ( pScrollBar == GetDlgItem(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR) ) {
		switch (nSBCode) {
			case SB_TOP:
			case SB_BOTTOM:
			case SB_LINEDOWN:
			case SB_LINEUP:
			case SB_PAGEDOWN:
			case SB_PAGEUP:
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				pos = SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_GETPOS, NULL, NULL);
				SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, pos);
				break;
			case SB_ENDSCROLL:
			default:
				return;
		}
	}
}

static void OpacityTooltip(CTipWin* tip, HWND hDlg, int trackbar, int pos, const wchar_t *UILanguageFile)
{
	wchar_t *uimsg;
	GetI18nStrWW("Tera Term", "TOOLTIP_TITLEBAR_OPACITY", L"Opacity %.1f %%", UILanguageFile, &uimsg);
	wchar_t *tipbuf;
	aswprintf(&tipbuf, uimsg, (pos / 255.0) * 100);
	RECT rc;
	::GetWindowRect(::GetDlgItem(hDlg, trackbar), &rc);
	tip->SetText(tipbuf);
	tip->SetPos(rc.right, rc.bottom);
	tip->SetHideTimer(1000);
	if (! tip->IsVisible()) {
		tip->SetVisible(TRUE);
	}
	free(tipbuf);
	free(uimsg);
}

BOOL CVisualPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int sel;

	switch (wParam) {
		case IDC_THEME_EDITOR_BUTTON | (BN_CLICKED << 16): {
			ThemeDialog(m_hInst, m_hWnd, &cv);
			break;
		}
		case IDC_THEME_FILE | (CBN_SELCHANGE << 16): {
			int r = GetCurSel(IDC_THEME_FILE);
			// �Œ�̂Ƃ��A�t�@�C��������͂ł���悤�ɂ���
			BOOL enable = (r == 1) ? TRUE : FALSE;
			EnableDlgItem(IDC_THEME_EDIT, enable);
			EnableDlgItem(IDC_THEME_BUTTON, enable);
			break;
		}
		case IDC_THEME_BUTTON | (BN_CLICKED << 16): {
			// �e�[�}�t�@�C����I������
			OPENFILENAMEW ofn = {};
			wchar_t szFile[MAX_PATH];
			wchar_t *curdir;
			wchar_t *theme_dir;
			hGetCurrentDirectoryW(&curdir);

			if (GetFileAttributesW(ts.EtermLookfeel.BGThemeFileW) != INVALID_FILE_ATTRIBUTES) {
				wcsncpy_s(szFile, _countof(szFile), ts.EtermLookfeel.BGThemeFileW, _TRUNCATE);
			} else {
				szFile[0] = 0;
			}

			aswprintf(&theme_dir, L"%s\\theme", ts.HomeDirW);

			ofn.lStructSize = get_OPENFILENAME_SIZEW();
			ofn.hwndOwner = GetSafeHwnd();
			ofn.lpstrFilter = L"Theme Files(*.ini)\0*.ini\0All Files(*.*)\0*.*\0";
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = _countof(szFile);
			ofn.lpstrTitle = L"select theme file";
			ofn.lpstrInitialDir = theme_dir;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			BOOL ok = GetOpenFileNameW(&ofn);
			SetCurrentDirectoryW(curdir);
			free(curdir);
			free(theme_dir);
			if (ok) {
				SetDlgItemTextW(IDC_THEME_EDIT, szFile);
			}
			return TRUE;
		}

		case IDC_EDIT_BGIMG_BRIGHTNESS | (EN_CHANGE << 16) :
			{
				int b;

				b = GetDlgItemInt(IDC_EDIT_BGIMG_BRIGHTNESS);
				if (b < 0) {
					b = 0;
					SetDlgItemNum(IDC_EDIT_BGIMG_BRIGHTNESS, b);
				}
				else if (b > 255) {
					b = 255;
					SetDlgItemNum(IDC_EDIT_BGIMG_BRIGHTNESS, b);
				}
			}
			return TRUE;

		case IDC_ANSI_COLOR | (LBN_SELCHANGE << 16):
			sel = SendDlgItemMessage(IDC_ANSI_COLOR, LB_GETCURSEL, 0, 0);
			if (sel != -1) {
				SetupRGBbox(sel);
				::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);
			}
			return TRUE;

		case IDC_COLOR_RED | (EN_CHANGE << 16) :
		case IDC_COLOR_GREEN | (EN_CHANGE << 16) :
		case IDC_COLOR_BLUE | (EN_CHANGE << 16) :
			{
				int r, g, b;

				sel = GetCurSel(IDC_ANSI_COLOR);
				if (sel < 0 || sel > _countof(ts.ANSIColor)-1) {
					return TRUE;
				}

				r = GetDlgItemInt(IDC_COLOR_RED);
				if (r < 0) {
					r = 0;
					SetDlgItemNum(IDC_COLOR_RED, r);
				}
				else if (r > 255) {
					r = 255;
					SetDlgItemNum(IDC_COLOR_RED, r);
				}

				g = GetDlgItemInt(IDC_COLOR_GREEN);
				if (g < 0) {
					g = 0;
					SetDlgItemNum(IDC_COLOR_GREEN, g);
				}
				else if (g > 255) {
					g = 255;
					SetDlgItemNum(IDC_COLOR_GREEN, g);
				}

				b = GetDlgItemInt(IDC_COLOR_BLUE);
				if (b < 0) {
					b = 0;
					SetDlgItemNum(IDC_COLOR_BLUE, b);
				}
				else if (b > 255) {
					b = 255;
					SetDlgItemNum(IDC_COLOR_BLUE, b);
				}

				// OK �������Ȃ��Ă��ݒ肪�ۑ�����Ă���
				ts.ANSIColor[sel] = RGB(r, g, b);

				::InvalidateRect(GetDlgItem(IDC_SAMPLE_COLOR), NULL, TRUE);
			}
			return TRUE;
		case IDC_ALPHA_BLEND_ACTIVE | (EN_CHANGE << 16):
			{
				int pos;
				pos = GetDlgItemInt(IDC_ALPHA_BLEND_ACTIVE);
				if(pos < 0) {
					pos = 0;
					SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, pos);
				}
				else if(pos > 255) {
					pos = 255;
					SetDlgItemNum(IDC_ALPHA_BLEND_ACTIVE, pos);
				}
				SendDlgItemMessage(IDC_ALPHA_BLEND_ACTIVE_TRACKBAR, TBM_SETPOS, TRUE, pos);
				OpacityTooltip(TipWin, m_hWnd, IDC_ALPHA_BLEND_ACTIVE, pos, ts.UILanguageFileW);
				return TRUE;
			}
		case IDC_ALPHA_BLEND_INACTIVE | (EN_CHANGE << 16):
			{
				int pos;
				pos = GetDlgItemInt(IDC_ALPHA_BLEND_INACTIVE);
				if(pos < 0) {
					pos = 0;
					SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, pos);
				}
				else if(pos > 255) {
					pos = 255;
					SetDlgItemNum(IDC_ALPHA_BLEND_INACTIVE, pos);
				}
				SendDlgItemMessage(IDC_ALPHA_BLEND_INACTIVE_TRACKBAR, TBM_SETPOS, TRUE, pos);
				OpacityTooltip(TipWin, m_hWnd, IDC_ALPHA_BLEND_INACTIVE, pos, ts.UILanguageFileW);
				return TRUE;
			}
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

HBRUSH CVisualPropPageDlg::OnCtlColor(HDC hDC, HWND hWnd)
{
	if ( hWnd == GetDlgItem(IDC_SAMPLE_COLOR) ) {
		BYTE r, g, b;
		char buf[8];

		GetDlgItemTextA(IDC_COLOR_RED, buf, sizeof(buf));
		r = atoi(buf);

		GetDlgItemTextA(IDC_COLOR_GREEN, buf, sizeof(buf));
		g = atoi(buf);

		GetDlgItemTextA(IDC_COLOR_BLUE, buf, sizeof(buf));
		b = atoi(buf);

		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, RGB(r, g, b) );

		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return TTCPropertyPage::OnCtlColor(hDC, hWnd);
}

void CVisualPropPageDlg::OnOK()
{
	int sel;
	char buf[MAXPATHLEN];
	int flag_changed = 0;

	// (1)
	GetDlgItemTextA(IDC_ALPHA_BLEND_ACTIVE, buf, sizeof(buf));
	if (isdigit(buf[0])) {
		int i = atoi(buf);
		ts.AlphaBlendActive =
			(i < 0) ? 0 :
			(i > 255) ? 255 : i;
	}
	GetDlgItemTextA(IDC_ALPHA_BLEND_INACTIVE, buf, sizeof(buf));
	if (isdigit(buf[0])) {
		int i = atoi(buf);
		ts.AlphaBlendInactive =
			(i < 0) ? 0 :
			(i > 255) ? 255 : i;
	}

	// (2) �e�[�}�t�@�C���I��
	{
		int r = GetCurSel(IDC_THEME_FILE);
		switch (r) {
		default:
			assert(FALSE);
			// fall through
		case 0:
			ts.EtermLookfeel.BGEnable = 0;
			break;
		case 1: {
			// �e�[�}�t�@�C���w��
			ts.EtermLookfeel.BGEnable = 1;

			wchar_t* theme_file;
			hGetDlgItemTextW(m_hWnd, IDC_THEME_EDIT, &theme_file);

			if (ts.EtermLookfeel.BGThemeFileW != NULL) {
				free(ts.EtermLookfeel.BGThemeFileW);
			}
			ts.EtermLookfeel.BGThemeFileW = theme_file;
			break;
		}
		case 2: {
			// �����_���e�[�}
			ts.EtermLookfeel.BGEnable = 2;
			if (ts.EtermLookfeel.BGThemeFileW != NULL) {
				free(ts.EtermLookfeel.BGThemeFileW);
			}
			ts.EtermLookfeel.BGThemeFileW = NULL;
			break;
		}
		}
	}


	// (3)
	sel = GetCurSel(IDC_MOUSE_CURSOR);
	if (sel >= 0 && sel < MOUSE_CURSOR_MAX) {
		strncpy_s(ts.MouseCursorName, sizeof(ts.MouseCursorName), MouseCursor[sel].name, _TRUNCATE);
	}

	// (4)Font quality
	switch (GetCurSel(IDC_FONT_QUALITY)) {
		case 0:
			ts.FontQuality = DEFAULT_QUALITY;
			break;
		case 1:
			ts.FontQuality = NONANTIALIASED_QUALITY;
			break;
		case 2:
			ts.FontQuality = ANTIALIASED_QUALITY;
			break;
		case 3:
			ts.FontQuality = CLEARTYPE_QUALITY;
			break;
		default: // Invalid value.
			break;
	}

	// (6) Attr Bold Color
	if (((ts.ColorFlag & CF_BOLDCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_BOLD)) {
		ts.ColorFlag ^= CF_BOLDCOLOR;
	}
	if (((ts.FontFlag & FF_BOLD) != 0) != GetCheck(IDC_ENABLE_ATTR_FONT_BOLD)) {
		ts.FontFlag ^= FF_BOLD;
	}

	// (7) Attr Blink Color
	if (((ts.ColorFlag & CF_BLINKCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_BLINK)) {
		ts.ColorFlag ^= CF_BLINKCOLOR;
	}

	// (8) Attr Reverse Color
	if (((ts.ColorFlag & CF_REVERSECOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_REVERSE)) {
		ts.ColorFlag ^= CF_REVERSECOLOR;
	}

	// Underline Attr
	if (((ts.FontFlag & FF_UNDERLINE) != 0) != GetCheck(IDC_ENABLE_ATTR_FONT_UNDERLINE)) {
		ts.FontFlag ^= FF_UNDERLINE;
	}
	if (((ts.ColorFlag & CF_UNDERLINE) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_UNDERLINE)) {
		ts.ColorFlag ^= CF_UNDERLINE;
	}

	// URL Underline Attr
	if (((ts.FontFlag & FF_URLUNDERLINE) != 0) != GetCheck(IDC_ENABLE_ATTR_FONT_URL)) {
		ts.FontFlag ^= FF_URLUNDERLINE;
	}
	if (((ts.ColorFlag & CF_URLCOLOR) != 0) != GetCheck(IDC_ENABLE_ATTR_COLOR_URL)) {
		ts.ColorFlag ^= CF_URLCOLOR;
	}

	// Color
	if (((ts.ColorFlag & CF_ANSICOLOR) != 0) != GetCheck(IDC_ENABLE_ANSI_COLOR)) {
		ts.ColorFlag ^= CF_ANSICOLOR;
	}

	ts.EtermLookfeel.BGFastSizeMove = GetCheck(IDC_CHECK_FAST_SIZE_MOVE);
	ts.EtermLookfeel.BGNoCopyBits = GetCheck(IDC_CHECK_FLICKER_LESS_MOVE);

	if (flag_changed) {
		// re-launch
		// RestartTeraTerm(GetSafeHwnd(), &ts);
	}
}

void CVisualPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalVisual, 0);
}

// CLogPropPageDlg �_�C�A���O

CLogPropPageDlg::CLogPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CLogPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_Log",
				 L"Log", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CLogPropPageDlg::~CLogPropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CLogPropPageDlg ���b�Z�[�W �n���h��

#define LOG_ROTATE_SIZETYPE_NUM 3
static const char *LogRotateSizeType[] = {
	"Byte", "KB", "MB"
};

static const char *GetLogRotateSizeType(int val)
{
	if (val >= LOG_ROTATE_SIZETYPE_NUM)
		val = 0;

	return LogRotateSizeType[val];
}

void CLogPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_VIEWLOG_LABEL, "DLG_TAB_LOG_EDITOR" },
		{ IDC_DEFAULTNAME_LABEL, "DLG_TAB_LOG_FILENAME" },
		{ IDC_DEFAULTPATH_LABEL, "DLG_TAB_LOG_FILEPATH" },
		{ IDC_AUTOSTART, "DLG_TAB_LOG_AUTOSTART" },
		// Log rotate
		{ IDC_LOG_ROTATE, "DLG_TAB_LOG_ROTATE" },
		{ IDC_ROTATE_SIZE_TEXT, "DLG_TAB_LOG_ROTATE_SIZE_TEXT" },
		{ IDC_ROTATE_STEP_TEXT, "DLG_TAB_LOG_ROTATESTEP" },
		// Log options
		// FIXME: ���b�Z�[�W�J�^���O�͊����̃��O�I�v�V�����̂��̂𗬗p�������A�A�N�Z�����[�^�L�[���d�����邩������Ȃ��B
		{ IDC_LOG_OPTION_GROUP, "DLG_FOPT" },
		{ IDC_OPT_BINARY, "DLG_FOPT_BINARY" },
		{ IDC_OPT_APPEND, "DLG_FOPT_APPEND" },
		{ IDC_OPT_PLAINTEXT, "DLG_FOPT_PLAIN" },
		{ IDC_OPT_HIDEDLG, "DLG_FOPT_HIDEDIALOG" },
		{ IDC_OPT_INCBUF, "DLG_FOPT_ALLBUFFINFIRST" },
		{ IDC_OPT_TIMESTAMP, "DLG_FOPT_TIMESTAMP" },
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	const static I18nTextInfo fopt_timestamp[] = {
		{ "DLG_FOPT_TIMESTAMP_LOCAL", L"Local Time" },
		{ "DLG_FOPT_TIMESTAMP_UTC", L"UTC" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_LOGGING", L"Elapsed Time (Logging)" },
		{ "DLG_FOPT_TIMESTAMP_ELAPSED_CONNECTION", L"Elapsed Time (Connection)" },
	};
	SetI18nListW("Tera Term", m_hWnd, IDC_OPT_TIMESTAMP_TYPE, fopt_timestamp, _countof(fopt_timestamp),
				 ts.UILanguageFileW, 0);

	// Viewlog Editor path (2005.1.29 yutaka)
	SetDlgItemTextA(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor);

	// Log Default File Name (2006.8.28 maya)
	SetDlgItemTextA(IDC_DEFAULTNAME_EDITOR, ts.LogDefaultName);

	// Log Default File Path (2007.5.30 maya)
	SetDlgItemTextW(IDC_DEFAULTPATH_EDITOR, ts.LogDefaultPathW);

	/* Auto start logging (2007.5.31 maya) */
	SetCheck(IDC_AUTOSTART, ts.LogAutoStart);

	// Log rotate
	SetCheck(IDC_LOG_ROTATE, ts.LogRotate != ROTATE_NONE);

	for (int i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
		SendDlgItemMessageA(IDC_ROTATE_SIZE_TYPE, CB_ADDSTRING, 0, (LPARAM)LogRotateSizeType[i]);
	}
	int TmpLogRotateSize = ts.LogRotateSize;
	for (int i = 0 ; i < ts.LogRotateSizeType ; i++)
		TmpLogRotateSize /= 1024;
	SetDlgItemInt(IDC_ROTATE_SIZE, TmpLogRotateSize, FALSE);
	SendDlgItemMessageA(IDC_ROTATE_SIZE_TYPE, CB_SELECTSTRING, -1, (LPARAM)GetLogRotateSizeType(ts.LogRotateSizeType));
	SetDlgItemInt(IDC_ROTATE_STEP, ts.LogRotateStep, FALSE);
	if (ts.LogRotate == ROTATE_NONE) {
		EnableDlgItem(IDC_ROTATE_SIZE_TEXT, FALSE);
		EnableDlgItem(IDC_ROTATE_SIZE, FALSE);
		EnableDlgItem(IDC_ROTATE_SIZE_TYPE, FALSE);
		EnableDlgItem(IDC_ROTATE_STEP_TEXT, FALSE);
		EnableDlgItem(IDC_ROTATE_STEP, FALSE);
	} else {
		EnableDlgItem(IDC_ROTATE_SIZE_TEXT, TRUE);
		EnableDlgItem(IDC_ROTATE_SIZE, TRUE);
		EnableDlgItem(IDC_ROTATE_SIZE_TYPE, TRUE);
		EnableDlgItem(IDC_ROTATE_STEP_TEXT, TRUE);
		EnableDlgItem(IDC_ROTATE_STEP, TRUE);
	}

	// Log options
	SetCheck(IDC_OPT_BINARY, ts.LogBinary != 0);
	if (ts.LogBinary) {
		EnableDlgItem(IDC_OPT_PLAINTEXT, FALSE);
		EnableDlgItem(IDC_OPT_TIMESTAMP, FALSE);
	} else {
		EnableDlgItem(IDC_OPT_PLAINTEXT, TRUE);
		EnableDlgItem(IDC_OPT_TIMESTAMP, TRUE);
	}
	SetCheck(IDC_OPT_APPEND, ts.Append != 0);
	SetCheck(IDC_OPT_PLAINTEXT, ts.LogTypePlainText != 0);
	SetCheck(IDC_OPT_HIDEDLG, ts.LogHideDialog != 0);
	SetCheck(IDC_OPT_INCBUF, ts.LogAllBuffIncludedInFirst != 0);
	SetCheck(IDC_OPT_TIMESTAMP, ts.LogTimestamp != 0);

	SetCurSel(IDC_OPT_TIMESTAMP_TYPE, ts.LogTimestampType);
	if (ts.LogBinary || !ts.LogTimestamp) {
		EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
	}
	else {
		EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
	}
/*
	switch (ts.LogTimestampType) {
		case CSF_CBRW:
			cmb->SetCurSel(3);
			break;
		case CSF_CBREAD:
			cmb->SetCurSel(2);
			break;
		case CSF_CBWRITE:
			cmb->SetCurSel(1);
			break;
		default: // off
			cmb->SetCurSel(0);
			break;
	}
*/

	// �_�C�A���O�Ƀt�H�[�J�X�𓖂Ă�
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_VIEWLOG_EDITOR));
}

BOOL CLogPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_VIEWLOG_PATH | (BN_CLICKED << 16):
			{
				wchar_t fileW[_countof(ts.ViewlogEditor)];
				MultiByteToWideChar(CP_ACP, 0, ts.ViewlogEditor, -1, fileW, _countof(fileW));

				OPENFILENAMEW ofn;

				memset(&ofn, 0, sizeof(ofn));
				ofn.lStructSize = get_OPENFILENAME_SIZEW();
				ofn.hwndOwner = GetSafeHwnd();
				ofn.lpstrFilter = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_LOGVIEW_APP_FILTER", L"exe(*.exe)\\0*.exe\\0all(*.*)\\0*.*\\0\\0", ts.UILanguageFile);
				ofn.lpstrFile = fileW;
				ofn.nMaxFile = _countof(fileW);
				ofn.lpstrTitle = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_LOGVIEW_APP_TITLE", L"Choose a executing file with launching logging file", ts.UILanguageFile);
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				BOOL ok = GetOpenFileNameW(&ofn);
				if (ok) {
					char *file = ToCharW(fileW);
					strncpy_s(ts.ViewlogEditor, sizeof(ts.ViewlogEditor), file, _TRUNCATE);
					free(file);
					SetDlgItemTextA(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor);
				}
				free((void *)ofn.lpstrFilter);
				free((void *)ofn.lpstrTitle);
			}
			return TRUE;

		case IDC_DEFAULTPATH_PUSH | (BN_CLICKED << 16):
			// ���O�f�B���N�g���̑I���_�C�A���O
			{
				wchar_t *title = TTGetLangStrW("Tera Term", "FILEDLG_SELECT_LOGDIR_TITLE", L"Select log folder", ts.UILanguageFile);
				wchar_t *default_path;
				hGetDlgItemTextW(m_hWnd, IDC_DEFAULTPATH_EDITOR, &default_path);
				if (default_path[0] == 0) {
					free(default_path);
					default_path = _wcsdup(ts.LogDirW);
				}
				wchar_t *new_path;
				if (doSelectFolderW(GetSafeHwnd(), default_path, title, &new_path)) {
					SetDlgItemTextW(IDC_DEFAULTPATH_EDITOR, new_path);
					free(new_path);
				}
				free(default_path);
				free(title);
			}

			return TRUE;

		case IDC_LOG_ROTATE | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_LOG_ROTATE)) {
					EnableDlgItem(IDC_ROTATE_SIZE_TEXT, TRUE);
					EnableDlgItem(IDC_ROTATE_SIZE, TRUE);
					EnableDlgItem(IDC_ROTATE_SIZE_TYPE, TRUE);
					EnableDlgItem(IDC_ROTATE_STEP_TEXT, TRUE);
					EnableDlgItem(IDC_ROTATE_STEP, TRUE);
				} else {
					EnableDlgItem(IDC_ROTATE_SIZE_TEXT, FALSE);
					EnableDlgItem(IDC_ROTATE_SIZE, FALSE);
					EnableDlgItem(IDC_ROTATE_SIZE_TYPE, FALSE);
					EnableDlgItem(IDC_ROTATE_STEP_TEXT, FALSE);
					EnableDlgItem(IDC_ROTATE_STEP, FALSE);
				}

			}
			return TRUE;

		case IDC_OPT_BINARY | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_OPT_BINARY)) {
					EnableDlgItem(IDC_OPT_PLAINTEXT, FALSE);
					EnableDlgItem(IDC_OPT_TIMESTAMP, FALSE);
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
				} else {
					EnableDlgItem(IDC_OPT_PLAINTEXT, TRUE);
					EnableDlgItem(IDC_OPT_TIMESTAMP, TRUE);

					if (GetCheck(IDC_OPT_TIMESTAMP)) {
						EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
					}
				}
			}
			return TRUE;

		case IDC_OPT_TIMESTAMP | (BN_CLICKED << 16):
			{
				if (GetCheck(IDC_OPT_TIMESTAMP)) {
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, TRUE);
				} else {
					EnableDlgItem(IDC_OPT_TIMESTAMP_TYPE, FALSE);
				}
			}
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

void CLogPropPageDlg::OnOK()
{
	char buf[80], buf2[80];
	time_t time_local;
	struct tm tm_local;

	// Viewlog Editor path (2005.1.29 yutaka)
	GetDlgItemTextA(IDC_VIEWLOG_EDITOR, ts.ViewlogEditor, _countof(ts.ViewlogEditor));

	// Log Default File Name (2006.8.28 maya)
	GetDlgItemTextA(IDC_DEFAULTNAME_EDITOR, buf, sizeof(buf));
	if (isInvalidStrftimeChar(buf)) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_LOGFILE_INVALID_CHAR_ERROR", L"Invalid character is included in log file name.",
			MB_ICONEXCLAMATION };
		TTMessageBoxA(m_hWnd, &info, ts.UILanguageFile);
		return;
	}

	// ���ݎ������擾
	time(&time_local);
	localtime_s(&tm_local, & time_local);
	// ����������ɕϊ�
	if (strlen(buf) != 0 && strftime(buf2, sizeof(buf2), buf, &tm_local) == 0) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_LOGFILE_TOOLONG_ERROR", L"The log file name is too long.",
			MB_ICONEXCLAMATION };
		TTMessageBoxA(m_hWnd, &info, ts.UILanguageFile);
		return;
	}
	if (isInvalidFileNameChar(buf2)) {
		static const TTMessageBoxInfoW info = {
			"Tera Term",
			"MSG_ERROR", L"ERROR",
			"MSG_LOGFILE_INVALID_CHAR_ERROR", L"Invalid character is included in log file name.",
			MB_ICONEXCLAMATION };
		TTMessageBoxA(m_hWnd, &info, ts.UILanguageFile);
		return;
	}
	strncpy_s(ts.LogDefaultName, sizeof(ts.LogDefaultName), buf, _TRUNCATE);

	// Log Default File Path (2007.5.30 maya)
	free(ts.LogDefaultPathW);
	hGetDlgItemTextW(m_hWnd, IDC_DEFAULTPATH_EDITOR, &ts.LogDefaultPathW);

	/* Auto start logging (2007.5.31 maya) */
	ts.LogAutoStart = GetCheck(IDC_AUTOSTART);

	/* Log Rotate */
	if (GetCheck(IDC_LOG_ROTATE)) {  /* on */
		ts.LogRotate = ROTATE_SIZE;
		GetDlgItemTextA(IDC_ROTATE_SIZE_TYPE, buf, _countof(buf));
		ts.LogRotateSizeType = 0;
		for (int i = 0 ; i < LOG_ROTATE_SIZETYPE_NUM ; i++) {
			if (strcmp(buf, LogRotateSizeType[i]) == 0) {
				ts.LogRotateSizeType = i;
				break;
			}
		}
		ts.LogRotateSize = GetDlgItemInt(IDC_ROTATE_SIZE);
		for (int i = 0 ; i < ts.LogRotateSizeType ; i++)
			ts.LogRotateSize *= 1024;

		ts.LogRotateStep = GetDlgItemInt(IDC_ROTATE_STEP);

	} else { /* off */
		ts.LogRotate = ROTATE_NONE;
		/* �c��̃����o�[�͈Ӑ}�I�ɐݒ���c���B*/
	}

	// Log Options
	if (GetCheck(IDC_OPT_BINARY)) {
		ts.LogBinary = 1;
	}
	else {
		ts.LogBinary = 0;
	}

	if (GetCheck(IDC_OPT_APPEND)) {
		ts.Append = 1;
	}
	else {
		ts.Append = 0;
	}

	if (GetCheck(IDC_OPT_PLAINTEXT)) {
		ts.LogTypePlainText = 1;
	}
	else {
		ts.LogTypePlainText = 0;
	}

	if (GetCheck(IDC_OPT_HIDEDLG)) {
		ts.LogHideDialog = 1;
	}
	else {
		ts.LogHideDialog = 0;
	}

	if (GetCheck(IDC_OPT_INCBUF)) {
		ts.LogAllBuffIncludedInFirst = 1;
	}
	else {
		ts.LogAllBuffIncludedInFirst = 0;
	}

	if (GetCheck(IDC_OPT_TIMESTAMP)) {
		ts.LogTimestamp = 1;
	}
	else {
		ts.LogTimestamp = 0;
	}

	ts.LogTimestampType = GetCurSel(IDC_OPT_TIMESTAMP_TYPE);
}

void CLogPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditionalLog, 0);
}

/////////////////////////////
// cygterm.cfg �ǂݏ���

#define CYGTERM_FILE "cygterm.cfg"  // CygTerm configuration file
#define CYGTERM_FILE_MAXLINE 100

void ReadCygtermConfFile(const char *homedir, cygterm_t *psettings)
{
	const char *cfgfile = CYGTERM_FILE; // CygTerm configuration file
	char cfg[MAX_PATH];
	FILE *fp;
	char buf[256], *head, *body;
	cygterm_t settings;

	// try to read CygTerm config file
	memset(&settings, 0, sizeof(settings));
	_snprintf_s(settings.term, sizeof(settings.term), _TRUNCATE, "ttermpro.exe %%s %%d /E /KR=SJIS /KT=SJIS /VTICON=CygTerm /nossh");
	_snprintf_s(settings.term_type, sizeof(settings.term_type), _TRUNCATE, "vt100");
	_snprintf_s(settings.port_start, sizeof(settings.port_start), _TRUNCATE, "20000");
	_snprintf_s(settings.port_range, sizeof(settings.port_range), _TRUNCATE, "40");
	_snprintf_s(settings.shell, sizeof(settings.shell), _TRUNCATE, "auto");
	_snprintf_s(settings.env1, sizeof(settings.env1), _TRUNCATE, "MAKE_MODE=unix");
	_snprintf_s(settings.env2, sizeof(settings.env2), _TRUNCATE, "");
	settings.login_shell = FALSE;
	settings.home_chdir = FALSE;
	settings.agent_proxy = FALSE;

	strncpy_s(cfg, sizeof(cfg), homedir, _TRUNCATE);
	AppendSlash(cfg, sizeof(cfg));
	strncat_s(cfg, sizeof(cfg), cfgfile, _TRUNCATE);

	fp = fopen(cfg, "r");
	if (fp != NULL) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			size_t len = strlen(buf);

			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';

			split_buffer(buf, '=', &head, &body);
			if (head == NULL || body == NULL)
				continue;

			if (_stricmp(head, "TERM") == 0) {
				_snprintf_s(settings.term, sizeof(settings.term), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "TERM_TYPE") == 0) {
				_snprintf_s(settings.term_type, sizeof(settings.term_type), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "PORT_START") == 0) {
				_snprintf_s(settings.port_start, sizeof(settings.port_start), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "PORT_RANGE") == 0) {
				_snprintf_s(settings.port_range, sizeof(settings.port_range), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "SHELL") == 0) {
				_snprintf_s(settings.shell, sizeof(settings.shell), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "ENV_1") == 0) {
				_snprintf_s(settings.env1, sizeof(settings.env1), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "ENV_2") == 0) {
				_snprintf_s(settings.env2, sizeof(settings.env2), _TRUNCATE, "%s", body);

			}
			else if (_stricmp(head, "LOGIN_SHELL") == 0) {
				if (strchr("YyTt", *body)) {
					settings.login_shell = TRUE;
				}

			}
			else if (_stricmp(head, "HOME_CHDIR") == 0) {
				if (strchr("YyTt", *body)) {
					settings.home_chdir = TRUE;
				}

			}
			else if (_stricmp(head, "SSH_AGENT_PROXY") == 0) {
				if (strchr("YyTt", *body)) {
					settings.agent_proxy = TRUE;
				}

			}
			else {
				// TODO: error check

			}
		}
		fclose(fp);
	}

	memcpy(psettings, &settings, sizeof(cygterm_t));
}

BOOL WriteCygtermConfFile(const char *homedir, cygterm_t *psettings)
{
	const char *cfgfile = CYGTERM_FILE; // CygTerm configuration file
	const char *tmpfile = "cygterm.tmp";
	char cfg[MAX_PATH];
	char tmp[MAX_PATH];
	FILE *fp;
	FILE *tmp_fp;
	char buf[256], *head, *body;
	cygterm_t settings;
	char *line[CYGTERM_FILE_MAXLINE];
	int i, linenum;

	memcpy(&settings, psettings, sizeof(cygterm_t));

	strncpy_s(cfg, sizeof(cfg), homedir, _TRUNCATE);
	AppendSlash(cfg, sizeof(cfg));
	strncat_s(cfg, sizeof(cfg), cfgfile, _TRUNCATE);

	strncpy_s(tmp, sizeof(tmp), homedir, _TRUNCATE);
	AppendSlash(tmp, sizeof(tmp));
	strncat_s(tmp, sizeof(tmp), tmpfile, _TRUNCATE);

	// cygterm.cfg �����݂���΁A�������񃁃����ɂ��ׂēǂݍ��ށB
	memset(line, 0, sizeof(line));
	linenum = 0;
	fp = fopen(cfg, "r");
	if (fp) {
		i = 0;
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			size_t len = strlen(buf);
			if (buf[len - 1] == '\n')
				buf[len - 1] = '\0';
			if (i < CYGTERM_FILE_MAXLINE)
				line[i++] = _strdup(buf);
			else
				break;
		}
		linenum = i;
		fclose(fp);
	}

	tmp_fp = fopen(cfg, "w");
	if (tmp_fp == NULL) {
		return FALSE;
#if 0
		char uimsg[MAX_UIMSG];
		get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts->UILanguageFile);
		get_lang_msg("MSG_CYGTERM_CONF_WRITEFILE_ERROR", ts->UIMsg, sizeof(ts->UIMsg),
			"Can't write CygTerm configuration file (%d).", ts->UILanguageFile);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts->UIMsg, GetLastError());
		MessageBox(NULL, buf, uimsg, MB_ICONEXCLAMATION);
#endif
	}
	else {
		if (linenum > 0) {
			for (i = 0; i < linenum; i++) {
				split_buffer(line[i], '=', &head, &body);
				if (head == NULL || body == NULL) {
					fprintf(tmp_fp, "%s\n", line[i]);
				}
				else if (_stricmp(head, "TERM") == 0) {
					fprintf(tmp_fp, "TERM = %s\n", settings.term);
					settings.term[0] = '\0';
				}
				else if (_stricmp(head, "TERM_TYPE") == 0) {
					fprintf(tmp_fp, "TERM_TYPE = %s\n", settings.term_type);
					settings.term_type[0] = '\0';
				}
				else if (_stricmp(head, "PORT_START") == 0) {
					fprintf(tmp_fp, "PORT_START = %s\n", settings.port_start);
					settings.port_start[0] = '\0';
				}
				else if (_stricmp(head, "PORT_RANGE") == 0) {
					fprintf(tmp_fp, "PORT_RANGE = %s\n", settings.port_range);
					settings.port_range[0] = '\0';
				}
				else if (_stricmp(head, "SHELL") == 0) {
					fprintf(tmp_fp, "SHELL = %s\n", settings.shell);
					settings.shell[0] = '\0';
				}
				else if (_stricmp(head, "ENV_1") == 0) {
					fprintf(tmp_fp, "ENV_1 = %s\n", settings.env1);
					settings.env1[0] = '\0';
				}
				else if (_stricmp(head, "ENV_2") == 0) {
					fprintf(tmp_fp, "ENV_2 = %s\n", settings.env2);
					settings.env2[0] = '\0';
				}
				else if (_stricmp(head, "LOGIN_SHELL") == 0) {
					fprintf(tmp_fp, "LOGIN_SHELL = %s\n", (settings.login_shell == TRUE) ? "yes" : "no");
					settings.login_shell = FALSE;
				}
				else if (_stricmp(head, "HOME_CHDIR") == 0) {
					fprintf(tmp_fp, "HOME_CHDIR = %s\n", (settings.home_chdir == TRUE) ? "yes" : "no");
					settings.home_chdir = FALSE;
				}
				else if (_stricmp(head, "SSH_AGENT_PROXY") == 0) {
					fprintf(tmp_fp, "SSH_AGENT_PROXY = %s\n", (settings.agent_proxy == TRUE) ? "yes" : "no");
					settings.agent_proxy = FALSE;
				}
				else {
					fprintf(tmp_fp, "%s = %s\n", head, body);
				}
			}
		}
		else {
			fputs("# CygTerm setting\n", tmp_fp);
			fputs("\n", tmp_fp);
		}
		if (settings.term[0] != '\0') {
			fprintf(tmp_fp, "TERM = %s\n", settings.term);
		}
		if (settings.term_type[0] != '\0') {
			fprintf(tmp_fp, "TERM_TYPE = %s\n", settings.term_type);
		}
		if (settings.port_start[0] != '\0') {
			fprintf(tmp_fp, "PORT_START = %s\n", settings.port_start);
		}
		if (settings.port_range[0] != '\0') {
			fprintf(tmp_fp, "PORT_RANGE = %s\n", settings.port_range);
		}
		if (settings.shell[0] != '\0') {
			fprintf(tmp_fp, "SHELL = %s\n", settings.shell);
		}
		if (settings.env1[0] != '\0') {
			fprintf(tmp_fp, "ENV_1 = %s\n", settings.env1);
		}
		if (settings.env2[0] != '\0') {
			fprintf(tmp_fp, "ENV_2 = %s\n", settings.env2);
		}
		if (settings.login_shell) {
			fprintf(tmp_fp, "LOGIN_SHELL = yes\n");
		}
		if (settings.home_chdir) {
			fprintf(tmp_fp, "HOME_CHDIR = yes\n");
		}
		if (settings.agent_proxy) {
			fprintf(tmp_fp, "SSH_AGENT_PROXY = yes\n");
		}
		fclose(tmp_fp);

		// �_�C���N�g�Ƀt�@�C���ɏ������ނ悤�ɂ����̂ŁA���L�����͕s�v�B
#if 0
		if (remove(cfg) != 0 && errno != ENOENT) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts->UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_REMOVEFILE_ERROR", ts->UIMsg, sizeof(ts->UIMsg),
				"Can't remove old CygTerm configuration file (%d).", ts->UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts->UIMsg, GetLastError());
			MessageBox(NULL, buf, uimsg, MB_ICONEXCLAMATION);
		}
		else if (rename(tmp, cfg) != 0) {
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts->UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_RENAMEFILE_ERROR", ts->UIMsg, sizeof(ts->UIMsg),
				"Can't rename CygTerm configuration file (%d).", ts->UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts->UIMsg, GetLastError());
			MessageBox(NULL, buf, uimsg, MB_ICONEXCLAMATION);
		}
		else {
			// cygterm.cfg �t�@�C���ւ̕ۑ�������������A���b�Z�[�W�_�C�A���O��\������B
			// ���߂āASave setup�����s����K�v�͂Ȃ����Ƃ𒍈ӊ��N����B
			// (2012.5.1 yutaka)
			// Save setup ���s���ɁACygTerm�̐ݒ��ۑ�����悤�ɂ������Ƃɂ��A
			// �_�C�A���O�\�����s�v�ƂȂ邽�߁A�폜����B
			// (2015.11.12 yutaka)
			get_lang_msg("MSG_TT_NOTICE", uimsg, sizeof(uimsg), "MSG_TT_NOTICE", ts->UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_SAVED_NOTICE", ts->UIMsg, sizeof(ts->UIMsg),
				"%s has been saved. Do not do save setup.", ts->UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts->UIMsg, CYGTERM_FILE);
			MessageBox(NULL, buf, uimsg, MB_OK | MB_ICONINFORMATION);
		}
#endif
	}

	// �Y�ꂸ�Ƀ������t���[���Ă����B
	for (i = 0; i < linenum; i++) {
		free(line[i]);
	}

	return TRUE;
}

/////////////////////////////

// CCygwinPropPageDlg �_�C�A���O

CCygwinPropPageDlg::CCygwinPropPageDlg(HINSTANCE inst)
	: TTCPropertyPage(inst, CCygwinPropPageDlg::IDD)
{
	wchar_t *UIMsg;
	GetI18nStrWW("Tera Term", "DLG_TABSHEET_TITLE_CYGWIN",
				 L"Cygwin", ts.UILanguageFileW, &UIMsg);
	m_psp.pszTitle = UIMsg;
	m_psp.dwFlags |= (PSP_USETITLE | PSP_HASHELP);
}

CCygwinPropPageDlg::~CCygwinPropPageDlg()
{
	free((void *)m_psp.pszTitle);
}

// CCygwinPropPageDlg ���b�Z�[�W �n���h��

void CCygwinPropPageDlg::OnInitDialog()
{
	TTCPropertyPage::OnInitDialog();

	static const DlgTextInfo TextInfos[] = {
		{ IDC_CYGWIN_PATH_LABEL, "DLG_TAB_CYGWIN_PATH" }
	};
	SetDlgTextsW(m_hWnd, TextInfos, _countof(TextInfos), ts.UILanguageFileW);

	ReadCygtermConfFile(ts.HomeDir, &settings);

	SetDlgItemTextA(IDC_TERM_EDIT, settings.term);
	SetDlgItemTextA(IDC_TERM_TYPE, settings.term_type);
	SetDlgItemTextA(IDC_PORT_START, settings.port_start);
	SetDlgItemTextA(IDC_PORT_RANGE, settings.port_range);
	SetDlgItemTextA(IDC_SHELL, settings.shell);
	SetDlgItemTextA(IDC_ENV1, settings.env1);
	SetDlgItemTextA(IDC_ENV2, settings.env2);

	SetCheck(IDC_LOGIN_SHELL, settings.login_shell);
	SetCheck(IDC_HOME_CHDIR, settings.home_chdir);
	SetCheck(IDC_AGENT_PROXY, settings.agent_proxy);

	// Cygwin install path
	SetDlgItemTextA(IDC_CYGWIN_PATH, ts.CygwinDirectory);

	// �_�C�A���O�Ƀt�H�[�J�X�𓖂Ă�
	::SetFocus(::GetDlgItem(GetSafeHwnd(), IDC_CYGWIN_PATH));
}

BOOL CCygwinPropPageDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case IDC_SELECT_FILE | (BN_CLICKED << 16):
			// Cygwin install �f�B���N�g���̑I���_�C�A���O
			wchar_t *title = TTGetLangStrW("Tera Term", "DIRDLG_CYGTERM_DIR_TITLE", L"Select Cygwin directory", ts.UILanguageFile);
			wchar_t *buf;
			hGetDlgItemTextW(m_hWnd, IDC_CYGWIN_PATH, &buf);
			wchar_t *path;
			if (doSelectFolderW(GetSafeHwnd(), buf, title, &path)) {
				SetDlgItemTextW(IDC_CYGWIN_PATH, path);
				free(path);
			}
			free(buf);
			free(title);
			return TRUE;
	}

	return TTCPropertyPage::OnCommand(wParam, lParam);
}

/**
 * @brief	cygterm_t���r����
 * @param	a	cygterm_t
 * @param	b	cygterm_t
 * @retval	TRUE	����
 * @retval	FALSE	�قȂ�
 */
BOOL CmpCygtermConfFile(const cygterm_t *a, const cygterm_t *b)
{
	if ((strcmp(a->term, b->term) != 0) ||
		(strcmp(a->term_type, b->term_type) != 0) ||
		(strcmp(a->port_start, b->port_start) != 0) ||
		(strcmp(a->port_range, b->port_range) != 0) ||
		(strcmp(a->shell, b->shell) != 0) ||
		(strcmp(a->env1, b->env1) != 0) ||
		(strcmp(a->env2, b->env2) != 0) ||
		(a->login_shell != b->login_shell) ||
		(a->home_chdir != b->home_chdir) ||
		(a->agent_proxy != b->agent_proxy)) {
		return FALSE;
	}
	return TRUE;
}

void CCygwinPropPageDlg::OnOK()
{
	cygterm_t settings_prop;

	// �v���p�e�B�[�V�[�g����l����荞��
	GetDlgItemTextA(IDC_TERM_EDIT, settings_prop.term, sizeof(settings_prop.term));
	GetDlgItemTextA(IDC_TERM_TYPE, settings_prop.term_type, sizeof(settings_prop.term_type));
	GetDlgItemTextA(IDC_PORT_START, settings_prop.port_start, sizeof(settings_prop.port_start));
	GetDlgItemTextA(IDC_PORT_RANGE, settings_prop.port_range, sizeof(settings_prop.port_range));
	GetDlgItemTextA(IDC_SHELL, settings_prop.shell, sizeof(settings_prop.shell));
	GetDlgItemTextA(IDC_ENV1, settings_prop.env1, sizeof(settings_prop.env1));
	GetDlgItemTextA(IDC_ENV2, settings_prop.env2, sizeof(settings_prop.env2));

	settings_prop.login_shell = GetCheck(IDC_LOGIN_SHELL);
	settings_prop.home_chdir = GetCheck(IDC_HOME_CHDIR);
	settings_prop.agent_proxy = GetCheck(IDC_AGENT_PROXY);

	// �ύX����Ă���ꍇ cygterm.cfg �֏�������
	if (CmpCygtermConfFile(&settings_prop, &settings) == FALSE) {
		if (WriteCygtermConfFile(ts.HomeDir, &settings_prop) == FALSE) {
			char uimsg[MAX_UIMSG];
			char buf[256];
			get_lang_msg("MSG_ERROR", uimsg, sizeof(uimsg), "ERROR", ts.UILanguageFile);
			get_lang_msg("MSG_CYGTERM_CONF_WRITEFILE_ERROR", ts.UIMsg, sizeof(ts.UIMsg),
						 "Can't write CygTerm configuration file (%d).", ts.UILanguageFile);
			_snprintf_s(buf, sizeof(buf), _TRUNCATE, ts.UIMsg, GetLastError());
			MessageBoxA(buf, uimsg, MB_ICONEXCLAMATION);
		}
	}

	// Cygwin install path
	GetDlgItemTextA(IDC_CYGWIN_PATH, ts.CygwinDirectory, sizeof(ts.CygwinDirectory));
}

void CCygwinPropPageDlg::OnHelp()
{
	PostMessage(HVTWin, WM_USER_DLGHELP2, HlpMenuSetupAdditional, 0);
}

//////////////////////////////////////////////////////////////////////////////

// CAddSettingPropSheetDlg
CAddSettingPropSheetDlg::CAddSettingPropSheetDlg(HINSTANCE hInstance, HWND hParentWnd):
	TTCPropSheetDlg(hInstance, hParentWnd, ts.UILanguageFileW)
{
	// CPP,tmfc��TTCPropertyPage�h���N���X���琶��
	int i = 0;
	m_Page[i++] = new CGeneralPropPageDlg(hInstance);
	m_Page[i++] = new CSequencePropPageDlg(hInstance);
	m_Page[i++] = new CCopypastePropPageDlg(hInstance);
	m_Page[i++] = new CVisualPropPageDlg(hInstance);
	m_Page[i++] = new CLogPropPageDlg(hInstance);
	m_Page[i++] = new CCygwinPropPageDlg(hInstance);
	if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 ||
		(GetKeyState(VK_SHIFT) & 0x8000) != 0 ) {
		m_Page[i++] = new CDebugPropPage(hInstance);
	}
	m_PageCountCPP = i;

	HPROPSHEETPAGE page;
	for (i = 0; i < m_PageCountCPP; i++) {
		page = m_Page[i]->CreatePropertySheetPage();
		AddPage(page);
	}

	// TTCPropertyPage ���g�p���Ȃ� PropertyPage
	page = CodingPageCreate(hInstance, &ts);
	AddPage(page);
	page = FontPageCreate(hInstance, &ts);
	AddPage(page);

	wchar_t *title = TTGetLangStrW("Tera Term", "DLG_TABSHEET_TITLE", L"Tera Term: Additional settings", ts.UILanguageFile);
	SetCaption(title);
	free(title);
}

CAddSettingPropSheetDlg::~CAddSettingPropSheetDlg()
{
	for (int i = 0; i < m_PageCountCPP; i++) {
		delete m_Page[i];
	}
}

void CAddSettingPropSheetDlg::SetStartPage(Page page)
{
	int start_page = page == DefaultPage ? 0: 7;
	TTCPropSheetDlg::SetStartPage(start_page);
}
