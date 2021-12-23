/*
 * Copyright (C) 1994-1998 T. Teranishi
 * (C) 2006- TeraTerm Project
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

/* misc. routines  */

#include <sys/stat.h>
#include <sys/utime.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <shlobj.h>
#include <ctype.h>
#include <mbctype.h>	// for _ismbblead
#include <assert.h>

#include "teraterm.h"
#include "tttypes.h"
#include "compat_win.h"
#include "codeconv.h"

/* OS version with GetVersionEx(*1)

                dwMajorVersion   dwMinorVersion    dwPlatformId
Windows95       4                0                 VER_PLATFORM_WIN32_WINDOWS
Windows98       4                10                VER_PLATFORM_WIN32_WINDOWS
WindowsMe       4                90                VER_PLATFORM_WIN32_WINDOWS
WindowsNT4.0    4                0                 VER_PLATFORM_WIN32_NT
Windows2000     5                0                 VER_PLATFORM_WIN32_NT
WindowsXP       5                1                 VER_PLATFORM_WIN32_NT
WindowsXPx64    5                2                 VER_PLATFORM_WIN32_NT
WindowsVista    6                0                 VER_PLATFORM_WIN32_NT
Windows7        6                1                 VER_PLATFORM_WIN32_NT
Windows8        6                2                 VER_PLATFORM_WIN32_NT
Windows8.1(*2)  6                2                 VER_PLATFORM_WIN32_NT
Windows8.1(*3)  6                3                 VER_PLATFORM_WIN32_NT
Windows10(*2)   6                2                 VER_PLATFORM_WIN32_NT
Windows10(*3)   10               0                 VER_PLATFORM_WIN32_NT

(*1) GetVersionEx()�� c4996 warning �ƂȂ�̂́AVS2013(_MSC_VER=1800) ����ł��B
(*2) manifest�� supportedOS Id ��ǉ����Ă��Ȃ��B
(*3) manifest�� supportedOS Id ��ǉ����Ă���B
*/

// for isInvalidFileNameChar / replaceInvalidFileNameChar
static char *invalidFileNameChars = "\\/:*?\"<>|";

// �t�@�C���Ɏg�p���邱�Ƃ��ł��Ȃ�����
// cf. Naming Files, Paths, and Namespaces
//     http://msdn.microsoft.com/en-us/library/aa365247.aspx
// (2013.3.9 yutaka)
static char *invalidFileNameStrings[] = {
	"AUX", "CLOCK$", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
	"CON", "CONFIG$", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
	"NUL", "PRN",
	".", "..",
	NULL
};


