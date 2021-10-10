// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define minimalTextBufferSize 32768L

#define textBufferLineBreak '\r' // for LineNumOf and PosOf

class TextBuffer {
public:
	TextBuffer(void);
	virtual ~TextBuffer(void);
	virtual void Indent(int32_t indent);
	virtual bool GetModified(void);
	virtual void SetModified(bool on);
	virtual size_t Length(void);
	virtual int32_t TheLength();
	virtual int32_t TheLengthInUTF8(void);
	virtual int32_t LineNumOf(int32_t pos);
	virtual int32_t PosOf(int32_t lineNum);
	virtual void GetText(size_t *textLen, char text[]); 
	virtual void GetText(size_t *textLen, wchar_t text[]);
	virtual void GetText(std::wstring &text); 
	virtual void SetText(int32_t textLen, const char text[]); 
	virtual void SetText(size_t textLen, const wchar_t text[]);
	virtual wchar_t GetCh(int32_t atPos);
	virtual void GetLine(int32_t atPos, int32_t *lineLen, wchar_t line[], int32_t *lineSepLen);
	virtual void GetRange(int32_t fromPos, int32_t toPos, wchar_t text[]);
	virtual int32_t Search(int32_t fromPos, bool wrapAround, wchar_t target[], bool caseSensitive); // returns position of target if found else -1
	virtual void Append(const wchar_t strg[]);
	virtual void AppendRange(const wchar_t strg[], int32_t fromPos, int32_t toPos);
	virtual void AppendCh(wchar_t ch);
	virtual void AppendLine(const wchar_t strg[]);
	virtual void AppendText(const TextBuffer *text);
	virtual void AppendTextRange(const TextBuffer *text, int32_t fromPos, int32_t toPos);
	virtual void Insert(int32_t atPos, const wchar_t strg[]);
	virtual void InsertRange(int32_t atPos, const wchar_t strg[], int32_t fromPos, int32_t toPos);
	virtual void InsertCh(int32_t atPos, wchar_t ch);
	virtual void Delete(int32_t fromPos, int32_t toPos);
protected:
	bool AssertTextSize(size_t deltaSize);
	size_t size,used; // number of wchar_t
	int32_t indent;
	bool modified;
	wchar_t *text;
};
