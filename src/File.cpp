// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//#define NOMINMAX
//#include <Windows.h>
#define _CRT_SECURE_NO_WARNINGS 

#include <assert.h>

#include "pch.h"
#include <sys/stat.h>

#define IsUnicode(b) ((b[0] == 0xFF) && (b[1] == 0xFE))
#define IsUnicodeBig(b) ((b[0] == 0xFE) && (b[1] == 0xFF))
#define IsUTF8(b) ((b[0] == 0xEF) && (b[1] == 0xBB) && (b[2] == 0xBF)) 

bool File::Error() {
	return m_error;
} // File::Error

File::File(void) {
	// this->m_hfile = INVALID_HANDLE_VALUE;
	this->m_hfile = nullptr; 

} // File::File

File::~File(void) {
	//if (this->m_hfile != INVALID_HANDLE_VALUE)
	//	this->Close(false);
	if (this->m_hfile != nullptr)
		this->Close(false);
} // File::~File

void File::OpenOld(const std::string& fileName) {
	//m_hfile = CreateFileA(fileName, GENERIC_READ /* | GENERIC_WRITE */, FILE_SHARE_READ, NULL,
	//	OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	//m_error = (m_hfile == INVALID_HANDLE_VALUE);
	m_fileName = fileName; 
	m_hfile = fopen(fileName.c_str(), "rb"); 
	m_error = (m_hfile == nullptr); 

} // File::OpenOld

void File::OpenNew(const std::string& fileName) {
	//m_hfile = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
	//	CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	//m_error = (m_hfile == INVALID_HANDLE_VALUE);
	m_fileName = fileName;
	m_hfile = fopen(fileName.c_str(), "wb");
	m_error = (m_hfile == nullptr); 
} // File::OpenNew

int32_t File::Length(void) {
	//return (m_hfile != INVALID_HANDLE_VALUE) ? GetFileSize(m_hfile, NULL) : 0;
	struct stat st; 
	stat(m_fileName.c_str(), &st); 
	return static_cast<int32_t>(st.st_size);
} // File::Length

void File::SetPos(int32_t pos, bool truncate) {
	//SetFilePointer(m_hfile, pos, NULL, FILE_BEGIN);
	//if (truncate)
	//	SetEndOfFile(m_hfile);
	fseek((FILE*)m_hfile, SEEK_SET, pos); 
} // File::SetPos

int32_t File::GetPos(void) {
	return static_cast<int32_t>(ftell((FILE*)m_hfile));
} // File::GetPos

void File::ReadBytes(int32_t numBytes, void *buffer) {
	//if (m_hfile != INVALID_HANDLE_VALUE) {
	//	DWORD cb = 0;
	//	m_error = !ReadFile(m_hfile, buffer, numBytes, &cb, NULL);
	//}
	size_t size = 0; 
	if (m_hfile != nullptr)
	{
		size = fread(buffer, 1, numBytes, (FILE*)m_hfile); 
	}
} // File::ReadBytes

void File::ReadUnicode(int32_t *len, wchar_t **text)
{
	assert(false); 
	/*if (m_hfile != INVALID_HANDLE_VALUE) {
		DWORD cb;
		BYTE start[4];

		m_error = !ReadFile(m_hfile, start, 4, &cb, NULL);
		assert(4 == cb);
		if (m_error) return;

		int32_t fileLen = this->Length();

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
				for (int32_t i = 0; i < *len; i++)
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
	*/ 
}

void File::WriteBytes(int32_t numBytes, void *buffer)
{
	//if (m_hfile != INVALID_HANDLE_VALUE) {
	//	DWORD cb = 0;
	//	m_error = !WriteFile(m_hfile, buffer, numBytes, &cb, NULL);
	//}
	size_t size = 0; 
	if (m_hfile != nullptr)
	{
		size = fwrite(buffer, 1, numBytes, (FILE*)m_hfile); 
	}
} // File::WriteBytes

void File::Close(bool truncate) {
	//if (!this->m_error && truncate) this->SetPos(this->GetPos(), true);
	//if (this->m_hfile != INVALID_HANDLE_VALUE)
	//	this->m_error = !CloseHandle(this->m_hfile);
	//else
	//	this->m_error = true;
	if (m_hfile != nullptr)
	{
		fclose((FILE*)m_hfile); 
	}
	this->m_hfile = nullptr;
} // File::Close