// for b64encode/b64decode
static char *b64enc_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char b64dec_table[] = {
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
   -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
   -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

void b64encode(PCHAR d, int dsize, PCHAR s, int len)
{
	unsigned int b = 0;
	int state = 0;
	unsigned char *src, *dst;

	src = (unsigned char *)s;
	dst = (unsigned char *)d;

	if (dsize == 0 || dst == NULL || src == NULL) {
		return;
	}
	if (dsize < 5) {
		*dst = 0;
		return;
	}

	while (len > 0) {
		b = (b << 8) | *src++;
		len--;
		state++;

		if (state == 3) {
			*dst++ = b64enc_table[(b>>18) & 0x3f];
			*dst++ = b64enc_table[(b>>12) & 0x3f];
			*dst++ = b64enc_table[(b>>6) & 0x3f];
			*dst++ = b64enc_table[b & 0x3f];
			dsize -= 4;
			state = 0;
			b = 0;
			if (dsize < 5)
				break;
		}
	}

	if (dsize >= 5) {
		if (state == 1) {
			*dst++ = b64enc_table[(b>>2) & 0x3f];
			*dst++ = b64enc_table[(b<<4) & 0x3f];
			*dst++ = '=';
			*dst++ = '=';
		}
		else if (state == 2) {
			*dst++ = b64enc_table[(b>>10) & 0x3f];
			*dst++ = b64enc_table[(b>>4) & 0x3f];
			*dst++ = b64enc_table[(b<<2) & 0x3f];
			*dst++ = '=';
		}
	}

	*dst = 0;
	return;
}

int b64decode(PCHAR dst, int dsize, PCHAR src)
{
	unsigned int b = 0;
	char c;
	int len = 0, state = 0;

	if (src == NULL || dst == NULL || dsize == 0)
		return -1;

	while (1) {
		if (isspace(*src)) {
			src++;
			continue;
		}

		if ((c = b64dec_table[*src++]) == -1)
			break;

		b = (b << 6) | c;
		state++;

		if (state == 4) {
			if (dsize > len)
				dst[len++] = (b >> 16) & 0xff;
			if (dsize > len)
				dst[len++] = (b >> 8) & 0xff;
			if (dsize > len)
				dst[len++] = b & 0xff;
			state = 0;
			if (dsize <= len)
				break;
		}
	}

	if (state == 2) {
		b <<= 4;
		if (dsize > len)
			dst[len++] = (b >> 8) & 0xff;
	}
	else if (state == 3) {
		b <<= 6;
		if (dsize > len)
			dst[len++] = (b >> 16) & 0xff;
		if (dsize > len)
			dst[len++] = (b >> 8) & 0xff;
	}
	if (len < dsize) {
		dst[len] = 0;
	}
	else {
		dst[dsize-1] = 0;
	}
	return len;
}

/**
 *	�t�@�C����(�p�X��)����͂���
 *	_splitpath�̃p�X�ƃt�@�C����������
 *
 *	@param[in]	PathName	�t�@�C�����A�t���p�X
 *	@param[out]	DirLen		�����̃X���b�V�����܂ރf�B���N�g���p�X��
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@param[out]	FNPos		�t�@�C�����ւ�index
 *							&PathName[FNPos] ���t�@�C����
 *							NULL�̂Ƃ��l��Ԃ��Ȃ�
 *	@retval		FALSE		PathName���s��
 */
BOOL GetFileNamePos(const char *PathName, int *DirLen, int *FNPos)
{
	BYTE b;
	const char *Ptr;
	const char *DirPtr;
	const char *FNPtr;
	const char *PtrOld;

	if (DirLen != NULL) *DirLen = 0;
	if (FNPos != NULL) *FNPos = 0;

	if (PathName==NULL)
		return FALSE;

	if ((strlen(PathName)>=2) && (PathName[1]==':'))
		Ptr = &PathName[2];
	else
		Ptr = PathName;
	if (Ptr[0]=='\\' || Ptr[0]=='/')
		Ptr = CharNext(Ptr);

	DirPtr = Ptr;
	FNPtr = Ptr;
	while (Ptr[0]!=0) {
		b = Ptr[0];
		PtrOld = Ptr;
		Ptr = CharNext(Ptr);
		switch (b) {
			case ':':
				return FALSE;
			case '/':	/* FALLTHROUGH */
			case '\\':
				DirPtr = PtrOld;
				FNPtr = Ptr;
				break;
		}
	}
	if (DirLen != NULL) *DirLen = DirPtr-PathName;
	if (FNPos != NULL) *FNPos = FNPtr-PathName;
	return TRUE;
}

BOOL ExtractFileName(PCHAR PathName, PCHAR FileName, int destlen)
{
	int i, j;

	if (FileName==NULL)
		return FALSE;
	if (! GetFileNamePos(PathName,&i,&j))
		return FALSE;
	strncpy_s(FileName,destlen,&PathName[j],_TRUNCATE);
	return (strlen(FileName)>0);
}

BOOL ExtractDirName(PCHAR PathName, PCHAR DirName)
{
	int i, j;

	if (DirName==NULL)
		return FALSE;
	if (! GetFileNamePos(PathName,&i,&j))
		return FALSE;
	memmove(DirName,PathName,i); // do not use memcpy
	DirName[i] = 0;
	return TRUE;
}

/* fit a filename to the windows-filename format */
/* FileName must contain filename part only. */
void FitFileName(PCHAR FileName, int destlen, const char *DefExt)
{
	int i, j, NumOfDots;
	char Temp[MAX_PATH];
	BYTE b;

	NumOfDots = 0;
	i = 0;
	j = 0;
	/* filename started with a dot is illeagal */
	if (FileName[0]=='.') {
		Temp[0] = '_';  /* insert an underscore char */
		j++;
	}

	do {
		b = FileName[i];
		i++;
		if (b=='.')
			NumOfDots++;
		if ((b!=0) &&
		    (j < MAX_PATH-1)) {
			Temp[j] = b;
			j++;
		}
	} while (b!=0);
	Temp[j] = 0;

	if ((NumOfDots==0) &&
	    (DefExt!=NULL)) {
		/* add the default extension */
		strncat_s(Temp,sizeof(Temp),DefExt,_TRUNCATE);
	}

	strncpy_s(FileName,destlen,Temp,_TRUNCATE);
}

// Append a slash to the end of a path name
void AppendSlash(PCHAR Path, int destlen)
{
	if (strcmp(CharPrev((LPCTSTR)Path,
	           (LPCTSTR)(&Path[strlen(Path)])),
	           "\\") != 0) {
		strncat_s(Path,destlen,"\\",_TRUNCATE);
	}
}

// Delete slashes at the end of a path name
void DeleteSlash(PCHAR Path)
{
	size_t i;
	for (i=strlen(Path)-1; i>=0; i--) {
		if (i ==0 && Path[i] == '\\' ||
		    Path[i] == '\\' && !_ismbblead(Path[i-1])) {
			Path[i] = '\0';
		}
		else {
			break;
		}
	}
}

void Str2Hex(PCHAR Str, PCHAR Hex, int Len, int MaxHexLen, BOOL ConvSP)
{
	BYTE b, low;
	int i, j;

	if (ConvSP)
		low = 0x20;
	else
		low = 0x1F;

	j = 0;
	for (i=0; i<=Len-1; i++) {
		b = Str[i];
		if ((b!='$') && (b>low) && (b<0x7f)) {
			if (j < MaxHexLen) {
				Hex[j] = b;
				j++;
			}
		}
		else {
			if (j < MaxHexLen-2) {
				Hex[j] = '$';
				j++;
				if (b<=0x9f) {
					Hex[j] = (char)((b >> 4) + 0x30);
				}
				else {
					Hex[j] = (char)((b >> 4) + 0x37);
				}
				j++;
				if ((b & 0x0f) <= 0x9) {
					Hex[j] = (char)((b & 0x0f) + 0x30);
				}
				else {
					Hex[j] = (char)((b & 0x0f) + 0x37);
				}
				j++;
			}
		}
	}
	Hex[j] = 0;
}

BYTE ConvHexChar(BYTE b)
{
	if ((b>='0') && (b<='9')) {
		return (b - 0x30);
	}
	else if ((b>='A') && (b<='F')) {
		return (b - 0x37);
	}
	else if ((b>='a') && (b<='f')) {
		return (b - 0x57);
	}
	else {
		return 0;
	}
}

int Hex2Str(PCHAR Hex, PCHAR Str, int MaxLen)
{
	BYTE b, c;
	int i, imax, j;

	j = 0;
	imax = strlen(Hex);
	i = 0;
	while ((i < imax) && (j<MaxLen)) {
		b = Hex[i];
		if (b=='$') {
			i++;
			if (i < imax) {
				c = Hex[i];
			}
			else {
				c = 0x30;
			}
			b = ConvHexChar(c) << 4;
			i++;
			if (i < imax) {
				c = Hex[i];
			}
			else {
				c = 0x30;
			}
			b = b + ConvHexChar(c);
		};

		Str[j] = b;
		j++;
		i++;
	}
	if (j<MaxLen) {
		Str[j] = 0;
	}

	return j;
}

BOOL DoesFileExist(const char *FName)
{
	// check if a file exists or not
	// �t�H���_�܂��̓t�@�C��������� TRUE ��Ԃ�
	struct _stat st;

	return (_stat(FName,&st)==0);
}

/**
 *	check if a folder exists or not
 *	�}�N���݊����̂���
 *	DoesFileExist() �͏]���ʂ�t�H���_�܂��̓t�@�C��������� TRUE ��Ԃ�
 *  DoesFolderExist() �̓t�H���_������ꍇ�̂� TRUE ��Ԃ��B
 */
BOOL DoesFolderExistW(const wchar_t *FName)
{
	const DWORD r = GetFileAttributesW(FName);
	if (r == INVALID_FILE_ATTRIBUTES) {
		return FALSE;
	}
	if (r & FILE_ATTRIBUTE_DIRECTORY) {
		return TRUE;
	}
	return FALSE;
}

BOOL DoesFolderExist(const char *FName)
{
	wchar_t *FNameW = ToWcharA(FName);
	BOOL r = DoesFolderExistW(FNameW);
	free(FNameW);
	return r;
}

long GetFSize(const char *FName)
{
	unsigned long long size = GetFSize64A(FName);
	return (long)size;
}

long GetFMtime(const char *FName)
{
	struct _stat st;

	if (_stat(FName,&st)==-1) {
		return 0;
	}
	return (long)st.st_mtime;
}

BOOL SetFMtime(const char *FName, DWORD mtime)
{
	struct _utimbuf filetime;

	filetime.actime = mtime;
	filetime.modtime = mtime;
	return _utime(FName, &filetime);
}

void uint2str(UINT i, PCHAR Str, int destlen, int len)
{
	char Temp[20];

	memset(Temp, 0, sizeof(Temp));
	_snprintf_s(Temp,sizeof(Temp),_TRUNCATE,"%u",i);
	Temp[len] = 0;
	strncpy_s(Str,destlen,Temp,_TRUNCATE);
}

void QuoteFName(PCHAR FName)
{
	int i;

	if (FName[0]==0) {
		return;
	}
	if (strchr(FName,' ')==NULL) {
		return;
	}
	i = strlen(FName);
	memmove(&(FName[1]),FName,i);
	FName[0] = '\"';
	FName[i+1] = '\"';
	FName[i+2] = 0;
}

// �t�@�C�����Ɏg�p�ł��Ȃ��������܂܂�Ă��邩�m���߂� (2006.8.28 maya)
int isInvalidFileNameChar(const char *FName)
{
	int i, len;
	char **p, c;

	// �`�F�b�N�Ώۂ̕��������������B(2013.3.9 yutaka)
	p = invalidFileNameStrings;
	while (*p) {
		if (_strcmpi(FName, *p) == 0) {
			return 1;  // Invalid
		}
		p++;
	}

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (_ismbblead(FName[i])) {
			i++;
			continue;
		}
		if ((FName[i] >= 0 && FName[i] < ' ') || strchr(invalidFileNameChars, FName[i])) {
			return 1;
		}
	}

	// �t�@�C�����̖����Ƀs���I�h����ы󔒂�NG�B
	c = FName[len - 1];
	if (c == '.' || c == ' ')
		return 1;

	return 0;
}

