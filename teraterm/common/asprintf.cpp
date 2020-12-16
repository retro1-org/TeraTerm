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

#include <stdio.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "asprintf.h"

/**
 *	�̈���m�ۂ��āA��������t�H�[�}�b�g���āA�|�C���^�Ԃ�
 *	�s�v�ɂȂ����� free() ���邱��
 *	@retval	�o�͕�����(�I�[��'\0'���܂�)
 *			�t�H�[�}�b�g�����񂪂��������Ƃ���"EILSEQ"
 *			���̑��G���[���� -1
 */
int vasprintf(char **strp, const char *fmt, va_list ap)
{
	char *tmp_ptr = NULL;
	size_t tmp_size = 128;
	for(;;) {
		int len;
		int err;
		tmp_ptr = (char *)realloc(tmp_ptr, tmp_size);
		assert(tmp_ptr != NULL);
		if (tmp_ptr == NULL) {
			*strp = NULL;
			return -1;
		}
		len = _vsnprintf_s(tmp_ptr, tmp_size, _TRUNCATE, fmt, ap);
		if (len != -1) {
			len++;	// +1 for '\0' (terminator)
			tmp_ptr = (char *)realloc(tmp_ptr, len);
			*strp = tmp_ptr;
			return len;
		}
		err = errno;
		if (err == EILSEQ) {
			*strp = _strdup("EILSEQ");
			return 7;
		}
		tmp_size *= 2;
	}
}

/**
 *	�̈���m�ۂ��āA��������t�H�[�}�b�g���āA�|�C���^�Ԃ�
 *	�s�v�ɂȂ����� free() ���邱��
 *	@retval	�o�͕�����(�I�[��L'\0'���܂�)
 *			�t�H�[�}�b�g�����񂪂��������Ƃ���L"EILSEQ"
 *			���̑��G���[���� -1
 */
int vaswprintf(wchar_t **strp, const wchar_t *fmt, va_list ap)
{
	wchar_t *tmp_ptr = NULL;
	size_t tmp_size = 128;
	for(;;) {
		int len;
		int err;
		tmp_ptr = (wchar_t *)realloc(tmp_ptr, sizeof(wchar_t) * tmp_size);
		assert(tmp_ptr != NULL);
		len = _vsnwprintf_s(tmp_ptr, tmp_size, _TRUNCATE, fmt, ap);
		if (len != -1) {
			len++;	// +1 for '\0' (terminator)
			tmp_ptr = (wchar_t *)realloc(tmp_ptr, sizeof(wchar_t) * len);
			*strp = tmp_ptr;
			return len;
		}
		err = errno;
		if (err == EILSEQ) {
			*strp = _wcsdup(L"EILSEQ");
			return 7;
		}
		tmp_size *= 2;
	}
}

/**
 *	�̈���m�ۂ��āA��������t�H�[�}�b�g���āA�|�C���^�Ԃ�
 *	�s�v�ɂȂ����� free() ���邱��
 *	@retval	�o�͕�����(�I�[��'\0'���܂�)
 *			�t�H�[�}�b�g�����񂪂��������Ƃ���"EILSEQ"
 *			���̑��G���[���� -1
 */
int asprintf(char **strp, const char *fmt, ...)
{
	int r;
	va_list ap;
	va_start(ap, fmt);
	r = vasprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}

/**
 *	�̈���m�ۂ��āA��������t�H�[�}�b�g���āA�|�C���^�Ԃ�
 *	�s�v�ɂȂ����� free() ���邱��
 *	@retval	�o�͕�����(�I�[��'\0'���܂�)
 *			�t�H�[�}�b�g�����񂪂��������Ƃ���L"EILSEQ"
 *			���̑��G���[���� -1
 */
int aswprintf(wchar_t **strp, const wchar_t *fmt, ...)
{
	int r;
	va_list ap;
	va_start(ap, fmt);
	r = vaswprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}
