/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2005-2019 TeraTerm Project
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

/* TTFILE.DLL, file transfer, VT window printing */
#include "teraterm.h"
#include "tttypes.h"
#include "ttftypes.h"
#include <direct.h>
#include <commdlg.h>
#include <string.h>

#include "ttlib.h"
#include "ftlib.h"
#include "dlglib.h"
#include "kermit.h"
#include "xmodem.h"
#include "ymodem.h"
#include "zmodem.h"
#include "bplus.h"
#include "quickvan.h"

#include "filesys_proto.h"
#include "ttfile_proto.h"

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <assert.h>

void _ProtoInit(int Proto, PFileVarProto fv, PCHAR pv, PComVar cv, PTTSet ts)
{
	switch (Proto) {
	case PROTO_KMT:
		KmtInit(fv,(PKmtVar)pv,cv,ts);
		break;
	case PROTO_XM:
		fv->Init(fv,cv,ts);
		break;
	case PROTO_YM:
		YInit(fv,(PYVar)pv,cv,ts);
		break;
	case PROTO_ZM:
		ZInit(fv,(PZVar)pv,cv,ts);
		break;
	case PROTO_BP:
		BPInit(fv,(PBPVar)pv,cv,ts);
		break;
	case PROTO_QV:
		QVInit(fv,(PQVVar)pv,cv,ts);
		break;
	}
}

BOOL _ProtoParse(int Proto, PFileVarProto fv, PCHAR pv, PComVar cv)
{
	BOOL Ok;

	Ok = FALSE;
	switch (Proto) {
	case PROTO_KMT:
		Ok = KmtReadPacket(fv,(PKmtVar)pv,cv);
		break;
	case PROTO_XM:
		Ok = fv->Parse(fv, cv);
		break;
	case PROTO_YM:
		switch (((PYVar)pv)->YMode) {
		case IdYReceive:
			Ok = YReadPacket(fv,(PYVar)pv,cv);
			break;
		case IdYSend:
			Ok = YSendPacket(fv,(PYVar)pv,cv);
			break;
		}
		break;
	case PROTO_ZM:
		Ok = ZParse(fv,(PZVar)pv,cv);
		break;
	case PROTO_BP:
		Ok = BPParse(fv,(PBPVar)pv,cv);
		break;
	case PROTO_QV:
		switch (((PQVVar)pv)->QVMode) {
		case IdQVReceive:
			Ok = QVReadPacket(fv,(PQVVar)pv,cv);
			break;
		case IdQVSend:
			Ok = QVSendPacket(fv,(PQVVar)pv,cv);
			break;
		}
		break;
	}
	return Ok;
}

void _ProtoTimeOutProc(int Proto, PFileVarProto fv, PCHAR pv, PComVar cv)
{
	switch (Proto) {
	case PROTO_KMT:
		KmtTimeOutProc(fv,(PKmtVar)pv,cv);
		break;
	case PROTO_XM:
		fv->TimeOutProc(fv,cv);
		break;
	case PROTO_YM:
		YTimeOutProc(fv,(PYVar)pv,cv);
		break;
	case PROTO_ZM:
		ZTimeOutProc(fv,(PZVar)pv,cv);
		break;
	case PROTO_BP:
		BPTimeOutProc(fv,(PBPVar)pv,cv);
		break;
	case PROTO_QV:
		QVTimeOutProc(fv,(PQVVar)pv,cv);
		break;
	}
}

BOOL _ProtoCancel(int Proto, PFileVarProto fv, PCHAR pv, PComVar cv)
{
	switch (Proto) {
	case PROTO_KMT:
		KmtCancel(fv,(PKmtVar)pv,cv);
		break;
	case PROTO_XM:
		fv->Cancel(fv,cv);
		break;
	case PROTO_YM:
		YCancel(fv, (PYVar)pv,cv);
		break;
	case PROTO_ZM:
		ZCancel((PZVar)pv);
		break;
	case PROTO_BP:
		if (((PBPVar)pv)->BPState != BP_Failure) {
			BPCancel((PBPVar)pv);
			return FALSE;
		}
		break;
	case PROTO_QV:
		QVCancel(fv,(PQVVar)pv,cv);
		break;
	}
	return TRUE;
}

int _ProtoSetOpt(PFileVarProto fv, int request, ...)
{
	va_list ap;
	va_start(ap, request);
	int r = fv->SetOptV(fv, request, ap);
	va_end(ap);
	return r;
}