// �t�@�C�����Ɏg�p�ł��Ȃ������� c �ɒu��������
// c �� 0 ���w�肵���ꍇ�͕������폜����
void replaceInvalidFileNameChar(PCHAR FName, unsigned char c)
{
	int i, j=0, len;

	if ((c >= 0 && c < ' ') || strchr(invalidFileNameChars, c)) {
		c = 0;
	}

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (_ismbblead(FName[i])) {
			FName[j++] = FName[i];
			FName[j++] = FName[++i];
			continue;
		}
		if ((FName[i] >= 0 && FName[i] < ' ') || strchr(invalidFileNameChars, FName[i])) {
			if (c) {
				FName[j++] = c;
			}
		}
		else {
			FName[j++] = FName[i];
		}
	}
	FName[j] = 0;
}

// strftime �ɓn���Ȃ��������܂܂�Ă��邩�m���߂� (2006.8.28 maya)
int isInvalidStrftimeChar(PCHAR FName)
{
	int i, len, p;

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (FName[i] == '%') {
			if (FName[i+1] != 0) {
				p = i+1;
				if (FName[i+2] != 0 && FName[i+1] == '#') {
					p = i+2;
				}
				switch (FName[p]) {
					case 'a':
					case 'A':
					case 'b':
					case 'B':
					case 'c':
					case 'd':
					case 'H':
					case 'I':
					case 'j':
					case 'm':
					case 'M':
					case 'p':
					case 'S':
					case 'U':
					case 'w':
					case 'W':
					case 'x':
					case 'X':
					case 'y':
					case 'Y':
					case 'z':
					case 'Z':
					case '%':
						i = p;
						break;
					default:
						return 1;
				}
			}
			else {
				// % �ŏI����Ă���ꍇ�̓G���[�Ƃ���
				return 1;
			}
		}
	}

	return 0;
}

