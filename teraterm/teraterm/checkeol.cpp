
#include <stdlib.h>
#include <windows.h>

#include "checkeol.h"

// tttypes.h
#define CR   0x0D
#define LF   0x0A

struct CheckEOLData_st {
	BOOL cr_hold;
};

CheckEOLData_t *CheckEOLCreate(void)
{
	CheckEOLData_t *self = (CheckEOLData_t *)calloc(sizeof(CheckEOLData_t), 1);
	return self;
}

void CheckEOLDestroy(CheckEOLData_t *self)
{
	free(self);
}

void CheckEOLClear(CheckEOLData_t *self)
{
	self->cr_hold = FALSE;
}

/**
 *	����EOL(���s), u32 ���o�͂��邩���ׂ�
 *
 *	�߂�l�� CheckEOLRet �� OR �ŕԂ�
 *
 *	@retval	CheckEOLNoOutput	�����o�͂��Ȃ�
 *	@retval	CheckEOLOutputEOL	���s�R�[�h���o�͂���
 *	@retval	CheckEOLOutputChar	u32�����̂܂܏o�͂���
 */
CheckEOLRet CheckEOLCheck(CheckEOLData_t *self, unsigned int u32)
{
   	// ���͂����s(CR or LF)�̏ꍇ�A
	// ���s�̎��(CR or LF or CR+LF)�������Ŕ��肵��
	// OutputLogNewLine() �ŉ��s���o�͂���
	//		����    CR hold     ���s�o��   	CR hold �ύX
   	// 		+-------+-----------+-----------+------------
	//		CR      �Ȃ�        ���Ȃ�		�Z�b�g����
	//		LF      �Ȃ�        ����		�ω��Ȃ�
	//		���̑�  �Ȃ�        ���Ȃ�		�ω��Ȃ�
	//		CR      ����        ����		�ω��Ȃ�(�z�[���h�����܂�)
	//		LF      ����        ����		�N���A����
	//		���̑�  ����        ����		�N���A����
	if (self->cr_hold == FALSE) {
		if (u32 == CR) {
			self->cr_hold = TRUE;
			return CheckEOLNoOutput;
		}
		else if (u32 == LF) {
			return CheckEOLOutputEOL;
		}
		else {
			return CheckEOLOutputChar;
		}
	}
	else {
		if (u32 == CR) {
			return CheckEOLOutputEOL;
		}
		else if (u32 == LF) {
			self->cr_hold = FALSE;
			return CheckEOLOutputEOL;
		}
		else {
			self->cr_hold = FALSE;
			return (CheckEOLRet)(CheckEOLOutputEOL | CheckEOLOutputChar);
		}
	}
}
