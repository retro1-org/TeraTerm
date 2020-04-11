#include <windows.h>
#include <assert.h>

/**
 *	win16_lcreat() �� wchar_t��
 *	@param[in]	iAttribute	teraterm�ł�0�����g�p���Ȃ�
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) �I�[�v���ł��Ȃ�����
 *				(���ۂ�API��HFILE_ERROR((HFILE)-1)��Ԃ�)
 */
HANDLE win16_lcreatW(const wchar_t *FileName, int iAttribute)
{
	HANDLE handle;
	assert(iAttribute == 0);
	handle = CreateFileW(FileName,
						 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return handle;
}

/**
 *	@param[in]	iAttribute	teraterm�ł�0�����g�p���Ȃ�
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) �I�[�v���ł��Ȃ�����
 *				(���ۂ�API��HFILE_ERROR((HFILE)-1)��Ԃ�)
 */
HANDLE win16_lcreat(const char *FileName, int iAttribute)
{
	HANDLE handle;
	assert(iAttribute == 0);
	handle = CreateFileA(FileName,
						 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
						 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return handle;
}

/**
 *	win16_lopen() �� wchar_t ��
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) �I�[�v���ł��Ȃ�����
 *				(���ۂ�API��HFILE_ERROR((HFILE)-1)��Ԃ�)
 */
HANDLE win16_lopenW(const wchar_t *FileName, int iReadWrite)
{
	HANDLE handle;
	switch(iReadWrite) {
	case OF_READ:
		// read only
		handle = CreateFileW(FileName,
							 GENERIC_READ, FILE_SHARE_READ, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_WRITE:
		// write
		handle = CreateFileW(FileName,
							 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_READWRITE:
		// read/write (teraterm�ł�ttpmacro/ttl.c����1�ӏ��݂̂Ŏg�p����Ă���
		handle = CreateFileW(FileName,
							 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	default:
		assert(FALSE);
		handle = INVALID_HANDLE_VALUE;
		break;
	}
	return handle;
}

/**
 *	@retval 	handle
 *	@retval 	INVALID_HANDLE_VALUE((HANDLE)(LONG_PTR)-1) �I�[�v���ł��Ȃ�����
 *				(���ۂ�API��HFILE_ERROR((HFILE)-1)��Ԃ�)
 */
HANDLE win16_lopen(const char *FileName, int iReadWrite)
{
	HANDLE handle;
	switch(iReadWrite) {
	case OF_READ:
		// read only
		handle = CreateFileA(FileName,
							 GENERIC_READ, FILE_SHARE_READ, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_WRITE:
		// write
		handle = CreateFileA(FileName,
							 GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	case OF_READWRITE:
		// read/write (teraterm�ł�ttpmacro/ttl.c����1�ӏ��݂̂Ŏg�p����Ă���
		handle = CreateFileA(FileName,
							 GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL,
							 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		break;
	default:
		assert(FALSE);
		handle = INVALID_HANDLE_VALUE;
		break;
	}
	return handle;
}

/**
 *	@retval �Ȃ�
 *			(���ۂ�API�̓I�[�v�����Ă���HFILE��Ԃ�)
 */
void win16_lclose(HANDLE hFile)
{
	CloseHandle(hFile);
}

/**
 *	@retval �ǂݍ��݃o�C�g��
 */
UINT win16_lread(HANDLE hFile, LPVOID lpBuffer, UINT uBytes)
{
	DWORD NumberOfBytesRead;
	BOOL Result = ReadFile(hFile, lpBuffer, uBytes, &NumberOfBytesRead, NULL);
	if (Result == FALSE) {
		return 0;
	}
	return NumberOfBytesRead;
}

/**
 *	@retval �������݃o�C�g��
 */
UINT win16_lwrite(HANDLE hFile, const char*buf, UINT length)
{
	DWORD NumberOfBytesWritten;
	BOOL result = WriteFile(hFile, buf, length, &NumberOfBytesWritten, NULL);
	if (result == FALSE) {
		return 0;
	}
	return NumberOfBytesWritten;
}

/*
 *	@param[in]	iOrigin
 *				@arg 0(FILE_BEGIN)
 *				@arg 1(FILE_CURRENT)
 *				@arg 2(FILE_END)
 *	@retval �t�@�C���ʒu
 *	@retval HFILE_ERROR((HFILE)-1)	�G���[
 *	@retval INVALID_SET_FILE_POINTER((DWORD)-1) �G���[
 */
LONG win16_llseek(HANDLE hFile, LONG lOffset, int iOrigin)
{
	DWORD pos = SetFilePointer(hFile, lOffset, NULL, iOrigin);
	if (pos == INVALID_SET_FILE_POINTER) {
		return HFILE_ERROR;
	}
	return pos;
}
