/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2021- TeraTerm Project
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

/* Language */
typedef enum {
	IdEnglish  = 1,
	IdJapanese,
	IdRussian,
	IdKorean,  	//HKS
	IdUtf8,
	IdChinese,
	IdDebug,
	IdLangMax,
} IdLanguage;

  /* Kanji Code ID */
  /*  ts.KanjiCode, ts.KanjiCodeSend の値 */

// ts.Language == IdEnglish
typedef enum {
	IdISO8859_1,
	IdISO8859_2,
	IdISO8859_3,
	IdISO8859_4,
	IdISO8859_5,
	IdISO8859_6,
	IdISO8859_7,
	IdISO8859_8,
	IdISO8859_9,
	IdISO8859_10,
	IdISO8859_11,
	IdISO8859_13,
	IdISO8859_14,
	IdISO8859_15,
	IdISO8859_16,
} IdKanjiCode;

// ts.Language == IdJapanese
#define IdSJIS  1
#define IdEUC   2
#define IdJIS   3
#define IdUTF8  4		// IdUtf8 (小文字)は ts.Language 用

// ts.Language == IdRussian
// Russian code sets
#define IdWindows 1
#define IdKOI8    2
#define Id866     3
#define IdISO     4

// ts.Language == IdKorean
// Korean
#define	IdKoreanCP949 1		// CP949, KS5601

// ts.Language == IdChinese
// China
#define	IdCnGB2312		1	// 1 CP936, GB2312
#define	IdCnBig5		2	// 2 CP950, Big5

  /* KanjiIn modes */
#define IdKanjiInA 1
#define IdKanjiInB 2
  /* KanjiOut modes */
#define IdKanjiOutB 1
#define IdKanjiOutJ 2
#define IdKanjiOutH 3
