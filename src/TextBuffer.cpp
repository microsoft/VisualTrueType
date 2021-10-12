// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define _CRT_SECURE_NO_DEPRECATE 
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <assert.h> // assert
#include <stdio.h> // swprintf
#include <string.h> // strncpy etc.
#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <climits>
#include <codecvt>
#include <locale>

#include "TextBuffer.h"
#ifndef _WIN32
#define swprintf(wcs, ...) swprintf(wcs, 1024, __VA_ARGS__)
#endif

#ifndef	Min
	#define Min(a,b)	((a) < (b) ? (a) : (b))
#endif
#ifndef	Max
	#define Max(a,b)	((a) > (b) ? (a) : (b))
#endif

TextBuffer::TextBuffer(void) {
	this->size = this->used = 0;
	this->indent = 0;
	this->modified = false;
	this->text = NULL;
} // TextBuffer::TextBuffer

TextBuffer::~TextBuffer(void) {
	if (this->text != NULL) free(this->text);
} // TextBuffer::~TextBuffer

void TextBuffer::Indent(int32_t indent) {
	this->indent = Max(0,this->indent + indent);
} // TextBuffer::Indent

bool TextBuffer::GetModified(void) {
	return this->modified;
} // TextBuffer::GetModified

void TextBuffer::SetModified(bool on) {
	this->modified = on;
} // TextBuffer::SetModified

size_t TextBuffer::Length(void) {
	return this->used;
} // TextBuffer::Length

int32_t  TextBuffer::TheLength()
{
	return (int32_t)this->Length();
}

int32_t TextBuffer::TheLengthInUTF8(void)
{
	std::string text8;

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	text8 = converter.to_bytes(this->text);

	return static_cast<int32_t>(text8.length()); 
}

int32_t TextBuffer::LineNumOf(int32_t pos) {
	wchar_t *text,*textEnd;
	int32_t lineNum;

	for (text = this->text, textEnd = text + Max(0,Min(pos,(int32_t)this->used)), lineNum = 0; text < textEnd; text++)
		if (*text == textBufferLineBreak)
			lineNum++;
	
	return lineNum;
} // TextBuffer::LineNumOf

int32_t TextBuffer::PosOf(int32_t lineNum) {
	wchar_t *text,*textEnd;

	for (text = this->text, textEnd = text + this->used; lineNum > 0 && text < textEnd; text++)
		if (*text == textBufferLineBreak)
			lineNum--;

	return (int32_t)(ptrdiff_t)(text - this->text);
} // TextBuffer::PosOf


void TextBuffer::GetText(size_t *textLen, char text[])
{
	std::string text8; 

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	text8 = converter.to_bytes(this->text);

	*textLen = static_cast<int32_t>(text8.size());
	if (*textLen > 0)
	{
		std::copy(text8.begin(), text8.end(), text);
		text[text8.size()] = '\0'; // don't forget the terminating 0
	}
}

void TextBuffer::GetText(size_t *textLen, wchar_t text[]) {
	*textLen = this->used;
	if (*textLen > 0) wcsncpy(text,this->text,*textLen);
	text[*textLen] = L'\0';
} // TextBuffer::GetText

void TextBuffer::GetText(std::wstring &text)
{
	text.assign(this->text); 	 
}

void TextBuffer::SetText(int32_t textLen, const char text[])
{
	if (textLen == 0)
	{
		this->SetText(0, L"");
	}
	else
	{
		std::string str(reinterpret_cast<const char*>(text), textLen);
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wstr = converter.from_bytes(str);

		this->SetText(wstr.length(), wstr.c_str()); 
	}
}

void TextBuffer::SetText(size_t textLen, const wchar_t text[]) {
	if (!this->AssertTextSize(textLen + 1 - this->used)) return;

	if (textLen > 0) 
		wcsncpy(this->text,text,textLen);
	this->used = textLen;
	this->text[this->used] = L'\0';
	this->modified = false; // !!!
} // TextBuffer::SetText

