// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define minimalTextBufferSize 32768L

#define textBufferLineBreak '\r' // for LineNumOf and PosOf

class TextBuffer {
public:
	TextBuffer(void);
	virtual ~TextBuffer(void);
	virtual void Indent(int indent);
	virtual bool GetModified(void);
	virtual void SetModified(bool on);
	virtual size_t Length(void);
	virtual int TheLength();
	virtual int TheLengthInUTF8(void);
	virtual int LineNumOf(int pos);
	virtual int PosOf(int lineNum);
	virtual void GetText(size_t *textLen, char text[]); 
	virtual void GetText(size_t *textLen, wchar_t text[]);
	virtual void GetText(std::wstring &text); 
	virtual void SetText(int textLen, const char text[]); 
	virtual void SetText(size_t textLen, const wchar_t text[]);
	virtual wchar_t GetCh(int atPos);
	virtual void GetLine(int atPos, int *lineLen, wchar_t line[], int *lineSepLen);
	virtual void GetRange(int fromPos, int toPos, wchar_t text[]);
	virtual int Search(int fromPos, bool wrapAround, wchar_t target[], bool caseSensitive); // returns position of target if found else -1
	virtual void Append(const wchar_t strg[]);
	virtual void AppendRange(const wchar_t strg[], int fromPos, int toPos);
	virtual void AppendCh(wchar_t ch);
	virtual void AppendLine(const wchar_t strg[]);
	virtual void AppendText(const TextBuffer *text);
	virtual void AppendTextRange(const TextBuffer *text, int fromPos, int toPos);
	virtual void Insert(int atPos, const wchar_t strg[]);
	virtual void InsertRange(int atPos, const wchar_t strg[], int fromPos, int toPos);
	virtual void InsertCh(int atPos, wchar_t ch);
	virtual void Delete(int fromPos, int toPos);
protected:
	bool AssertTextSize(size_t deltaSize);
	size_t size,used; // number of wchar_t
	int indent;
	bool modified;
	wchar_t *text;
};
