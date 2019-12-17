/*
 * Copyright (C) 2019 TeraTerm Project
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

#pragma warning(push, 0)
#include <stdlib.h>
#include <stdio.h>
#pragma warning(pop)

#include "unicode.h"

/**
 *	East_Asian_Width �Q�l���� �擾
 *
 *	@retval	'F'		Fullwidth �S�p
 *	@retval	'H'		Halfwidth ���p
 *	@retval	'W'		Wide �L
 *	@retval	'n'		Na,Narrow ��
 *	@retval	'A'		Ambiguous �B��
 *					�����ɂ���ĕ��������قȂ镶���B
 *					���A�W�A�̑g�łƂ���ȊO�̑g�ł̗����ɏo�����A
 *					���A�W�A�̏]�������R�[�h�ł͂�����S�p�Ƃ��Ĉ����邱�Ƃ�����B
 *					�M���V�A������L���������ȂǁB
 *	@retval	'N'		Neutral ����
 *					���A�W�A�̑g�łɂ͒ʏ�o�������A�S�p�ł����p�ł��Ȃ��B�A���r�A�����ȂǁB
 */
char UnicodeGetWidthProperty(unsigned long u32)
{
	typedef struct {
		unsigned long code_from;
		unsigned long code_to;
		char property;
	} east_asian_width_map_t;
	// �e�[�u���ɓ����Ă��Ȃ��ꍇ�� H
	const static east_asian_width_map_t east_asian_width_map[] = {
#include "unicode_asian_width.tbl"
	};
	const east_asian_width_map_t *table = east_asian_width_map;
	const size_t table_size = _countof(east_asian_width_map);
	char result;

	// �e�[�u���O�`�F�b�N
	if (u32 < east_asian_width_map[0].code_from) {
		return 'H';
	}
	if (east_asian_width_map[table_size-1].code_to < u32) {
		return 'H';
	}

	// �e�[�u������
	result = 'H';
	size_t low = 0;
	size_t high = table_size - 1;
	while (low < high) {
		size_t mid = (low + high) / 2;
		if (table[mid].code_from <= u32 && u32 <= table[mid].code_to) {
			result = table[mid].property;
			break;
		} else if (table[mid].code_to < u32) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	return result;
}

typedef struct {
	unsigned long code_from;
	unsigned long code_to;
} UnicodeTable_t;

/**
 * u32���e�[�u���̃f�[�^�Ɋ܂܂�Ă��邩���ׂ�
 */
static int UnicodeSimpleSearchTable(
	const UnicodeTable_t *table, size_t table_size,
	unsigned long u32)
{
	if (u32 < table[0].code_from) {
		return 0;
	}
	if (u32 > table[table_size-1].code_to) {
		return 0;
	}
	size_t low = 0;
	size_t high = table_size - 1;
	while (low <= high) {
		size_t mid = (low + high) / 2;
		if (table[mid].code_from <= u32 && u32 <= table[mid].code_to) {
			return 1;
		} else if (table[mid].code_to < u32) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
	// �e�[�u���͈̔͊O
	return 0;
}

/*
 * ������������������
 *		EMOJI MODIFIER �����������Ƃ��Ĉ���
 *
 *	@retval	0		���������ł͂Ȃ�
 *	@retval	1		���������ł���
 */
int UnicodeIsCombiningCharacter(unsigned long u32)
{
	const static UnicodeTable_t CombiningCharacterList[] = {
#include "unicode_combine.tbl"
	};
	return UnicodeSimpleSearchTable(
		CombiningCharacterList, _countof(CombiningCharacterList),
		u32);
}


int UnicodeIsEmoji(unsigned long u32)
{
	const static UnicodeTable_t EmojiList[] = {
#include "unicode_emoji.tbl"
	};
	return UnicodeSimpleSearchTable(
		EmojiList, _countof(EmojiList),
		u32);
}

/**
 *	�ّ̎��Z���N�^���`�F�b�N����
 *
 *	@retval	0		�ّ̎��Z���N�^�ł͂Ȃ�
 *	@retval	1		�ّ̎��Z���N�^�ł���
 */
int UnicodeIsVariationSelector(unsigned long u32)
{
	if ((0x00180b <= u32 && u32 <= 0x00180d) ||	// FVS (Mongolian Free Variation Selector)
		(0x00fe00 <= u32 && u32 <= 0x00fe0f) ||	// SVS VS1�`VS16
		(0x0e0100 <= u32 && u32 <= 0x0e01ef))	// IVS VS17�`VS256
	{
		return 1;
	}
	return 0;
}


#if 0
int main(int, char *[])
{
	static const unsigned long codes[] = {
#if 0
		0, 1, 0x7f,
		0x80,
		0x0e00ff,
		0x0e0100,
		0x10fffd,
#endif
		0x10fffe,
	};

	for (size_t i = 0; i < _countof(codes); i++) {
		unsigned long code = codes[i];
		printf("U+%06lx %c\n", code, UnicodeGetWidthProperty(code));
	}
	return 0;
}
#endif
