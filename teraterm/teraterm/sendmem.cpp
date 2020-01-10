/*
 * (C) 2019-2020 TeraTerm Project
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tttypes.h"
#include "ttcommon.h"
#include "ftdlg_lite.h"

#include "ttwinman.h"		// for ts

#define	SENDMEM_USE_OLD_API	0

#if SENDMEM_USE_OLD_API
#include "ttftypes.h"		// for TFileVar
#include "filesys.h"		// for SendVar
#include "codeconv.h"		// for ToCharW()
#else
#include "../ttpmacro/fileread.h"
#endif

#include "sendmem.h"

typedef enum {
	SendMemTypeTextLF,		// wchar_t 0x0a
	SendMemTypeTextCRLF,	// wchar_t 0x0d + 0x0a
	SendMemTypeBinary,
} SendMemType;

// ���M����VTWIN�ɔr����������
#define	USE_ENABLE_WINDOW	0	// 1=�r������

typedef struct SendMemTag {
	const BYTE *send_ptr;  // ���M�f�[�^�ւ̃|�C���^
	size_t send_len;	   // ���M�f�[�^�T�C�Y
	SendMemType type;
	BOOL local_echo_enable;
	BOOL send_enable;
	DWORD delay_per_line;  // (ms)
	DWORD delay_per_char;
	HWND hWnd;	 // �^�C�}�[���󂯂�window
	int timer_id;  // �^�C�}�[ID
	char *UILanguageFile;
	wchar_t *dialog_caption;
	wchar_t *filename;
	//
	size_t send_left;
	size_t send_index;
	BOOL waited;
	DWORD last_send_tick;
	//
	CFileTransLiteDlg *dlg;
	class SendMemDlgObserver *dlg_observer;
	HINSTANCE hInst_;
	HWND hWndParent_;
	//
	PComVar cv_;
	BOOL pause;
} SendMem;

typedef SendMem sendmem_work_t;

extern "C" IdTalk TalkStatus;
extern "C" HWND HVTWin;

static sendmem_work_t *sendmem_work;

class SendMemDlgObserver : public CFileTransLiteDlg::Observer {
public:
	SendMemDlgObserver(HWND hWndParent, void (*OnClose)(), void (*OnPause)(BOOL paused)) {
		hWndParent_ = hWndParent;
		OnClose_ = OnClose;
		OnPause_ = OnPause;
	}
private:
	void OnClose()
	{
		OnClose_();
	};
	void OnPause(BOOL paused)
	{
		OnPause_(paused);
	};
	void OnHelp()
	{
		MessageBoxA(hWndParent_, "no help topic", "Tera Term", MB_OK);
	}
	HWND hWndParent_;
	void (*OnClose_)();
	void (*OnPause_)(BOOL paused);
};

static wchar_t *wcsnchr(const wchar_t *str, size_t len, wchar_t chr)
{
	for (size_t i = 0; i < len; ++i) {
		wchar_t c = *str;
		if (c == 0) {
			return NULL;
		}
		if (c == chr) {
			return (wchar_t *)str;
		}
		str++;
	}
	return NULL;
}

static void EndPaste()
{
	sendmem_work_t *p = sendmem_work;
	p->send_ptr = NULL;

	TalkStatus = IdTalkKeyb;
	if (p->dlg != NULL) {
		p->dlg->Destroy();
		delete p->dlg;
		delete p->dlg_observer;
	}
	free(p->UILanguageFile);
	free(p->dialog_caption);
	free(p->filename);
	free(p);

	sendmem_work = NULL;

	// ����ł���悤�ɂ���
#if USE_ENABLE_WINDOW
	EnableWindow(HVTWin, TRUE);
	SetFocus(HVTWin);
#endif
}

static void OnClose()
{
	EndPaste();
}

static void OnPause(BOOL paused)
{
	sendmem_work_t *p = sendmem_work;
	p->pause = paused;
	if (!paused) {
		// �|�[�Y������, �^�C�}�[�C�x���g�ōđ��̃g���K������
		SetTimer(p->hWnd, p->timer_id, 0, NULL);
	}
}

static void SendMemStart_i(SendMem *sm)
{
	sendmem_work = sm;
	sendmem_work_t *p = sm;

	p->send_left = p->send_len;
	p->send_index = 0;
	p->waited = FALSE;
	p->pause = FALSE;

	if (p->hWndParent_ != NULL) {
		// �_�C�A���O�𐶐�����
		p->dlg = new CFileTransLiteDlg();
		p->dlg->Create(NULL, NULL, sm->UILanguageFile);
		if (p->dialog_caption != NULL) {
			p->dlg->SetCaption(p->dialog_caption);
		}
		if (p->filename != NULL) {
			p->dlg->SetFilename(p->filename);
		}
		p->dlg_observer = new SendMemDlgObserver(p->hWndParent_, OnClose, OnPause);
		p->dlg->SetObserver(p->dlg_observer);
	}

	TalkStatus = IdTalkSendMem;

	// ���M�J�n���ɃE�B���h�E�𑀍�ł��Ȃ��悤�ɂ���
#if USE_ENABLE_WINDOW
	EnableWindow(HVTWin, FALSE);
#endif
}

static void GetOutBuffInfo(const TComVar *cv_, size_t *use, size_t *free)
{
	if (use != NULL) {
		*use = cv_->OutBuffCount;
	}
	if (free != NULL) {
		*free = OutBuffSize - cv_->InBuffCount;
	}
}

static void GetInBuffInfo(const TComVar *cv_, size_t *use, size_t *free)
{
	if (use != NULL) {
		*use = cv_->InBuffCount;
	}
	if (free != NULL) {
		*free = InBuffSize - cv_->InBuffCount;
	}
}

/**
 * ���M
 */
