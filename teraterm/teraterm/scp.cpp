/*
 * (C) 2022- TeraTerm Project
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
 *	TODO
 *	- unicode(wchar_t) filename
 *	- init()/uninit() per ssh connect/disconnect
 */

#include <windows.h>

#include "scp.h"

typedef int (CALLBACK *PSSH_start_scp)(char *, char *);
typedef int (CALLBACK * PSSH_scp_sending_status)(void);

static HMODULE h = NULL;
static PSSH_start_scp start_scp = NULL;
static PSSH_start_scp receive_file = NULL;
static PSSH_scp_sending_status scp_sending_status = NULL;

/**
 * @brief SCP�֐��̃A�h���X���擾
 * @retval TRUE ok
 * @retval FALSE dll���Ȃ�/dll��scp���M�ɑΉ����Ă��Ȃ�
 */
static BOOL ScpInit(void)
{
	if (h == NULL) {
		if ((h = GetModuleHandle("ttxssh.dll")) == NULL) {
			return FALSE;
		}
	}

	if (start_scp == NULL) {
		start_scp = (PSSH_start_scp)GetProcAddress(h, "TTXScpSendfile");
		if (start_scp == NULL) {
			return FALSE;
		}
	}
	if (scp_sending_status == NULL) {
		scp_sending_status = (PSSH_scp_sending_status)GetProcAddress(h, "TTXScpSendingStatus");
		if (scp_sending_status == NULL) {
			return FALSE;
		}
	}

	if (receive_file == NULL) {
		receive_file = (PSSH_start_scp)GetProcAddress(h, "TTXScpReceivefile");
		if (receive_file == NULL) {
			return FALSE;
		}
	}

	return TRUE;
}

/**
 *	�t�@�C���𑗐M����
 */
BOOL ScpSend(const char *local, const char *remote)
{
	if (start_scp == NULL) {
		ScpInit();
	}
	if (start_scp == NULL) {
		return FALSE;
	}
	BOOL r = (BOOL)start_scp((char*)local, (char*)remote);
	return r;
}

/**
 *	�t�@�C�����M���
 *	@retval	FALSE	���M���Ă��Ȃ�
 *	@retval	TRUE	���M��
 */
BOOL ScpGetStatus(void)
{
	if (scp_sending_status == NULL) {
		ScpInit();
	}
	if (scp_sending_status == NULL) {
		return FALSE;
	}
	BOOL r = (BOOL)scp_sending_status();
	return r;
}

/**
 *	�t�@�C������M����
 */
BOOL ScpReceive(const char *remotefile, const char *localfile)
{
	if (receive_file == NULL) {
		ScpInit();
	}
	if (receive_file == NULL) {
		return FALSE;
	}
	BOOL r = (BOOL)receive_file((char*)remotefile, (char*)localfile);
	return r;
}