wchar_t TextBuffer::GetCh(int32_t atPos) {
	return 0 <= atPos && atPos < (int32_t)this->used ? this->text[atPos] : L'\0';
} // TextBuffer::GetCh

void TextBuffer::GetLine(int32_t atPos, int32_t *lineLen, wchar_t line[], int32_t *lineSepLen) {
	int32_t startPos,endPos,usedLineLen;

	*lineLen = *lineSepLen = 0;
	if (atPos < 0 || (int32_t)this->used <= atPos) return;
	startPos = atPos;
	while (startPos > 0 && this->text[startPos-1] != textBufferLineBreak) startPos--;
	endPos = atPos;
	while (endPos < (int32_t)this->used && this->text[endPos] != textBufferLineBreak) endPos++;
	*lineLen = endPos - startPos;
	*lineSepLen = 1;
	usedLineLen = Min(*lineLen,0xff);
	wcsncpy(line,&this->text[startPos],usedLineLen);
	line[usedLineLen] = L'\0';
} // TextBuffer::GetLine

void TextBuffer::GetRange(int32_t fromPos, int32_t toPos, wchar_t text[]) {
	int32_t chars;

	fromPos = (int32_t)Max(0,Min(fromPos,(int32_t)this->used));
	toPos   = (int32_t)Max(fromPos,Min(toPos,(int32_t)this->used));
	chars   = toPos - fromPos;
	if (chars < 0) return;

	if (chars > 0) wcsncpy(text,&this->text[fromPos],chars);
	text[chars] = L'\0';
} // TextBuffer::GetRange

int32_t TextBuffer::Search(int32_t fromPos, bool wrapAround, wchar_t target[], bool caseSensitive) {
#ifdef _DEBUG
	assert(false);
#endif
	
	return -1; // not found
} // TextBuffer::Search