void SendMemContinuously(void)
{
	sendmem_work_t *p = sendmem_work;

	if (p->send_ptr == NULL) {
		EndPaste();
		return;
	}

	if (p->pause) {
		return;
	}

	// �I�[?
	if (p->send_left == 0) {
		// �I��, ���M�o�b�t�@����ɂȂ�܂ő҂�
		size_t out_buff_use;
		GetOutBuffInfo(p->cv_, &out_buff_use, NULL);

		if (p->dlg != NULL) {
			p->dlg->RefreshNum(p->send_index, p->send_len - out_buff_use);
		}

		if (out_buff_use == 0) {
			// ���M�o�b�t�@����ɂȂ���
			EndPaste();
			return;
		}
	}

	if (p->waited) {
		const DWORD delay = p->delay_per_line > 0 ? p->delay_per_line : p->delay_per_char;
		if (GetTickCount() - p->last_send_tick < delay) {
			// �E�G�C�g����
			return;
		}
	}

	// ���M�ł���o�b�t�@�T�C�Y
	size_t buff_len;
	{
		size_t out_buff_free;
		GetOutBuffInfo(p->cv_, NULL, &out_buff_free);
		buff_len = out_buff_free;
		if (p->local_echo_enable) {
			// ���[�J���G�R�[���K�v�ȏꍇ�́A���̓o�b�t�@���l��
			size_t in_buff_free;
			GetInBuffInfo(p->cv_, NULL, &in_buff_free);
			if (buff_len > in_buff_free) {
				buff_len = in_buff_free;
			}
		}
	}
	if (buff_len == 0) {
		// �o�b�t�@�ɋ󂫂��Ȃ�
		return;
	}

	// ���M��
	BOOL need_delay = FALSE;
	size_t send_len;
	if (p->delay_per_char > 0) {
		// 1�L�����N�^���M
		need_delay = TRUE;
		if (p->type == SendMemTypeBinary) {
			send_len = 1;
		}
		else {
			send_len = sizeof(wchar_t);
		}
	}
	else if (p->delay_per_line > 0) {
		// 1���C�����M
		need_delay = TRUE;

		// 1�s���o��(���s�R�[�h�� 0x0a �ɐ��K������Ă���)
		const wchar_t *line_top = (wchar_t *)&p->send_ptr[p->send_index];
		const wchar_t *line_end = wcsnchr(line_top, p->send_left, 0x0a);
		if (line_end != NULL) {
			// 0x0a �܂ő��M
			send_len = ((line_end - line_top) + 1) * sizeof(wchar_t);
		}
		else {
			// ���s�����炸�A�Ō�܂ő��M
			send_len = p->send_left;
		}

		// 1�s�����M�o�b�t�@���傫��
		if (buff_len < send_len) {
			// ���M�o�b�t�@���܂Ő؂�l�߂�
			send_len = buff_len;
			return;
		}
	}
	else {
		// �S�͑��M
		send_len = p->send_left;
		if (buff_len < send_len) {
			send_len = buff_len;
		}
	}

	// ���M����
	const BYTE *send_ptr = (BYTE *)&p->send_ptr[p->send_index];
	p->send_index += send_len;
	p->send_left -= send_len;
	size_t sended_len;
	if (p->type == SendMemTypeBinary) {
		sended_len = CommBinaryBuffOut(p->cv_, (PCHAR)send_ptr, (int)send_len);
		if (p->local_echo_enable) {
			CommTextEcho(p->cv_, (PCHAR)send_ptr, (int)send_len);
		}
	}
	else {
		sended_len = CommTextOutW(p->cv_, (wchar_t *)send_ptr, (int)(send_len / sizeof(wchar_t)));
		if (p->local_echo_enable) {
			CommTextEchoW(p->cv_, (wchar_t *)send_ptr, (int)(send_len / sizeof(wchar_t)));
		}
		sended_len *= sizeof(wchar_t);
	}

	// �_�C�A���O�X�V
	if (p->dlg != NULL) {
		size_t out_buff_use;
		GetOutBuffInfo(p->cv_, &out_buff_use, NULL);
		p->dlg->RefreshNum(p->send_index - out_buff_use, p->send_len);
	}

	if (p->send_left != 0 && need_delay) {
		// wait�ɓ���
		p->waited = TRUE;
		p->last_send_tick = GetTickCount();
		// �^�C�}�[��idle�𓮍삳���邽�߂Ɏg�p���Ă���
		const DWORD delay = p->delay_per_line > 0 ? p->delay_per_line : p->delay_per_char;
		SetTimer(p->hWnd, p->timer_id, delay, NULL);
	}
}

