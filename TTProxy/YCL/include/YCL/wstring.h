/*
 * Copyright (C) 2021- TeraTerm Project
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

#ifndef _YCL_WSTRING_H_
#define _YCL_WSTRING_H_

#pragma once

#include <YCL/common.h>

#include <string.h>
#include <wchar.h>

namespace yebisuya {

// ������̊Ǘ��E������s���N���X�B
class WString {
private:
	// ��������i�[����o�b�t�@�B
	// ������̑O�ɂ͎Q�ƃJ�E���^�������A
	// �����j���̍ۂɂ͂�����ύX����B
	const wchar_t* string;

	// utilities
	// ��������i�[����o�b�t�@���쐬����B
	// ������ƎQ�ƃJ�E���^�̕��̗̈���m�ۂ���B
	// �Q�ƃJ�E���^��0�ɂȂ��Ă���B
	// ����:
	//	length ������̒����B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static wchar_t* createBuffer(size_t length) {
		size_t* count = (size_t*) new unsigned char[sizeof (size_t) + sizeof (wchar_t) * (length + 1)];
		*count = 0;
		return (wchar_t*) (count + 1);
	}
	// ��������i�[�����o�b�t�@���쐬����B
	// ����:
	//	source �i�[���镶����B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static const wchar_t* create(const wchar_t* source) {
		return source != NULL ? create(source, wcslen(source)) : NULL;
	}
	// ��������i�[�����o�b�t�@���쐬����B
	// ����:
	//	source �i�[���镶����B
	//	length ������̒����B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static const wchar_t* create(const wchar_t* source, size_t length) {
		if (source != NULL) {
			wchar_t* buffer = createBuffer(length);
			wmemcpy(buffer, source, length);
			buffer[length] = '\0';
			return buffer;
		}
		return NULL;
	}
	// ��̕������A�����i�[�����o�b�t�@���쐬����B
	// ����:
	//	str1 �A�����镶����(�O)�B
	//	str2 �A�����镶����(��)�B
	// �Ԓl:
	//	�쐬�����o�b�t�@�̕����񕔂̃A�h���X�B
	static const wchar_t* concat(const wchar_t* str1, const wchar_t* str2) {
		size_t len1 = wcslen(str1);
		size_t len2 = wcslen(str2);
		wchar_t* buffer = createBuffer(len1 + len2);
		wmemcpy(buffer, str1, len1);
		wmemcpy(buffer + len1, str2, len2);
		buffer[len1 + len2] = '\0';
		return buffer;
	}
	// private methods
	// �Q�ƃJ�E���^�����炵�A0�ɂȂ�����o�b�t�@��j������B
	void release() {
		if (string != NULL) {
			size_t* count = (size_t*) string - 1;
			if (--*count == 0)
				delete[] (unsigned char*) count;
		}
	}
	// �Q�ƃJ�E���^�𑝂₷�B
	void add() {
		if (string != NULL) {
			size_t* count = (size_t*) string - 1;
			++*count;
		}
	}
	// �ʂ̃o�b�t�@�ƒu��������B
	// ���̃o�b�t�@�̎Q�ƃJ�E���^�����炵�A
	// �V�����o�b�t�@�̎Q�ƃJ�E���^�𑝂₷�B
	// ����:
	//	source �u��������V�����o�b�t�@�B
	void set(const wchar_t* source) {
		if (string != source) {
			release();
			string = source;
			add();
		}
	}
public:
	// constructor
	// �f�t�H���g�R���X�g���N�^�B
	// NULL�������Ă���̂ŁA���̂܂܂ŕ����񑀍삷��ƃA�N�Z�X�ᔽ�ɂȂ�̂Œ��ӁB
	WString():string(NULL) {
	}
	// ���̕�������w�肷��R���X�g���N�^�B
	// ����:
	//	source ���̕�����B
	WString(const wchar_t* source):string(NULL) {
		set(create(source));
	}
	// ���̕�����𒷂��t���Ŏw�肷��R���X�g���N�^�B
	// ����:
	//	source ���̕�����B
	//	length ������̒����B
	WString(const wchar_t* source, size_t length):string(NULL) {
		set(create(source, length));
	}
	// �R�s�[�R���X�g���N�^�B
	// ����:
	//	source ���̕�����B
	WString(const WString& source):string(NULL) {
		set(source.string);
	}
	// ��̕������A������R���X�g���N�^�B
	// ����:
	//	str1 �O�ɂȂ镶����B
	//	str2 ��ɂȂ镶����B
	WString(const wchar_t* str1, const wchar_t* str2):string(NULL) {
		set(concat(str1, str2));
	}
	// ��̕������A������R���X�g���N�^�B
	// ����:
	//	str1 �O�ɂȂ镶����B
	//	str2 ��ɂȂ镶����B
	WString(const WString& str1, const wchar_t* str2):string(NULL) {
		set(*str2 != '\0' ? concat(str1.string, str2) : str1.string);
	}
	// destructor
	// �f�X�g���N�^�B
	// �h�����邱�Ƃ͍l���Ă��Ȃ��̂ŉ��z�֐��ɂ͂��Ȃ��B
	~WString() {
		release();
	}
	// public methods
	// ���̕�����̌�Ɏw��̕������A������B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A�����ꂽ������B
	WString concat(const wchar_t* source)const {
		return WString(*this, source);
	}
	// ������Ƃ̔�r���s���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	���������0�Astr�̕����傫����Ε��A��������ΐ��B
	int compareTo(const wchar_t* str)const {
		if (str == NULL)
			return string == NULL ? 0 : 1;
		else if (string == NULL)
			return -1;
		return wcscmp(string, str);
	}
	// ������Ƃ̔�r��啶���������̋�ʂȂ��ōs���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	���������0�Astr�̕����傫����Ε��A��������ΐ��B
	int compareToIgnoreCase(const wchar_t* str)const {
		if (str == NULL)
			return string == NULL ? 0 : 1;
		else if (string == NULL)
			return -1;
		return _wcsicmp(string, str);
	}
	// ������Ƃ̔�r���s���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	��������ΐ^�B
	bool equals(const wchar_t* str)const {
		return compareTo(str) == 0;
	}
	// ������Ƃ̔�r��啶���������̋�ʂȂ��ōs���B
	// NULL�Ƃ���r�ł���B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	��������ΐ^�B
	bool equalsIgnoreCase(const wchar_t* str)const {
		return compareToIgnoreCase(str) == 0;
	}
	// �w�肳�ꂽ������Ŏn�܂��Ă��邩�ǂ����𔻒肷��B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	�w�肳�ꂽ������Ŏn�܂��Ă���ΐ^�B
	bool startsWith(const wchar_t* str)const {
		return startsWith(str, 0);
	}
	// �w��̈ʒu����w�肳�ꂽ������Ŏn�܂��Ă��邩�ǂ����𔻒肷��B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	�w�肳�ꂽ������Ŏn�܂��Ă���ΐ^�B
	bool startsWith(const wchar_t* str, int offset)const {
		return wcsncmp(string, str, wcslen(str)) == 0;
	}
	// �w�肳�ꂽ������ŏI����Ă��邩�ǂ����𔻒肷��B
	// ����:
	//	str ��r���镶����B
	// �Ԓl:
	//	�w�肳�ꂽ������ŏI����Ă���ΐ^�B
	//
	bool endsWith(const wchar_t* str)const {
		size_t str_length = wcslen(str);
		size_t string_length = length();
		if (string_length < str_length)
			return false;
		return wcscmp(string + string_length - str_length, str) == 0;
	}
	// �w��̕������ǂ̈ʒu�ɂ��邩��T���B
	// ����:
	//	chr �T�������B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int indexOf(char chr)const {
		return indexOf(chr, 0);
	}
	// �w��̕������ǂ̈ʒu�ɂ��邩���w��̈ʒu����T���B
	// ����:
	//	chr �T�������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int indexOf(wchar_t chr, size_t from)const {
		if (from >= length())
			return -1;
		const wchar_t* found = wcschr(string + from, chr);
		if (found == NULL)
			return -1;
		return found - string;
	}
	// �w��̕����񂪂ǂ̈ʒu�ɂ��邩��T���B
	// ����:
	//	str �T��������B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	int indexOf(const wchar_t* str)const {
		return indexOf(str, 0);
	}
	// �w��̕����񂪂ǂ̈ʒu�ɂ��邩���w��̈ʒu����T���B
	// ����:
	//	str �T��������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	//
	int indexOf(const wchar_t* str, size_t from)const {
		if (from >= length())
			return -1;
		const wchar_t* found = wcsstr(string + from, str);
		if (found == NULL)
			return -1;
		return found - string;
	}
	// ������̒�����Ԃ��B
	size_t length()const {
		return wcslen(string);
	}
	// �w��̕������Ō�Ɍ�����ʒu���擾����B
	// ����:
	//	chr �T�������B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(char chr)const {
		return lastIndexOf(chr, (size_t) -1);
	}
	// �w��̕������w��̈ʒu�����O�ōŌ�Ɍ�����ʒu���擾����B
	// ����:
	//	chr �T�������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	�����̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(wchar_t chr, size_t from)const {
		size_t len = length();
		if (from > len - 1)
			from = len - 1;
		const wchar_t* s = string;
		const wchar_t* end = string + from;
		const wchar_t* found = NULL;
		while (*s != '0' && s <= end) {
			if (*s == chr)
				found = s;
			s++;
		}
		return found != NULL ? found - string : -1;
	}
	// �w��̕����񂪍Ō�Ɍ�����ʒu���擾����B
	// ����:
	//	str �T��������B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(const wchar_t* str)const {
		return lastIndexOf(str, (size_t) -1);
	}
	// �w��̕����񂪎w��̈ʒu�����O�ōŌ�Ɍ�����ʒu���擾����B
	// ����:
	//	str �T��������B
	//	from �T���n�߂�ʒu�B
	// �Ԓl:
	//	������̌��������C���f�b�N�X�B������Ȃ����-1�B
	int lastIndexOf(const wchar_t* str, size_t from)const {
		size_t len = length();
		size_t str_len = wcslen(str);
		if (from > len - str_len)
			from = len - str_len;
		const wchar_t* s = string + from;
		while (s >= string) {
			if (wcsncmp(s, str, str_len) == 0)
				return s - string;
			s--;
		}
		return -1;
	}
	// ������̈ꕔ�����o���B
	// ����:
	//	start ���o��������̐擪�̈ʒu�B
	// �Ԓl:
	//	������̈ꕔ�B
	WString substring(int start)const {
		return WString(string + start);
	}
	// ������̈ꕔ�����o���B
	// ����:
	//	start ���o��������̐擪�̈ʒu�B
	//	end ���o��������̌�̈ʒu�B
	// �Ԓl:
	//	������̈ꕔ�B
	WString substring(int start, int end)const {
		return WString(string + start, end - start);
	}
	// �w��̈ʒu�ɂ��镶�������o���B
	// ����:
	//	index ���o�������̈ʒu�B
	// �Ԓl:
	//	�w��̈ʒu�ɂ��镶���B
	char charAt(size_t index)const {
		return index < length() ? string[index] : '\0';
	}
	// �w��̕������w��̕����ɒu�������܂��B
	// ����:
	//	oldChr ���̕����B
	//	newChr �u�������镶���B
	// �Ԓl:
	//	�u����̕�����B
	WString replace(char oldChr, char newChr)const {
		WString result(string);
		char* s = (char*) result.string;
		while (*s != '\0'){
			if (WString::isLeadByte(*s))
				s++;
			else if (*s == oldChr)
				*s = newChr;
			s++;
		}
		return result;
	}
	// �����񒆂̑啶�����������ɕϊ�����B
	// �Ԓl:
	//	�ϊ���̕�����B
	WString toLowerCase()const {
		WString result(string);
		char* s = (char*) result.string;
		while (*s != '\0'){
			if (WString::isLeadByte(*s))
				s++;
			else if ('A' <= *s && *s <= 'Z')
				*s += 'a' - 'A';
			s++;
		}
		return result;
	}
	// �����񒆂̏�������啶���ɕϊ�����B
	// �Ԓl:
	//	�ϊ���̕�����B
	WString toUpperCase()const {
		WString result(string);
		char* s = (char*) result.string;
		while (*s != '\0'){
			if (WString::isLeadByte(*s))
				s++;
			else if ('a' <= *s && *s <= 'z')
				*s += 'A' - 'a';
			s++;
		}
		return result;
	}
	// ������̑O��̋󔒕������폜����B
	// �Ԓl:
	//	�폜��̕�����B
	WString trim()const {
		const wchar_t* s = string;
		while (*s != '\0' && (unsigned char) *s <= ' ')
			s++;
		const wchar_t* start = s;
		s = string + length();
		while (s > start && (*s != '\0' && (unsigned char) *s <= ' '))
			s--;
		return WString(start, s - start);
	}

	// operators

	// const char*�ւ̃L���X�g���Z�q
	// �Ԓl:
	//	������ւ̃A�h���X�B
	operator const wchar_t*()const {
		return string;
	}
	// char�z��̂悤�Ɉ������߂�[]���Z�q�B
	// ����:
	//	index �擾���镶���̃C���f�b�N�X�B
	// �Ԓl:
	//	�w��̃C���f�b�N�X�ɂ��镶���B
	char operator[](size_t index)const {
		return charAt(index);
	}
	// �������A�����邽�߂�+���Z�q�B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A������������B
	WString operator+(const wchar_t* source)const {
		return WString(string, source);
	}
	// �������A�����邽�߂�+���Z�q�B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A������������B
	WString operator+(const WString& source)const {
		return *string != '\0' ? WString(string, source.string) : source;
	}
	// �������A�����邽�߂�+���Z�q�B
	// ����:
	//	str1 �A�����镶����(�O)�B
	//	str2 �A�����镶����(��)�B
	// �Ԓl:
	//	�A������������B
	friend WString operator+(const wchar_t* str1, const WString& str2) {
		return *str1 != '\0' ? WString(str1, str2.string) : str2;
	}
	// ������Z�q�B
	// ����:
	//	source ������镶����B
	// �Ԓl:
	//	������ʁB
	WString& operator=(const wchar_t* source) {
		set(create(source));
		return *this;
	}
	// ������Z�q�B
	// ����:
	//	source ������镶����B
	// �Ԓl:
	//	������ʁB
	WString& operator=(const WString& source) {
		set(source.string);
		return *this;
	}
	// �A���������ʂ������鉉�Z�q�B
	// ����:
	//	source �A�����镶����B
	// �Ԓl:
	//	�A�����ʁB
	WString& operator+=(const wchar_t* source) {
		if (*source != '\0')
			set(concat(string, source));
		return *this;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator==(const WString& str)const {
		return compareTo(str.string) == 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator==(const wchar_t* str)const {
		return compareTo(str) == 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕�����������ΐ^�B
	friend bool operator==(const wchar_t* str1, const WString& str2) {
		return str2.compareTo(str1) == 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����������Ȃ���ΐ^�B
	bool operator!=(const WString& str)const {
		return compareTo(str) != 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����������Ȃ���ΐ^�B
	bool operator!=(const wchar_t* str)const {
		return compareTo(str) != 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕����������Ȃ���ΐ^�B
	friend bool operator!=(const wchar_t* str1, const WString& str2) {
		return str2.compareTo(str1) != 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫����ΐ^�B
	bool operator<(const WString& str)const {
		return compareTo(str) < 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫����ΐ^�B
	bool operator<(const wchar_t* str)const {
		return compareTo(str) < 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕����傫����ΐ^�B
	friend bool operator<(const wchar_t* str1, const WString& str2) {
		return str2.compareTo(str1) > 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫������������ΐ^�B
	bool operator<=(const WString& str)const {
		return compareTo(str) <= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕����傫������������ΐ^�B
	bool operator<=(const wchar_t* str)const {
		return compareTo(str) <= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕����傫������������ΐ^�B
	friend bool operator<=(const wchar_t* str1, const WString& str2) {
		return str2.compareTo(str1) >= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator>(const WString& str)const {
		return compareTo(str) > 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�����������ΐ^�B
	bool operator>(const wchar_t* str)const {
		return compareTo(str) > 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕�����������ΐ^�B
	friend bool operator>(const wchar_t* str1, const WString& str2) {
		return str2.compareTo(str1) < 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�������������������ΐ^�B
	bool operator>=(const WString& str)const {
		return compareTo(str) >= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str ��r�Ώۂ̕�����B
	// �Ԓl:
	//	str�̕�������������������ΐ^�B
	bool operator>=(const wchar_t* str)const {
		return compareTo(str) >= 0;
	}
	// ��r���Z�q�B
	// ����:
	//	str1 ��r���镶����B
	//	str2 ��r���镶����B
	// �Ԓl:
	//	str1���str2�̕�������������������ΐ^�B
	friend bool operator>=(const wchar_t* str1, const WString& str2) {
		return str2.compareTo(str1) <= 0;
	}

	// public utilities

	// 2�o�C�g�����̍ŏ���1�o�C�g���ǂ����𔻒肷��B
	// ����:
	//	���肷��o�C�g�B
	// �Ԓl:
	//	2�o�C�g�����̍ŏ���1�o�C�g�ł���ΐ^�B
	static bool isLeadByte(char ch) {
	#ifdef _INC_WINDOWS
		return ::IsDBCSLeadByte(ch) != 0;
	#else
		return (ch & 0x80) != 0;
	#endif
	}
};

}

#endif//_YCL_WSTRING_H_
