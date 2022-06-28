// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

class File 
{
public:
	bool Error(void);

	explicit File(void);
	virtual ~File(void);
	virtual void OpenOld(const std::string &fileName);
	virtual void OpenNew(const std::string &fileName);
	virtual int32_t Length(void);
	virtual void SetPos(int32_t pos, bool truncate);
	virtual int32_t GetPos(void);
	virtual void ReadBytes(int32_t numBytes, void *buffer);
	virtual void ReadUnicode(int32_t *len, wchar_t **text);
	virtual void WriteBytes(int32_t numBytes, void *buffer);
	virtual void Close(bool truncate);
private:
	bool m_error;
	void *m_hfile;
	std::string m_fileName; 
};