/*
 *	���s�R�[�h��LF(0x0a)�����ɂ���
 *
 *	@param [in]	*src		������ւ̃|�C���^
 *	@param [in] *len		���͕�����(0�̂Ƃ������ŕ����񒷂𑪂�)
 *	@param [out] *len		�o�͕�����
 *	@return					�ϊ��㕶����(malloc���ꂽ�̈�)
 */
static wchar_t *NormalizeLineBreak(const wchar_t *src, size_t *len)
{
	size_t src_len = *len;
	if (src_len == 0) {
		src_len = wcslen(src) + 1;
	}
	wchar_t *dest_top = (wchar_t *)malloc(sizeof(wchar_t) * src_len);
	if (dest_top == NULL) {
		return NULL;
	}

	// CR+LF -> LF
	// CR    -> LF
	// LF    -> LF (�ϊ��s�v)
	int cr_count = 0;
	int lf_count = 0;
	const wchar_t *p = src;
	const wchar_t *p_end = src + src_len;
	wchar_t *dest = dest_top;
	while (p < p_end) {
		wchar_t c = *p++;
		if (c == CR) {
			if (*p == LF) {
				// CR+LF -> LF
				p++;
				*dest++ = LF;
			} else {
				// CR -> LF
				*dest++ = LF;
			}
		} else {
			*dest++ = c;
		}
	}

	*len = dest - dest_top;
	return dest_top;
}

/**
 *	������
 */
static SendMem *SendMemInit_()
{
	if (sendmem_work != NULL) {
		// ���M��
		return NULL;
	}
	SendMem *p = (SendMem *)calloc(sizeof(*p), 1);
	if (p == NULL) {
		return NULL;
	}

	p->send_ptr = NULL;
	p->send_len = 0;

	p->type = SendMemTypeBinary;
	p->local_echo_enable = FALSE;
	p->delay_per_char = 0;  // (ms)
	p->delay_per_line = 0;  // (ms)
	p->cv_ = NULL;
	p->hWnd = HVTWin;		// delay���Ɏg�p����^�C�}�[�p
	p->timer_id = IdPasteDelayTimer;
	p->hWndParent_ = NULL;
	return p;
}

/**
 *	�������ɂ���e�L�X�g�𑗐M����
 *	�f�[�^�͑��M�I�����free()�����
 *
 *	@param	ptr		�f�[�^�փ|�C���^(malloc()���ꂽ�̈�)
 *					���M��(���f��)�A�����I��free()�����
 *	@param	len		������(wchar_t�P��)
 *					0 �̏ꍇ�� L'\0' �܂�
 */
SendMem *SendMemTextW(wchar_t *str, size_t len)
{
	SendMem *p = SendMemInit_();
	if (p == NULL) {
		return NULL;
	}

	if (len == 0) {
		len = wcslen(str);
	}

	// ���s�R�[�h�𒲐����Ă���
	size_t new_len = len;
	p->send_ptr = (BYTE *)NormalizeLineBreak((wchar_t *)str, &new_len);
	p->send_len = new_len * sizeof(wchar_t);
	free(str);
	p->type = SendMemTypeTextLF;
	return p;
}

