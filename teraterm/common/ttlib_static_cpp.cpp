
#include <windows.h>
#include <string.h>
#if !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>


#include "i18n.h"
#include "layer_for_unicode.h"
#include "asprintf.h"

#include "ttlib.h"

/**
 *	GetI18nStrW() �̓��I�o�b�t�@��
 */
wchar_t *TTGetLangStrW(const char *section, const char *key, const wchar_t *def, const char *UILanguageFile)
{
	wchar_t *buf = (wchar_t *)malloc(MAX_UIMSG * sizeof(wchar_t));
	size_t size = GetI18nStrW(section, key, buf, MAX_UIMSG, def, UILanguageFile);
	buf = (wchar_t *)realloc(buf, size * sizeof(wchar_t));
	return buf;
}

/**
 *	MessageBox��\������
 *
 *	@param[in]	hWnd			�e window
 *	@param[in]	info			�^�C�g���A���b�Z�[�W
 *	@param[in]	uType			MessageBox�� uType
 *	@param[in]	UILanguageFile	lng�t�@�C��
 *	@param[in]	...				�t�H�[�}�b�g����
 *
 *	info.message ��������������Ƃ��āA
 *	UILanguageFile�����̈������o�͂���
 *
 *	info.message_key, info.message_default �����Ƃ�NULL�̏ꍇ
 *		�ψ�����1�ڂ�������������Ƃ��Ďg�p����
 */
int TTMessageBoxW(HWND hWnd, const TTMessageBoxInfoW *info, UINT uType, const char *UILanguageFile, ...)
{
	const char *section = info->section;
	wchar_t *title;
	if (info->title_key == NULL) {
		title = _wcsdup(info->title_default);
	}
	else {
		title = TTGetLangStrW(section, info->title_key, info->title_default, UILanguageFile);
	}

	wchar_t *message = NULL;
	if (info->message_key == NULL && info->message_default == NULL) {
		wchar_t *format;
		va_list ap;
		va_start(ap, UILanguageFile);
		format = va_arg(ap, wchar_t *);
		vaswprintf(&message, format, ap);
	}
	else {
		wchar_t *format = TTGetLangStrW(section, info->message_key, info->message_default, UILanguageFile);
		va_list ap;
		va_start(ap, UILanguageFile);
		vaswprintf(&message, format, ap);
		free(format);
	}

	int r = _MessageBoxW(hWnd, message, title, uType);

	free(title);
	free(message);

	return r;
}
