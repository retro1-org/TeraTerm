/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2007- TeraTerm Project
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

/* Constants, types for file transfer */
#pragma once

#define PROTO_KMT 1
#define PROTO_XM  2
#define PROTO_ZM  3
#define PROTO_BP  4
#define PROTO_QV  5
#define PROTO_YM  6

#define OpLog      1
#define OpSendFile 2
#define OpKmtRcv   3
#define OpKmtGet   4
#define OpKmtSend  5
#define OpKmtFin   6
#define OpXRcv     7
#define OpXSend    8
#define OpZRcv     9
#define OpZSend    10
#define OpBPRcv    11
#define OpBPSend   12
#define OpQVRcv    13
#define OpQVSend   14
#define OpYRcv     15
#define OpYSend    16

#define TitLog      "Log"
#define TitSendFile "Send file"
#define TitKmtRcv   "Kermit Receive"
#define TitKmtGet   "Kermit Get"
#define TitKmtSend  "Kermit Send"
#define TitKmtFin   "Kermit Finish"
#define TitXRcv     "XMODEM Receive"
#define TitXSend    "XMODEM Send"
#define TitYRcv     "YMODEM Receive"
#define TitYSend    "YMODEM Send"
#define TitZRcv     "ZMODEM Receive"
#define TitZSend    "ZMODEM Send"
#define TitBPRcv    "B-Plus Receive"
#define TitBPSend   "B-Plus Send"
#define TitQVRcv    "Quick-VAN Receive"
#define TitQVSend   "Quick-VAN Send"