/**
 *	�������ɂ���f�[�^�𑗐M����
 *	�f�[�^�͑��M�I�����free()�����
 *
 *	@param	ptr		�f�[�^�փ|�C���^(malloc()���ꂽ�̈�)
 *					���M��(���f��)�A�����I��free()�����
 *	@param	len		�f�[�^��(byte)
 */
SendMem *SendMemBinary(void *ptr, size_t len)
{
	SendMem *p = SendMemInit_();
	if (p == NULL) {
		return NULL;
	}

	p->send_ptr = (BYTE *)ptr;
	p->send_len = len;
	p->type = SendMemTypeBinary;
	return p;
}

void SendMemInitEcho(SendMem *sm, BOOL echo)
{
	sm->local_echo_enable = echo;
}

void SendMemInitDelay(SendMem* sm, DWORD per_line, DWORD per_char)
{
	sm->delay_per_char = per_char;	// (ms)
	sm->delay_per_line = per_line;
}

// �Z�b�g����ƃ_�C�A���O���o��
void SendMemInitDialog(SendMem *sm, HINSTANCE hInstance, HWND hWndParent, const char *UILanguageFile)
{
	sm->hInst_ = hInstance;
	sm->hWndParent_ = hWndParent;
	sm->UILanguageFile = _strdup(UILanguageFile);
}

void SendMemInitDialogCaption(SendMem *sm, const wchar_t *caption)
{
	if (sm->dialog_caption != NULL)
		free(sm->dialog_caption);
	sm->dialog_caption = _wcsdup(caption);
}

void SendMemInitDialogFilename(SendMem *sm, const wchar_t *filename)
{
	if (sm->filename != NULL)
		free(sm->filename);
	sm->filename = _wcsdup(filename);
}

extern "C" TComVar cv;
BOOL SendMemStart(SendMem *sm)
{
	// ���M���`�F�b�N
	if (sendmem_work != NULL) {
		return FALSE;
	}

	sm->cv_ = &cv;		// TODO �Ȃ�������
	SendMemStart_i(sm);
	return TRUE;
}

void SendMemFinish(SendMem *sm)
{
	free(sm->UILanguageFile);
	free(sm);
}

/**
 *	�t�@�C���𑗐M����
 *	@param[in]	filename	�t�@�C����
 *	@param[in]	binary		FALSE	text file
 *							TRUE	binary file
 */
#if SENDMEM_USE_OLD_API
BOOL SendMemSendFile(const wchar_t *filename, BOOL binary)
{
	if (SendVar != NULL) {
		return FALSE;
	}
	if (!NewFileVar(&SendVar)) {
		return FALSE;
	}

	char *FileNameA = ToCharW(filename);
	strncpy_s(SendVar->FullName, sizeof(SendVar->FullName), FileNameA, _TRUNCATE);
	free(FileNameA);

	SendVar->DirLen = 0;
	ts.TransBin = binary == FALSE ? 0 : 1;
	FileSendStart();
	return TRUE;
}
#else
BOOL SendMemSendFile(const wchar_t *filename, BOOL binary)
{
	SendMem *sm;
	if (!binary) {
		size_t str_len;
		wchar_t *str_ptr = LoadFileWW(filename, &str_len);
		if (str_ptr == NULL) {
			return FALSE;
		}
		sm = SendMemTextW(str_ptr, str_len);
	}
	else {
		size_t data_len;
		unsigned char *data_ptr = LoadFileBinary(filename, &data_len);
		if (data_ptr == NULL) {
			return FALSE;
		}
		sm = SendMemBinary(data_ptr, data_len);
	}
	SendMemInitDialog(sm, hInst, HVTWin, ts.UILanguageFile);
	SendMemInitDialogCaption(sm, L"send file");			// title
	SendMemInitDialogFilename(sm, filename);
	SendMemStart(sm);
	return TRUE;
}
#endif

/**
 *	�Z��������𑗐M����
 *	@param[in]	str		malloc���ꂽ�̈�̕�����
 *						���M�� free() �����
 *	TODO
 *		CBSendStart() @clipboar.c �Ɠ���
 */
BOOL SendMemPasteString(wchar_t *str)
{
	const size_t len = wcslen(str);
	CommTextOutW(&cv, str, (int)len);
	if (ts.LocalEcho > 0) {
		CommTextEchoW(&cv, str, (int)len);
	}

	free(str);
	return TRUE;
}
