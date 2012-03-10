// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File.h
//
// v 1.01 (2006.08.04)
//
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
namespace Lib {
// ----------------------------------------------------------------------------
enum EFileAccess {Read = 0x80000000L, Write = 0x40000000L,
				  ReadWrite = 0x80000000L | 0x40000000L};

// ----------------------------------------------------------------------------
class CFile
{
public:
	CFile(const char* FileName, EFileAccess Access);
	bool Write(const byte* SrcData, dword Size);
	bool Read(byte* DstData, dword Size);
	void SetPos(dword Pos);
	void Clear();
	dword GetPos() const;
	dword GetSize() const;
	bool IsOpened() const;
	~CFile();

	static bool Delete(const char* FileName);

private:
	bool m_IsOpen;

	dword m_Pos;
	dword m_Size;
	dword m_Handle;
};
// ----------------------------------------------------------------------------
} // namespace Lib
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=