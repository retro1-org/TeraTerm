
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// in
	const char* UILanguageFile;
	const char *filesend_filter;
	// out
	wchar_t* filename;		// IDOK���A�I���t�@�C�������Ԃ�,�g�p��free()���邱��
	BOOL binary;			// TRUE/FALSE = �o�C�i��/�e�L�X�g
	int delay_type;
	DWORD delay_tick;
	size_t send_size;
	// work
	WORD MsgDlgHelp;
} sendfiledlgdata;

INT_PTR sendfiledlg(HINSTANCE hInstance, HWND hWndParent, sendfiledlgdata *data);

#ifdef __cplusplus
}
#endif

