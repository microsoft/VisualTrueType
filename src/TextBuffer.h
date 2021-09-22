// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define minimalTextBufferSize 32768L

#define textBufferLineBreak '\r' // for LineNumOf and PosOf

class TextBuffer {
public:
	TextBuffer(void);
	virtual ~TextBuffer(void);
	virtual void Indent(long indent);
	virtual bool GetModified(void);
	virtual void SetModified(bool on);
	virtual size_t Length(void);
	virtual long TheLength();
	virtual long TheLengthInUTF8(void);
	virtual long LineNumOf(long pos);
	virtual long PosOf(long lineNum);
	virtual void GetText(size_t *textLen, char text[]); 
	virtual void GetText(size_t *textLen, wchar_t text[]);
	virtual void GetText(std::wstring &text); 
	virtual void SetText(long textLen, const char text[]); 
	virtual void SetText(size_t textLen, const wchar_t text[]);
	virtual wchar_t GetCh(long atPos);
	virtual void GetLine(long atPos, long *lineLen, wchar_t line[], long *lineSepLen);
	virtual void GetRange(long fromPos, long toPos, wchar_t text[]);
	virtual long Search(long fromPos, bool wrapAround, wchar_t target[], bool caseSensitive); // returns position of target if found else -1
	virtual void Append(const wchar_t strg[]);
	virtual void AppendRange(const wchar_t strg[], long fromPos, long toPos);
	virtual void AppendCh(wchar_t ch);
	virtual void AppendLine(const wchar_t strg[]);
	virtual void AppendText(const TextBuffer *text);
	virtual void AppendTextRange(const TextBuffer *text, long fromPos, long toPos);
	virtual void Insert(long atPos, const wchar_t strg[]);
	virtual void InsertRange(long atPos, const wchar_t strg[], long fromPos, long toPos);
	virtual void InsertCh(long atPos, wchar_t ch);
	virtual void Delete(long fromPos, long toPos);
protected:
	bool AssertTextSize(size_t deltaSize);
	size_t size,used; // number of wchar_t
	long indent;
	bool modified;
	wchar_t *text;
};
