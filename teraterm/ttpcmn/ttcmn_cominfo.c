/*
 * Copyright (C) 2021- TeraTerm Project
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

#include "codeconv.h"
#include "comportinfo.h"

#define DllExport __declspec(dllexport)
#include "ttcmn_cominfo.h"

/**
 *	COM�|�[�g���
 *
 *	@param[out]	ComPortTable[MAXCOMPORT]
 *	@param[in]	ComPortMax		�e�[�u���̍ő�g�p��(�ő�MAXCOMPORT)
 *	@param[out]	ComPortDesc[MAXCOMPORT]	(4096 in tttypes.h)
 *	@retval	�g�p�\��COM�|�[�g�� (0=�g�p�\�|�[�g���Ȃ�)
 *
 *		MAXCOMPORT = 4096 in tttypes.h
 *		���̒l�̓e�[�u��(COM�|�[�g�̌�)�̍ő吔��\��
 *
 *		����
 *			ComPortDesc�̓P�A���Ȃ��ƃ��������[�N����
 */
int WINAPI DetectComPorts(LPWORD ComPortTable, int ComPortMax, char **ComPortDesc)
{
	int count;
	int i;
	ComPortInfo_t *port_info;
	const ComPortInfo_t *p;

	// �ȑO�m�ۂ��������J������
	for (i = 0; i < ComPortMax; i++) {
		free(ComPortDesc[i]);
		ComPortDesc[i] = NULL;
	}

	port_info = ComPortInfoGet(&count, NULL);
	p = port_info;
	for (i = 0; i < count; i++) {
		ComPortTable[i] = p->port_no;
		ComPortDesc[i] = ToCharW(p->friendly_name);
		p++;
		if (i == ComPortMax - 1) {
			// �e�[�u�������ӂ��
			break;
		}
	}

	ComPortInfoFree(port_info, count);

	return count;
}

/**
 *	COM�|�[�g��񋓂��đ��݂��邩�`�F�b�N����
 *
 *	@param[in]	ComPort		�`�F�b�N����|�[�g�ԍ�
 *	@retval	-1	error(���݂͎g�p����Ă��Ȃ�)
 *	@retval	0	NOT FOUND;
 *	@retval	1	FOUND
 */
int WINAPI CheckComPort(WORD ComPort)
{
	int count;
	int i;
	ComPortInfo_t *port_info = ComPortInfoGet(&count, NULL);
	const ComPortInfo_t *p = port_info;
	int found = 0;
	for (i = 0; i < count; i++) {
		if (ComPort == p->port_no) {
			found = 1;
			break;
		}
		p++;
	}

	ComPortInfoFree(port_info, count);

	return found;
}
