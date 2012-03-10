// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// File.cpp
//
// v 1.01 (2006.08.04)
//
#include "Defs.h"
#include "File.h"
#include <windows.h>

// ----------------------------------------------------------------------------
using namespace Lib;

// ----------------------------------------------------------------------------
CFile::CFile(const char* FileName, EFileAccess Access)
{
	dword OpenType = OPEN_ALWAYS;
	if (Access == ::Read) OpenType = OPEN_EXISTING;
	HANDLE Handle = CreateFileA(FileName, Access, 0,
		null, OpenType, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (Handle == INVALID_HANDLE_VALUE)
	{
		m_IsOpen = false;
		m_Handle = 0;
		m_Size = 0;
	}
	else
	{
		m_IsOpen = true;
		m_Handle = dword((size_t)Handle);
		m_Size = GetFileSize((HANDLE)(size_t)Handle, null);
	}
	m_Pos = 0;
}

// ----------------------------------------------------------------------------
bool CFile::IsOpened() const
{
	return m_IsOpen;
}

// ----------------------------------------------------------------------------
bool CFile::Write(const byte* SrcData, dword Size)
{
	ErrIf(!m_IsOpen);
	DWORD Written;
	if (m_Pos + Size > m_Size) m_Size = m_Pos + Size;
	dword Result = WriteFile((HANDLE)(size_t)m_Handle,
		SrcData, Size, &Written, null);
	m_Pos += Written;
	return Result != 0;
}

// ----------------------------------------------------------------------------
bool CFile::Read(byte* DstData, dword Size)
{
	ErrIf(!m_IsOpen);
	DWORD WasRead;
	dword Result = ReadFile((HANDLE)(size_t)m_Handle,
		DstData, Size, &WasRead, null);
	m_Pos += WasRead;
	return Result != 0;
}

// ----------------------------------------------------------------------------
void CFile::Clear()
{
	// [!] Необходима проверка типа доступа (R/W)
	SetPos(0);
	SetEndOfFile((HANDLE)(size_t)m_Handle);
	m_Size = 0;
}

// ----------------------------------------------------------------------------
void CFile::SetPos(dword Pos)
{
	ErrIf(!m_IsOpen);
	ErrIf(Pos > m_Size);
	m_Pos = Pos;
	SetFilePointer((HANDLE)(size_t)m_Handle, Pos, 0, FILE_BEGIN);
}

// ----------------------------------------------------------------------------
dword CFile::GetPos() const
{
	ErrIf(!m_IsOpen);
	return m_Pos;
}

// ----------------------------------------------------------------------------
dword CFile::GetSize() const
{
	ErrIf(!m_IsOpen);
	return m_Size;
}

// ----------------------------------------------------------------------------
CFile::~CFile()
{
	if (m_IsOpen)
		CloseHandle((HANDLE)(size_t)m_Handle);
}

// ----------------------------------------------------------------------------
bool CFile::Delete(const char* FileName)
{
	// [?] Необходима проверка на правильность хендла
	return DeleteFileA(FileName) != 0;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=