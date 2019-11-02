/*
 * Copyright (C) 2019 TeraTerm Project
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

#pragma once

#include "../common/tmfc.h"
#include "ttmlib.h"

class CMacroDlgBase : public TTCDialog
{
protected:
	int PosX, PosY;  // �E�B���h�E�ʒu
	int WW, WH;		 // �E�B���h�E��

	CMacroDlgBase()
	{
		PosX = 0;
		PosY = 0;
		WW = 0;
		WH = 0;
	}

	/**
	 *	�_�C�A���O�̃T�C�Y�ƈʒu��ݒ肷��
	 */
	void SetDlgPos()
	{
		if (WW == 0 || WH == 0) {
			// �E�B���h�E�ʒu�̂ݐݒ肷�� errdlg
			if (IsValidPos()) {
				::SetWindowPos(m_hWnd, HWND_TOP, PosX, PosY, 0, 0, SWP_NOSIZE);
			}
			else {
				// �����Ɉړ�����
				CenterWindow(m_hWnd, NULL);
				// �ʒu��ۑ�
				RECT rcWnd;
				GetWindowRect(&rcWnd);
				PosX = rcWnd.left;
				PosY = rcWnd.top;
			}
		}
		else {
			// �E�B���h�E�T�C�Y�����킹�Đݒ肷��
			if (IsValidPos()) {
				// �E�B���h�E�T�C�Y���Z�b�g + �w��ʒu�ֈړ�
				::SetWindowPos(m_hWnd, HWND_TOP, PosX, PosY, WW, WH, 0);
			}
			else {
				// �E�B���h�E�T�C�Y���Z�b�g
				::SetWindowPos(m_hWnd, HWND_TOP, 0, 0, WW, WH, SWP_NOMOVE);
				// �f�B�X�v���C�̒����Ɉړ�����
				CenterWindow(m_hWnd, NULL);
				// �ʒu��ۑ�
				RECT rcWnd;
				GetWindowRect(&rcWnd);
				PosX = rcWnd.left;
				PosY = rcWnd.top;
			}
		}
	}

private:
	/**
	 *	�_�C�A���O�ʒu���L��?
	 *	@retval TRUE	�L��
	 *	@retval FALSE	����
	 */
	BOOL IsValidPos()
	{
		// return !(PosX <= GetMonitorLeftmost(PosX, PosY) - 100);
		return !((PosX == CW_USEDEFAULT) || (PosY == CW_USEDEFAULT));
	}
};
