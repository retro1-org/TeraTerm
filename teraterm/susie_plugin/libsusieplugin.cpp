/*
 * Copyright (C) 2020 TeraTerm Project
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

#include "libsusieplugin.h"

#if !defined(WITHOUT_TERATERM)
#include "layer_for_unicode.h"
#define CreateFileW _CreateFileW
#define GetFileAttributesW _GetFileAttributesW
#define GetFullPathNameW _GetFullPathNameW
#define FindFirstFileW _FindFirstFileW
#define FindNextFileW _FindNextFileW
#define LoadLibraryW _LoadLibraryW
#endif

#if defined(_M_X64)
#define PLUGIN_EXT	L".sph"
#else
#define PLUGIN_EXT	L".spi"
#endif

/**
 * Susie �v���O�C�����g���ĉ摜��ǂݍ���
 *
 *	@param[in]	nameSPI		�v���O�C���t�@�C����
 *	@param[in]	nameFile	�摜�t�@�C����(�摜�̎�ނ𔻒肷��ۂɎg���邩������Ȃ�)
 *	@param[in]	bufFile		�摜�f�[�^�ւ̃|�C���^
 *	@param[in]	sizeFile	�摜�f�[�^�T�C�Y
 *	@param[out]	pHBInfo		BITMAPINFO			LocalFree()���邱��
 *	@param[out]	pHBm		bitmap data			LocalFree()���邱��
 *
 *	�v���O�C����Unicode�p�X���ɑ��݂��Ă����[�h�ł���
 *	�摜�t�@�C�����̓v���O�C�����ő����g�p����Ȃ�
 *	�v���O�C�����Ńt�@�C���͈���Ȃ��̂� Unicode����ok�Ǝv����
 */
BOOL LoadPictureWithSPI(const wchar_t *nameSPI, const wchar_t *nameFile, unsigned char *bufFile, long sizeFile, HLOCAL *hbuf,
						HLOCAL *hbmi)
{
	char nameFileA[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, nameFile, -1, nameFileA, _countof(nameFileA), NULL, NULL);

	HINSTANCE hSPI;
	char spiVersion[8];
	int(__stdcall * SPI_IsSupported)(LPCSTR, void *);
	int(__stdcall * SPI_GetPicture)(LPCSTR buf, LONG_PTR len, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm, FARPROC,
									LONG_PTR lData);
	int(__stdcall * SPI_GetPluginInfo)(int, LPSTR, int);
	int ret;

	ret = FALSE;
	hSPI = NULL;

	// SPI �����[�h
	hSPI = LoadLibraryW(nameSPI);

	if (!hSPI)
		goto error;

	FARPROC *p = (FARPROC *)&SPI_GetPluginInfo;
	*p = GetProcAddress(hSPI, "GetPluginInfo");
	p = (FARPROC *)&SPI_IsSupported;
	*p = GetProcAddress(hSPI, "IsSupported");
	p = (FARPROC *)&SPI_GetPicture;
	*p = GetProcAddress(hSPI, "GetPicture");

	if (!SPI_GetPluginInfo || !SPI_IsSupported || !SPI_GetPicture)
		goto error;

	//�o�[�W�����`�F�b�N
	SPI_GetPluginInfo(0, spiVersion, 8);

	if (spiVersion[2] != 'I' || spiVersion[3] != 'N')
		goto error;

	if (!(SPI_IsSupported)(nameFileA, bufFile))
		goto error;

	if ((SPI_GetPicture)((LPCSTR)bufFile, sizeFile, 1, hbmi, hbuf, NULL, 0))
		goto error;

	ret = TRUE;

error:

	if (hSPI)
		FreeLibrary(hSPI);

	return ret;
}

/**
 *	Susie�v���O�C�����g���ĉ摜�t�@�C�������[�h����
 *	�w��t�H���_���̃v���O�C�����g���ă��[�h�����݂�
 *
 *	@param[in]	image_file	�摜�t�@�C��
 *	@param[in]	spi_path	"c:\\path\\to\\spi"
 *							���̃t�H���_�ɂ���v���O�C���t�@�C���ŉ摜�̃��[�h�����݂�
 *							�p�X�̍Ō�� "\\" �������Ă��Ȃ��Ă��ǂ�
 *	@param[out]	pHBInfo		BITMAPINFO			LocalFree()���邱��
 *	@param[out]	pHBm		bitmap data			LocalFree()���邱��
 *	@retval		TRUE		���[�hok
 *	@retval		FALSE		���[�h�ł��Ȃ�����
 */
BOOL SusieLoadPicture(const wchar_t *image_file, const wchar_t *spi_path, HANDLE *pHBInfo, HANDLE *pHBm)
{
	BOOL result = FALSE;
	*pHBInfo = NULL;
	*pHBm = NULL;

	//�t�@�C����ǂݍ���
	HANDLE hPictureFile = CreateFileW(image_file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hPictureFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	DWORD fileSize = GetFileSize(hPictureFile, 0);
	if (fileSize < 2 * 1024) {
		//�Œ� 2kb �͊m�� (Susie plugin �̎d�l���)
		fileSize = 2 * 1024;
	}
	unsigned char *fileBuf = (unsigned char *)malloc(fileSize);
	memset(fileBuf, 0, 2*1024);	//���� 2kb �� 0 �ŏ�����
	DWORD readByte;
	ReadFile(hPictureFile, fileBuf, fileSize, &readByte, 0);
	CloseHandle(hPictureFile);

	// spi_path ���΃p�X�ɕϊ�
	wchar_t spi_path_full[MAX_PATH];
	if (!GetFullPathNameW(spi_path, _countof(spi_path_full), spi_path_full, NULL)) {
		goto finish;
	}
	wchar_t *p = wcsrchr(spi_path_full, L'\\');
	if (p != NULL) {
		if (*(p + 1) == 0) {
			*p = 0;		// delete last '\\'
		}
	}
	DWORD attr = GetFileAttributesW(spi_path_full);
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		goto finish;
	}

	wchar_t spi_path_mask[MAX_PATH];
	wcsncpy_s(spi_path_mask, _countof(spi_path_mask), spi_path_full, _TRUNCATE);
	wcsncat_s(spi_path_mask, _countof(spi_path_mask), L"\\*.*", _TRUNCATE);

	//�v���O�C���𓖂����Ă���
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(spi_path_mask, &fd);
	if (hFind == INVALID_HANDLE_VALUE) {
		goto finish;
	}
	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		const wchar_t *ext = wcsrchr(fd.cFileName, L'.');
		if (ext == NULL) {
			// �g���q���Ȃ��t�@�C��?
			continue;
		}
		if (wcscmp(ext, L".dll") != 0 && wcscmp(ext, PLUGIN_EXT) != 0) {
			// .dll or .spi(or sph) �ȊO�̃t�@�C��
			continue;
		}

		wchar_t spiFileName[MAX_PATH];
		wcsncpy_s(spiFileName, _countof(spiFileName), spi_path_full, _TRUNCATE);
		wcsncat_s(spiFileName, _countof(spiFileName), L"\\", _TRUNCATE);
		wcsncat_s(spiFileName, _countof(spiFileName), fd.cFileName, _TRUNCATE);

		HLOCAL hbuf, hbmi;
		if (LoadPictureWithSPI(spiFileName, image_file, fileBuf, fileSize, &hbuf, &hbmi)) {
			*pHBInfo = hbmi;
			*pHBm = hbuf;
			result = TRUE;
			break;
		}
	} while (FindNextFileW(hFind, &fd));
	FindClose(hFind);

finish:
	free(fileBuf);
	return result;
}
