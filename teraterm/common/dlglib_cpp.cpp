/*
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

/* Routines for dialog boxes */

#include <windows.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>

#include "dlglib.h"
#include "ttlib.h"
#include "layer_for_unicode.h"

/**
 *	EndDialog() �݊��֐�
 */
BOOL TTEndDialog(HWND hDlgWnd, INT_PTR nResult)
{
	return EndDialog(hDlgWnd, nResult);
}

/**
 *	CreateDialogIndirectParam() �݊��֐�
 */
HWND TTCreateDialogIndirectParam(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,			// �I�[�i�[�E�B���h�E�̃n���h��
	DLGPROC lpDialogFunc,		// �_�C�A���O�{�b�N�X�v���V�[�W���ւ̃|�C���^
	LPARAM lParamInit)			// �������l
{
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, lpTemplateName);
	HWND hDlgWnd = _CreateDialogIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
	free(lpTemplate);
	return hDlgWnd;
}

/**
 *	CreateDialogParam() �݊��֐�
 */
HWND TTCreateDialogParam(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc,
	LPARAM lParamInit)
{
	return TTCreateDialogIndirectParam(hInstance, lpTemplateName,
									   hWndParent, lpDialogFunc, lParamInit);
}

/**
 *	CreateDialog() �݊��֐�
 */
HWND TTCreateDialog(
	HINSTANCE hInstance,
	LPCTSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc)
{
	return TTCreateDialogParam(hInstance, lpTemplateName,
							   hWndParent, lpDialogFunc, NULL);
}

/**
 *	DialogBoxParam() �݊��֐�
 *		EndDialog()�ł͂Ȃ��ATTEndDialog()���g�p���邱��
 */
INT_PTR TTDialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName,
						 HWND hWndParent,		// �I�[�i�[�E�B���h�E�̃n���h��
						 DLGPROC lpDialogFunc,  // �_�C�A���O�{�b�N�X�v���V�[�W���ւ̃|�C���^
						 LPARAM lParamInit)		// �������l
{
	DLGTEMPLATE *lpTemplate = TTGetDlgTemplate(hInstance, lpTemplateName);
	INT_PTR DlgResult = _DialogBoxIndirectParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit);
	free(lpTemplate);
	return DlgResult;
}

/**
 *	DialogBox() �݊��֐�
 *		EndDialog()�ł͂Ȃ��ATTEndDialog()���g�p���邱��
 */
INT_PTR TTDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc)
{
	return TTDialogBoxParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, NULL);
}

/**
 *	�_�C�A���O�t�H���g��ݒ肷��
 */
void SetDialogFont(const char *FontName, int FontPoint, int FontCharSet,
				   const char *UILanguageFile, const char *Section, const char *Key)
{
	LOGFONTA logfont;
	BOOL result;

	// �w��t�H���g���Z�b�g
	if (FontName != NULL && FontName[0] != 0) {
		// ���݃`�F�b�N
		result = IsExistFontA(FontName, FontCharSet, TRUE);
		if (result == TRUE) {
			TTSetDlgFontA(FontName, FontPoint, FontCharSet);
			return;
		}
	}

	// .lng�̎w��
	if (UILanguageFile != NULL && Section != NULL && Key != NULL) {
		result = GetI18nLogfont(Section, Key, &logfont, 0, UILanguageFile);
		if (result == TRUE) {
			if (IsExistFontA(logfont.lfFaceName, logfont.lfCharSet, TRUE)) {
				TTSetDlgFontA(logfont.lfFaceName, logfont.lfHeight, logfont.lfCharSet);
				return;
			}
		}
	}

	// ini,lng�Ŏw�肳�ꂽ�t�H���g��������Ȃ������Ƃ��A
	// messagebox()�̃t�H���g���Ƃ肠�����I�����Ă���
	GetMessageboxFont(&logfont);
	if (logfont.lfHeight < 0) {
		logfont.lfHeight = GetFontPointFromPixel(NULL, -logfont.lfHeight);
	}
	TTSetDlgFontA(logfont.lfFaceName, logfont.lfHeight, logfont.lfCharSet);
}


/**
 *	pixel����point���ɕϊ�����(�t�H���g�p)
 *		�� 1point = 1/72 inch, �t�H���g�̒P��
 *		�� �E�B���h�E�̕\����Ŕ{�����ω�����̂� hWnd ���K�v
 */
int GetFontPixelFromPoint(HWND hWnd, int pixel)
{
	if (hWnd == NULL) {
		hWnd = GetDesktopWindow();
	}
	HDC DC = GetDC(hWnd);
	int dpi = GetDeviceCaps(DC, LOGPIXELSY);	// dpi = dot per inch (96DPI)
	int point = MulDiv(pixel, dpi, 72);			// pixel = point / 72 * dpi
	ReleaseDC(hWnd, DC);
	return point;
}

/**
 *	point����pixel���ɕϊ�����(�t�H���g�p)
 *		�� 1point = 1/72 inch, �t�H���g�̒P��
 */
int GetFontPointFromPixel(HWND hWnd, int point)
{
	HDC DC = GetDC(hWnd);
	int dpi = GetDeviceCaps(DC, LOGPIXELSY);	// dpi = dot per inch (96DPI)
	int pixel = MulDiv(point, 72, dpi);			// point = pixel / dpi * 72
	ReleaseDC(hWnd, DC);
	return pixel;
}

/**
 *	���X�g�̉������g������(���̕���苷���Ȃ邱�Ƃ͂Ȃ�)
 *	@param[in]	dlg		�_�C�A���O�̃n���h��
 *	@param[in]	ID		�R���{�{�b�N�X��ID
 */
void ExpandCBWidth(HWND dlg, int ID)
{
	HWND hCtrlWnd = GetDlgItem(dlg, ID);
	int count = (int)SendMessage(hCtrlWnd, CB_GETCOUNT, 0, 0);
	HFONT hFont = (HFONT)SendMessage(hCtrlWnd, WM_GETFONT, 0, 0);
	int i, max_width = 0;
	HDC TmpDC = GetDC(hCtrlWnd);
	hFont = (HFONT)SelectObject(TmpDC, hFont);
	for (i=0; i<count; i++) {
		SIZE s;
		int len = (int)SendMessage(hCtrlWnd, CB_GETLBTEXTLEN, i, 0);
		char *lbl = (char *)calloc(len+1, sizeof(char));
		SendMessage(hCtrlWnd, CB_GETLBTEXT, i, (LPARAM)lbl);
		GetTextExtentPoint32(TmpDC, lbl, len, &s);
		if (s.cx > max_width)
			max_width = s.cx;
		free(lbl);
	}
	max_width += GetSystemMetrics(SM_CXVSCROLL);	// �X�N���[���o�[�̕�����������ł���
	SendMessage(hCtrlWnd, CB_SETDROPPEDWIDTH, max_width, 0);
	SelectObject(TmpDC, hFont);
	ReleaseDC(hCtrlWnd, TmpDC);
}
