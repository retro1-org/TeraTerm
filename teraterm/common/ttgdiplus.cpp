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

// TODO:
//	- GDI+ Flat API �֒u�������� DLL ���Ȃ��ꍇ���l��
//	  - GDI+ �� Windows XP���痘�pok

#include <windows.h>
#include <gdiplus.h>
#include <assert.h>

#include "ttgdiplus.h"

#define DEBUG_CHECK_IMAGE 0

static ULONG_PTR gdiplusToken;

void GDIPInit(void)
{
	assert(gdiplusToken == 0);
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::Status r = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	assert(r == Gdiplus::Ok);
	(void)r;
}

void GDIPUninit(void)
{
	Gdiplus::GdiplusShutdown(gdiplusToken);
	gdiplusToken = 0;
}

/**
 *	Gdiplus::Bitmap �̉摜�ۑ�
 *
 *	�g����
 *	Gdiplus::Bitmap bitmap;
 *	CLSID encoderClsid;
 *	GetEncoderClsid(L"image/png", &encoderClsid);
 *	bitmap.Save(L"Bird.png", &encoderClsid, NULL);
 */
#if DEBUG_CHECK_IMAGE
static int GetEncoderClsid(const wchar_t *mime_ext, CLSID *pClsid)
{
	UINT num = 0;	// number of image encoders
	UINT size = 0;	// size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo *pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;	// Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo *)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;	// Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j) {
		if ((wcsstr(pImageCodecInfo[j].MimeType, mime_ext) == NULL) ||
			(wcsstr(pImageCodecInfo[j].FilenameExtension, mime_ext) == NULL) ||
			(wcsstr(pImageCodecInfo[j].FormatDescription, mime_ext) == NULL)) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;	// Failure
}
#endif

#if DEBUG_CHECK_IMAGE
static void GDIPSavePNG(Gdiplus::Bitmap *bitmap, const wchar_t *filename)
{
	CLSID   encoderClsid;
	GetEncoderClsid(L"image/png", &encoderClsid);
	bitmap->Save(filename, &encoderClsid, NULL);
}
#endif

/**
 *	�摜�t�@�C����ǂݍ���� HBITMAP ��Ԃ�
 *
 *	GDI+�œǂݍ��񂾉摜��32bit bitmap�ƂȂ�
 *	(32�ȊO�ɂȂ邱�Ƃ����邩������Ȃ�)
 *	alpha�v���[�����܂�ł��Ȃ��摜�t�@�C����ǂݍ��ނƁA
 *	32bit bitmap�摜�f�[�^��alpha�l�͕s�����ɂȂ��Ă���
 *
 *	@param filename	�t�@�C����
 *	@retval			HBITMAP
 *					�s�v�ɂȂ����� DeleteObject() ���邱��
 *	@retval			NULL �t�@�C�����ǂݍ��߂Ȃ�����
 */
HBITMAP GDIPLoad(const wchar_t *filename)
{
	Gdiplus::Bitmap bitmap(filename);
	if (bitmap.GetLastStatus() != Gdiplus::Ok) {
		return NULL;
	}

#if DEBUG_CHECK_IMAGE
	GDIPSavePNG(&bitmap, L"test.png");
#endif

	HBITMAP hBmp;
	Gdiplus::Color bgColor = Gdiplus::Color(0, 0, 0);
	Gdiplus::Status r = bitmap.GetHBITMAP(bgColor, &hBmp);
	if (r != Gdiplus::Ok) {
		return NULL;
	}
	return hBmp;
}
