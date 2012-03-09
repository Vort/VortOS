// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CallRequest.h
#include "Library/Defs.h"
#include "Library/Array2.h"
#pragma once

// ----------------------------------------------------------------------------
class CCallRequest
{
public:
	CCallRequest(dword TypeID, dword CltPID, const byte* Data, dword DataSize)
		: m_TID(TypeID), m_Data(Data, DataSize)
	{
		m_CltPID = CltPID;
	}

	dword GetTypeID()
	{
		return m_TID;
	}

	dword GetCltPID()
	{
		return m_CltPID;
	}

	dword GetSize()
	{
		return m_Data.Size();
	}

	CArray<byte>& GetData()
	{
		return m_Data;
	}

private:
	dword m_TID;
	dword m_CltPID;
	CArray<byte> m_Data;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=