// strftime �ɓn���Ȃ��������폜���� (2006.8.28 maya)
void deleteInvalidStrftimeChar(PCHAR FName)
{
	int i, j=0, len, p;

	len = strlen(FName);
	for (i=0; i<len; i++) {
		if (FName[i] == '%') {
			if (FName[i+1] != 0) {
				p = i+1;
				if (FName[i+2] != 0 && FName[i+1] == '#') {
					p = i+2;
				}
				switch (FName[p]) {
					case 'a':
					case 'A':
					case 'b':
					case 'B':
					case 'c':
					case 'd':
					case 'H':
					case 'I':
					case 'j':
					case 'm':
					case 'M':
					case 'p':
					case 'S':
					case 'U':
					case 'w':
					case 'W':
					case 'x':
					case 'X':
					case 'y':
					case 'Y':
					case 'z':
					case 'Z':
					case '%':
						FName[j] = FName[i]; // %
						j++;
						i++;
						if (p-i == 2) {
							FName[j] = FName[i]; // #
							j++;
							i++;
						}
						FName[j] = FName[i];
						j++;
						break;
					default:
						i++; // %
						if (p-i == 2) {
							i++; // #
						}
				}
			}
			// % �ŏI����Ă���ꍇ�̓R�s�[���Ȃ�
		}
		else {
			FName[j] = FName[i];
			j++;
		}
	}

	FName[j] = 0;
}

