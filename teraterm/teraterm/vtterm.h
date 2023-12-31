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

/* TERATERM.EXE, VT terminal emulation */

#pragma once

#include "buffer.h" // for TCharAttr

#ifdef __cplusplus
extern "C" {
#endif

/* prototypes */
void ResetTerminal();
void ResetCharSet();
void ResetKeypadMode(BOOL DisabledModeOnly);
void HideStatusLine();
void ChangeTerminalSize(int Nx, int Ny);
int VTParse();
void FocusReport(BOOL Focus);
BOOL MouseReport(int Event, int Button, int Xpos, int Ypos);
BOOL BracketedPasteMode();
BOOL WheelToCursorMode();
void EndTerm();
void ChangeTerminalID();
void TermPasteStringNoBracket(const wchar_t *str, size_t len);
void TermPasteString(const wchar_t *str, size_t len);
void TermSendStartBracket(void);
void TermSendEndBracket(void);
void TermGetAttr(TCharAttr *attr);
void TermSetAttr(const TCharAttr *attr);
BOOL TermGetInsertMode(void);
void TermSetInsertMode(BOOL insert_mode);
BOOL TermGetAutoWrapMode(void);
void TermSetAutoWrapMode(BOOL auto_wrap_mode);
void TermSetNextDebugMode(void);
void TermSetDebugMode(BYTE mode);

#ifdef __cplusplus
}
#endif
