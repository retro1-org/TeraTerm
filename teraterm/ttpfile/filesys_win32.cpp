/*
 * (C) 2020 TeraTerm Project
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
#include "tttypes.h"
#include "codeconv.h"

#include "filesys_win32.h"

static BOOL _OpenRead(TFileVarProto *fv, const char *filename)
{
	HANDLE hFile = CreateFileA(filename,
							   GENERIC_READ, FILE_SHARE_READ, NULL,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		fv->FileHandle = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	fv->FileHandle = hFile;
	return TRUE;
}

static BOOL _OpenWrite(TFileVarProto *fv, const char *filename)
{
	HANDLE hFile = CreateFileA(filename,
							   GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
							   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		fv->FileHandle = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	fv->FileHandle = hFile;
	return TRUE;
}

static size_t _ReadFile(TFileVarProto *fv, void *buf, size_t bytes)
{
	HANDLE hFile = fv->FileHandle;
	DWORD NumberOfBytesRead;
	BOOL Result = ReadFile(hFile, buf, (UINT)bytes, &NumberOfBytesRead, NULL);
	if (Result == FALSE) {
		return 0;
	}
	return NumberOfBytesRead;
}

static size_t _WriteFile(TFileVarProto *fv, const void *buf, size_t bytes)
{
	HANDLE hFile = fv->FileHandle;
	DWORD NumberOfBytesWritten;
	UINT length = (UINT)bytes;
	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
	if (result == FALSE) {
		return 0;
	}
	return NumberOfBytesWritten;
}

static void _Close(TFileVarProto *fv)
{
	if (fv->FileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fv->FileHandle);
		fv->FileHandle = INVALID_HANDLE_VALUE;
	}
}

/**
 *	�t�@�C���̃t�@�C���T�C�Y���擾
 *	@param[in]	filenameU8		�t�@�C����(UTF-8)
 *	@retval		�t�@�C���T�C�Y
 */
static size_t _GetFSize(struct FileVarProto *fv, const char *filenameU8)
{
	size_t file_size = GetFSize64W(wc::fromUtf8(filenameU8));
	return file_size;
}

/**
 *	@retval	0	ok
 *	@retval	-1	error
 */
static int Seek(struct FileVarProto *fv, size_t offset)
{
	LONG lo = (LONG)((offset >> 0) & 0xffffffff);
	LONG hi = (LONG)((offset >> 32) & 0xffffffff);
	SetFilePointer(fv->FileHandle, lo, &hi, 0);
	if (GetLastError() != 0) {
		return -1;
	}
	return 0;
}

static void FileSysDestroy(TFileVarProto *fv)
{
	fv->Close(fv);
}

void FilesysCreate(TFileVarProto *fv)
{
	fv->FileHandle = INVALID_HANDLE_VALUE;
	fv->OpenRead = _OpenRead;
	fv->OpenWrite = _OpenWrite;
	fv->ReadFile = _ReadFile;
	fv->WriteFile = _WriteFile;
	fv->Close = _Close;
	fv->GetFSize = _GetFSize;
	fv->Seek = Seek;
	fv->FileSysDestroy = FileSysDestroy;
}