// �t���p�X����A�t�@�C���������݂̂� strftime �ŕϊ����� (2006.8.28 maya)
void ParseStrftimeFileName(PCHAR FName, int destlen)
{
	char filename[MAX_PATH];
	char dirname[MAX_PATH];
	char buf[MAX_PATH];
	char *c;
	time_t time_local;
	struct tm tm_local;

	// �t�@�C���������݂̂� flename �Ɋi�[
	ExtractFileName(FName, filename ,sizeof(filename));

	// strftime �Ɏg�p�ł��Ȃ��������폜
	deleteInvalidStrftimeChar(filename);

	// ���ݎ������擾
	time(&time_local);
	localtime_s(&tm_local, &time_local);

	// ����������ɕϊ�
	if (strftime(buf, sizeof(buf), filename, &tm_local) == 0) {
		strncpy_s(buf, sizeof(buf), filename, _TRUNCATE);
	}

	// �t�@�C�����Ɏg�p�ł��Ȃ��������폜
	deleteInvalidFileNameChar(buf);

	c = strrchr(FName, '\\');
	if (c != NULL) {
		ExtractDirName(FName, dirname);
		strncpy_s(FName, destlen, dirname, _TRUNCATE);
		AppendSlash(FName,destlen);
		strncat_s(FName, destlen, buf, _TRUNCATE);
	}
	else { // "\"���܂܂Ȃ�(�t���p�X�łȂ�)�ꍇ�ɑΉ� (2006.11.30 maya)
		strncpy_s(FName, destlen, buf, _TRUNCATE);
	}
}

void ConvFName(const char *HomeDir, PCHAR Temp, int templen, const char *DefExt, PCHAR FName, int destlen)
{
	// destlen = sizeof FName
	int DirLen, FNPos;

	FName[0] = 0;
	if ( ! GetFileNamePos(Temp,&DirLen,&FNPos) ) {
		return;
	}
	FitFileName(&Temp[FNPos],templen - FNPos,DefExt);
	if ( DirLen==0 ) {
		strncpy_s(FName,destlen,HomeDir,_TRUNCATE);
		AppendSlash(FName,destlen);
	}
	strncat_s(FName,destlen,Temp,_TRUNCATE);
}

// "\n" �����s�ɕϊ����� (2006.7.29 maya)
// "\t" ���^�u�ɕϊ����� (2006.11.6 maya)
void RestoreNewLine(PCHAR Text)
{
	int i, j=0, size=strlen(Text);
	char *buf = (char *)_alloca(size+1);

	memset(buf, 0, size+1);
	for (i=0; i<size; i++) {
		if (Text[i] == '\\' && i<size ) {
			switch (Text[i+1]) {
				case '\\':
					buf[j] = '\\';
					i++;
					break;
				case 'n':
					buf[j] = '\n';
					i++;
					break;
				case 't':
					buf[j] = '\t';
					i++;
					break;
				case '0':
					buf[j] = '\0';
					i++;
					break;
				default:
					buf[j] = '\\';
			}
			j++;
		}
		else {
			buf[j] = Text[i];
			j++;
		}
	}
	/* use memcpy to copy with '\0' */
	memcpy(Text, buf, size);
}

void GetDownloadFolder(char *dest, int destlen)
{
	wchar_t *download = GetDownloadFolderW();
	WideCharToACP_t(download, dest, destlen);
	free(download);
}

