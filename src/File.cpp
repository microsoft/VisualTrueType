// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define NOMINMAX
#include <Windows.h>
#include <assert.h>

#include "pch.h"

#define IsUnicode(b) ((b[0] == 0xFF) && (b[1] == 0xFE))
#define IsUnicodeBig(b) ((b[0] == 0xFE) && (b[1] == 0xFF))
#define IsUTF8(b) ((b[0] == 0xEF) && (b[1] == 0xBB) && (b[2] == 0xBF)) 

bool File::Error() {
	return m_error;
} // File::Error

File::File(void) {
	this->m_hfile = INVALID_HANDLE_VALUE;
} // File::File

File::~File(void) {
	if (this->m_hfile != INVALID_HANDLE_VALUE)
		this->Close(false);
} // File::~File

void File::OpenOld(long volRefNum, const wchar_t fileName[]) {
	//if (wcslen(fileName)<3 || (fileName[1] != L':' && fileName[1] != L'\\' && fileName[0] != L'\\'))
	//	SetCurrentDirectoryW(GetVolumePath(volRefNum));
	m_hfile = CreateFileW(fileName, GENERIC_READ /* | GENERIC_WRITE */, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	m_error = (m_hfile == INVALID_HANDLE_VALUE);
} // File::OpenOld

void File::OpenNew(long volRefNum, const wchar_t fileName[]) {
	//if (wcslen(fileName)<3 || (fileName[1] != L':' && fileName[1] != L'\\' && fileName[0] != L'\\'))
	//	SetCurrentDirectoryW(GetVolumePath(volRefNum));
	m_hfile = CreateFileW(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	m_error = (m_hfile == INVALID_HANDLE_VALUE);
} // File::OpenNew

void File::OpenOld(const wchar_t fileName[]) {
	m_hfile = CreateFileW(fileName, GENERIC_READ /* | GENERIC_WRITE */, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	m_error = (m_hfile == INVALID_HANDLE_VALUE);
} // File::OpenOld

void File::OpenNew(const wchar_t fileName[]) {
	m_hfile = CreateFileW(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	m_error = (m_hfile == INVALID_HANDLE_VALUE);
} // File::OpenNew

bool File::Exists(long volRefNum, const wchar_t fileName[]) {
	//if (wcslen(fileName)<3 || (fileName[1] != L':' && fileName[1] != L'\\' && fileName[0] != L'\\'))
	//	SetCurrentDirectoryW(GetVolumePath(volRefNum));
	DWORD dwAttrib = GetFileAttributes(fileName);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

long File::Length(void) {
	return (m_hfile != INVALID_HANDLE_VALUE) ? GetFileSize(m_hfile, NULL) : 0;
} // File::Length

void File::SetPos(long pos, bool truncate) {
	SetFilePointer(m_hfile, pos, NULL, FILE_BEGIN);
	if (truncate)
		SetEndOfFile(m_hfile);
} // File::SetPos

long File::GetPos(void) {
	return SetFilePointer(m_hfile, 0, NULL, FILE_CURRENT);
} // File::GetPos

void File::ReadBytes(long numBytes, void *buffer) {
	if (m_hfile != INVALID_HANDLE_VALUE) {
		DWORD cb = 0;
		m_error = !ReadFile(m_hfile, buffer, numBytes, &cb, NULL);
	}
} // File::ReadBytes

void File::ReadUnicode(long *len, wchar_t **text)
{
	if (m_hfile != INVALID_HANDLE_VALUE) {
		DWORD cb;
		BYTE start[4];

		m_error = !ReadFile(m_hfile, start, 4, &cb, NULL);
		assert(4 == cb);
		if (m_error) return;

		long fileLen = this->Length();

		if (IsUnicode(start) || IsUnicodeBig(start))
		{
			SetPos(2, false);

			*text = (wchar_t*)NewP(fileLen);
			if (*text == NULL)
			{
				m_error = true;
				return;
			}

			m_error = !ReadFile(m_hfile, *text, fileLen - sizeof(wchar_t), &cb, NULL);
			assert(fileLen - sizeof(wchar_t) == cb);
			if (m_error) return;

			*len = (fileLen - sizeof(wchar_t)) >> 1;
			(*text)[*len] = 0;

			if (IsUnicodeBig(start))
			{
				for (long i = 0; i < *len; i++)
				{
					(*text)[i] = SWAPW((*text)[i]);
				}
			}
		}
		else
		{
			SetPos(0, false);

			char* b = (char*)NewP(fileLen + sizeof(char));
			if (b == NULL)
			{
				m_error = true;
				return;
			}

			m_error = !ReadFile(m_hfile, b, fileLen, &cb, NULL);
			assert(fileLen == cb);
			if (m_error) return;

			*len = MultiByteToWideChar(CP_UTF8, 0, b, fileLen, NULL, 0);

			*text = (wchar_t *)NewP((*len + 1) * sizeof(wchar_t));
			if (*text == NULL) return;

			MultiByteToWideChar(CP_UTF8, 0, b, fileLen, *text, *len);

			DisposeP((void**)&b);
		}
	}
}

void File::WriteBytes(long numBytes, void *buffer)
{
	if (m_hfile != INVALID_HANDLE_VALUE) {
		DWORD cb = 0;
		m_error = !WriteFile(m_hfile, buffer, numBytes, &cb, NULL);
	}
} // File::WriteBytes

void File::Close(bool truncate) {
	if (!this->m_error && truncate) this->SetPos(this->GetPos(), true);
	if (this->m_hfile != INVALID_HANDLE_VALUE)
		this->m_error = !CloseHandle(this->m_hfile);
	else
		this->m_error = true;
	this->m_hfile = INVALID_HANDLE_VALUE;
} // File::Close

