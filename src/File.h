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
	virtual long Length(void);
	virtual void SetPos(long pos, bool truncate);
	virtual long GetPos(void);
	virtual void ReadBytes(long numBytes, void *buffer);
	virtual void ReadUnicode(long *len, wchar_t **text);
	virtual void WriteBytes(long numBytes, void *buffer);
	virtual void Close(bool truncate);
private:
	bool m_error;
	void *m_hfile;
	std::string m_fileName; 
};