/*
 *	UILanguageFile�̃t���p�X���擾����
 *
 *	@param[in]		HomeDir					exe,dll�̑��݂���t�H���_ GetHomeDir()�Ŏ擾�ł���
 *	@param[in]		UILanguageFileRel		lng�t�@�C���AHomeDir����̑��΃p�X
 *	@param[in,out]	UILanguageFileFull		lng�t�@�C��ptr�A�t���p�X
 *	@param[in]		UILanguageFileFullLen	lng�t�@�C��len�A�t���p�X
 */
void GetUILanguageFileFull(const char *HomeDir, const char *UILanguageFileRel,
						   char *UILanguageFileFull, size_t UILanguageFileFullLen)
{
	char CurDir[MAX_PATH];

	/* Get UILanguageFile Full Path */
	GetCurrentDirectory(sizeof(CurDir), CurDir);
	SetCurrentDirectory(HomeDir);
	_fullpath(UILanguageFileFull, UILanguageFileRel, UILanguageFileFullLen);
	SetCurrentDirectory(CurDir);
}

// �w�肵���G���g���� teraterm.ini ����ǂݎ�� (2009.3.23 yutaka)
void GetOnOffEntryInifile(char *entry, char *buf, int buflen)
{
	wchar_t *HomeDirW;
	wchar_t Temp[MAX_PATH];
	wchar_t *SetupFName;
	wchar_t *entryW = ToWcharA(entry);
	char * TempA;

	/* Get home directory */
	HomeDirW = GetHomeDirW(NULL);

	/* Get SetupFName */
	SetupFName = GetDefaultSetupFNameW(HomeDirW);

	/* Get LanguageFile name */
	GetPrivateProfileStringW(L"Tera Term", entryW, L"off",
							 Temp, _countof(Temp), SetupFName);

	TempA = ToCharW(Temp);
	strncpy_s(buf, buflen, TempA, _TRUNCATE);
	free(HomeDirW);
	free(SetupFName);
	free(TempA);
}

void get_lang_msgW(const char *key, wchar_t *buf, int buf_len, const wchar_t *def, const char *iniFile)
{
	GetI18nStrW("Tera Term", key, buf, buf_len, def, iniFile);
}

void get_lang_msg(const char *key, PCHAR buf, int buf_len, const char *def, const char *iniFile)
{
	GetI18nStr("Tera Term", key, buf, buf_len, def, iniFile);
}

int get_lang_font(const char *key, HWND dlg, PLOGFONT logfont, HFONT *font, const char *iniFile)
{
	if (GetI18nLogfont("Tera Term", key, logfont,
	                   GetDeviceCaps(GetDC(dlg),LOGPIXELSY),
	                   iniFile) == FALSE) {
		return FALSE;
	}

	if ((*font = CreateFontIndirect(logfont)) == NULL) {
		return FALSE;
	}

	return TRUE;
}

BOOL doSelectFolder(HWND hWnd, char *path, int pathlen, const char *def, const char *msg)
{
	wchar_t *defW = ToWcharA(def);
	wchar_t *msgW = ToWcharA(msg);
	wchar_t *pathW;
	BOOL r = doSelectFolderW(hWnd, defW, msgW, &pathW);
	if (r == TRUE) {
		WideCharToACP_t(pathW, path, pathlen);
		free(pathW);
	}
	free(msgW);
	free(defW);
	return r;
}

void OutputDebugPrintf(const char *fmt, ...)
{
	char tmp[1024];
	va_list arg;
	va_start(arg, fmt);
	_vsnprintf_s(tmp, sizeof(tmp), _TRUNCATE, fmt, arg);
	va_end(arg);
	OutputDebugStringA(tmp);
}

// OS�� WindowsNT �J�[�l�����ǂ����𔻕ʂ���B
//
// return TRUE:  NT kernel
//        FALSE: Not NT kernel
BOOL IsWindowsNTKernel()
{
	OSVERSIONINFOEXA osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_EQUAL;
	BOOL ret;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;
	dwlConditionMask = _VerSetConditionMask(dwlConditionMask, VER_PLATFORMID, op);
	ret = _VerifyVersionInfoA(&osvi, VER_PLATFORMID, dwlConditionMask);
	return (ret);
}

// OS�� Windows95 ���ǂ����𔻕ʂ���B
BOOL IsWindows95()
{
	return IsWindowsVer(VER_PLATFORM_WIN32_WINDOWS, 4, 0);
}

