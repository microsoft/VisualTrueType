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
	virtual int Length(void);
	virtual void SetPos(int pos, bool truncate);
	virtual int GetPos(void);
	virtual void ReadBytes(int numBytes, void *buffer);
	virtual void ReadUnicode(int *len, wchar_t **text);
	virtual void WriteBytes(int numBytes, void *buffer);
	virtual void Close(bool truncate);
private:
	bool m_error;
	void *m_hfile;
	std::string m_fileName; 
};