void TextBuffer::Append(const wchar_t strg[]) {
	size_t chars;
	
	chars = wcslen(strg);
	if (chars <= 0) return;
	
	if (!this->AssertTextSize(chars + 1)) return;
	
	if (chars > 0) wcsncpy(&this->text[this->used],strg,chars);
	this->used += chars;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::Append

void TextBuffer::AppendRange(const wchar_t strg[], int32_t fromPos, int32_t toPos) {
	size_t chars;
	
	chars = wcslen(strg);
	if (chars <= 0) return;
	
	fromPos = (int32_t)Max(0,Min(fromPos,(int32_t)chars));
	toPos = (int32_t)Max(fromPos,Min(toPos,(int32_t)chars));
	chars = toPos - fromPos;
	if (chars <= 0) return;
		
	if (!this->AssertTextSize(chars + 1)) return;
	
	wcsncpy(&this->text[this->used],&strg[fromPos],chars);
	this->used += chars;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::AppendRange

void TextBuffer::AppendCh(wchar_t ch) {
	if (!this->AssertTextSize(1 + 1)) return;
	
	this->text[this->used++] = ch;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::AppendCh

void TextBuffer::AppendLine(const wchar_t strg[]) {
	size_t chars, charsEol;
    int32_t indent;
	wchar_t eol[4];
	
	chars = wcslen(strg);
	if (chars < 0) return; // hardly, though...
	
	indent = chars > 0 ? this->indent : 0;
	
#ifndef _WIN32
	swprintf(eol,L"\n");
#else
	swprintf(eol,L"\r");
#endif
	charsEol = wcslen(eol);

	if (!this->AssertTextSize(indent + chars + charsEol + 1)) return;
	
	if (indent > 0) 
		memset(&this->text[this->used],L' ',indent * sizeof(wchar_t));
	this->used += indent;
	if (chars > 0) 
		wcsncpy(&this->text[this->used],strg,chars);
	this->used += chars;
	wcsncpy(&this->text[this->used],eol,charsEol);
	this->used += charsEol;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::AppendLine

void TextBuffer::AppendText(const TextBuffer *text) {
	if (text->used > 0) this->AppendRange(text->text,0,(int32_t)text->used);
} // TextBuffer::AppendText

void TextBuffer::AppendTextRange(const TextBuffer *text, int32_t fromPos, int32_t toPos) {
	fromPos = (int32_t)Max(0,Min(fromPos,(int32_t)text->used));
	toPos = (int32_t)Max(fromPos,Min(toPos,(int32_t)text->used));
	if (fromPos < toPos) this->AppendRange(text->text,fromPos,toPos);
} // TextBuffer::AppendTextRange

void TextBuffer::Insert(int32_t atPos, const wchar_t strg[]) {
	size_t chars;
    int32_t remainder;
	
	chars = wcslen(strg);
	if (chars <= 0) return;
	
	if (!this->AssertTextSize(chars + 1)) return;
	
	atPos =(int32_t) Max(0,Min(atPos,(int32_t)this->used));
	
	remainder = (int32_t)this->used - atPos;
	if (remainder > 0) 
		memmove(&this->text[atPos + chars],&this->text[atPos],remainder * sizeof(wchar_t));
	
	wcsncpy(&this->text[atPos],strg,chars);
	this->used += chars;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::Insert

void TextBuffer::InsertRange(int32_t atPos, const wchar_t strg[], int32_t fromPos, int32_t toPos) {
	int32_t remainder;
	
	size_t chars = wcslen(strg);
	if (chars <= 0) return;
	
	fromPos = (int32_t)Max(0,Min(fromPos,(int32_t)chars));
	toPos = (int32_t)Max(fromPos,Min(toPos,(int32_t)chars));
	chars = toPos - fromPos;
	if (chars <= 0) return;
		
	if (!this->AssertTextSize(chars + 1)) return;
	
	atPos = (int32_t)Max(0,Min(atPos,(int32_t)this->used));
	
	remainder = (int32_t)this->used - atPos;
	if (remainder > 0) 
		memmove(&this->text[atPos + chars],&this->text[atPos],remainder * sizeof(wchar_t));
	
	wcsncpy(&this->text[atPos],&strg[fromPos],chars);
	this->used += chars;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::InsertRange

void TextBuffer::InsertCh(int32_t atPos, wchar_t ch) {
	int32_t remainder;
	
	if (!this->AssertTextSize(1 + 1)) return;
	
	atPos = (int32_t)Max(0,Min(atPos,(int32_t)this->used));
	
	remainder = (int32_t)this->used - atPos;
	if (remainder > 0) 
		memmove(&this->text[atPos + 1],&this->text[atPos],remainder * sizeof(wchar_t));
	
	this->text[atPos] = ch;
	this->used++;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::InsertCh

void TextBuffer::Delete(int32_t fromPos, int32_t toPos) {
	int32_t chars,remainder;

	fromPos = (int32_t)Max(0,Min(fromPos,(int32_t)this->used));
	toPos = (int32_t)Max(fromPos,Min(toPos,(int32_t)this->used));
	chars = toPos - fromPos;
	if (chars <= 0) return;
	
	remainder = (int32_t)this->used - toPos;
	if (remainder > 0) 
		memmove(&this->text[fromPos],&this->text[toPos],remainder * sizeof(wchar_t));
	
	this->used -= chars;
	this->text[this->used] = L'\0';
	this->modified = true;
} // TextBuffer::Delete

bool TextBuffer::AssertTextSize(size_t deltaSize) {
	size_t newSize;
	wchar_t *tmpText;
	
	if (this->used + deltaSize <= this->size) return true;

	newSize = this->size + ((deltaSize + minimalTextBufferSize - 1)/minimalTextBufferSize)*minimalTextBufferSize;
	tmpText = (wchar_t *)malloc(newSize * sizeof(wchar_t));
	if (tmpText != NULL) {
		memcpy(tmpText,this->text,this->used * sizeof(wchar_t));
		if (this->text != NULL) 
			free(this->text);
		this->size = newSize;
		this->text = tmpText;
	}
	return tmpText != NULL;
} // TextBuffer::AssertTextSize