// OS�� WindowsMe ���ǂ����𔻕ʂ���B
BOOL IsWindowsMe()
{
	return IsWindowsVer(VER_PLATFORM_WIN32_WINDOWS, 4, 90);
}

// OS�� WindowsNT4.0 ���ǂ����𔻕ʂ���B
BOOL IsWindowsNT4()
{
	// VS2013�ȏゾ�� GetVersionEx() ���x���ƂȂ邽�߁AVerifyVersionInfo() ���g���B
	// �������AVS2013�Ńr���h�����v���O�����́A�������� NT4.0 �ł͓��삵�Ȃ����߁A
	// �������� FALSE ��Ԃ��Ă��悢��������Ȃ��B
	return IsWindowsVer(VER_PLATFORM_WIN32_NT, 4, 0);
}

// OS�� Windows2000 ���ǂ����𔻕ʂ���B
BOOL IsWindows2000()
{
	return IsWindowsVer(VER_PLATFORM_WIN32_NT, 5, 0);
}

// OS�� Windows2000 �ȍ~ ���ǂ����𔻕ʂ���B
//
// return TRUE:  2000 or later
//        FALSE: NT4 or earlier
BOOL IsWindows2000OrLater(void)
{
	return IsWindowsVerOrLater(5, 0);
}

// OS�� WindowsVista �ȍ~ ���ǂ����𔻕ʂ���B
//
// return TRUE:  Vista or later
//        FALSE: XP or earlier
BOOL IsWindowsVistaOrLater(void)
{
	return IsWindowsVerOrLater(6, 0);
}

// OS�� Windows7 �ȍ~ ���ǂ����𔻕ʂ���B
//
// return TRUE:  7 or later
//        FALSE: Vista or earlier
BOOL IsWindows7OrLater(void)
{
	return IsWindowsVerOrLater(6, 1);
}

// OS ���}���`���j�^ API ���T�|�[�g���Ă��邩�ǂ����𔻕ʂ���B
//   98 �ȍ~/2000 �ȍ~�� TRUE ��Ԃ�
BOOL HasMultiMonitorSupport()
{
	HMODULE mod;

	if (((mod = GetModuleHandle("user32.dll")) != NULL) &&
	    (GetProcAddress(mod, "MonitorFromPoint") != NULL)) {
		return TRUE;
	}
	return FALSE;
}

// OS �� DnsQuery ���T�|�[�g���Ă��邩�ǂ����𔻕ʂ���B
//   2000 �ȍ~�� TRUE ��Ԃ�
BOOL HasDnsQuery()
{
	HMODULE mod;

	if (((mod = GetModuleHandle("Dnsapi.dll")) != NULL) &&
		(GetProcAddress(mod, "DnsQuery") != NULL)) {
		return TRUE;
	}
	return FALSE;
}

// �ʒm�A�C�R���ł̃o���[���`�b�v�ɑΉ����Ă��邩���ʂ���B
// Me/2000 �ȍ~�� TRUE ��Ԃ�
BOOL HasBalloonTipSupport()
{
	return IsWindows2000OrLater() || IsWindowsMe();
}

// OPENFILENAMEA.lStructSize �ɑ������l
DWORD get_OPENFILENAME_SIZE()
{
	if (IsWindows2000OrLater()) {
		return sizeof(OPENFILENAMEA);
	}
	return OPENFILENAME_SIZE_VERSION_400A;
}

// OPENFILENAMEW.lStructSize �ɑ������l
DWORD get_OPENFILENAME_SIZEW()
{
	if (IsWindows2000OrLater()) {
		return sizeof(OPENFILENAMEW);
	}
//	return OPENFILENAME_SIZE_VERSION_400W;
	return CDSIZEOF_STRUCT(OPENFILENAMEW,lpTemplateName);
}

char *mctimelocal(char *format, BOOL utc_flag)
{
	SYSTEMTIME systime;
	static char strtime[29];
	char week[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char month[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	char tmp[5];
	unsigned int i = strlen(format);

	if (utc_flag) {
		GetSystemTime(&systime);
	}
	else {
		GetLocalTime(&systime);
	}
	memset(strtime, 0, sizeof(strtime));
	for (i=0; i<strlen(format); i++) {
		if (format[i] == '%') {
			char c = format[i + 1];
			switch (c) {
				case 'a':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%s", week[systime.wDayOfWeek]);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'b':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%s", month[systime.wMonth - 1]);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'd':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wDay);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'e':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%2d", systime.wDay);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'H':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wHour);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'N':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%03d", systime.wMilliseconds);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'm':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wMonth);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'M':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wMinute);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'S':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%02d", systime.wSecond);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'w':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%d", systime.wDayOfWeek);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case 'Y':
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%04d", systime.wYear);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					i++;
					break;
				case '%':
					strncat_s(strtime, sizeof(strtime), "%", _TRUNCATE);
					i++;
					break;
				default:
					_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", format[i]);
					strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
					break;
			}
		}
		else {
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%c", format[i]);
			strncat_s(strtime, sizeof(strtime), tmp, _TRUNCATE);
		}
	}

	return strtime;
}

char *strelapsed(DWORD start_time)
{
	static char strtime[20];
	int days, hours, minutes, seconds, msecs;
	DWORD delta = GetTickCount() - start_time;

	msecs = delta % 1000;
	delta /= 1000;

	seconds = delta % 60;
	delta /= 60;

	minutes = delta % 60;
	delta /= 60;

	hours = delta % 24;
	days = delta / 24;

	_snprintf_s(strtime, sizeof(strtime), _TRUNCATE,
		"%d %02d:%02d:%02d.%03d",
		days, hours, minutes, seconds, msecs);

	return strtime;
}

wchar_t * PASCAL GetParam(wchar_t *buff, size_t size, wchar_t *param)
{
	size_t i = 0;
	BOOL quoted = FALSE;

	while (*param == ' ' || *param == '\t') {
		param++;
	}

	if (*param == '\0' || *param == ';') {
		return NULL;
	}

	while (*param != '\0' && (quoted || (*param != ';' && *param != ' ' && *param != '\t'))) {
		if (*param == '"') {
			if (*(param+1) != '"') {
				quoted = !quoted;
			}
			else {
				if (i < size - 1) {
					buff[i++] = *param;
				}
				param++;
			}
		}
		if (i < size - 1) {
			buff[i++] = *param;
		}
		param++;
	}
	if (!quoted && (buff[i-1] == ';')) {
		i--;
	}

	buff[i] = '\0';
	return (param);
}

void PASCAL DequoteParam(wchar_t *dest, size_t dest_len, wchar_t *src)
{
	BOOL quoted = FALSE;
	wchar_t *dest_end = dest + dest_len - 1;

	if (src == dest) {
		while (*src != '\0' && *src != '"' && dest < dest_end) {
			src++; dest++;
		}
	}

	while (*src != '\0' && dest < dest_end) {
		if (*src != '"' || (*++src == '"' && quoted)) {
			*dest++ = *src++;
		}
		else {
			quoted = !quoted;
		}
	}

	*dest = '\0';
}

void PASCAL DeleteComment(PCHAR dest, int dest_size, PCHAR src)
{
	BOOL quoted = FALSE;
	PCHAR dest_end = dest + dest_size - 1;

	while (*src != '\0' && dest < dest_end && (quoted || *src != ';')) {
		*dest++ = *src;

		if (*src++ == '"') {
			if (*src == '"' && dest < dest_end) {
				*dest++ = *src++;
			}
			else {
				quoted = !quoted;
			}
		}
	}

	*dest = '\0';

	return;
}

void split_buffer(char *buffer, int delimiter, char **head, char **body)
{
	char *p1, *p2;

	*head = *body = NULL;

	if (!isalnum(*buffer) || (p1 = strchr(buffer, delimiter)) == NULL) {
		return;
	}

	*head = buffer;

	p2 = buffer;
	while (p2 < p1 && !isspace(*p2)) {
		p2++;
	}

	*p2 = '\0';

	p1++;
	while (*p1 && isspace(*p1)) {
		p1++;
	}

	*body = p1;
}

/**
 *	�_�C�A���O�t�H���g���擾����
 *	�G���[�͔������Ȃ�
 */
DllExport void GetMessageboxFont(LOGFONT *logfont)
{
	NONCLIENTMETRICS nci;
	const int st_size = CCSIZEOF_STRUCT(NONCLIENTMETRICS, lfMessageFont);
	BOOL r;

	memset(&nci, 0, sizeof(nci));
	nci.cbSize = st_size;
	r = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, st_size, &nci, 0);
	assert(r == TRUE);
	*logfont = nci.lfStatusFont;
}
