// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PT.h
#pragma once
#include "Library/Defs.h"
#include "PTE.h"

// ----------------------------------------------------------------------------
class CPT
{
public:
	CPT()
	{
	}

	CPTE& GetPTE(dword PTEIndex)
	{
		ErrIf(PTEIndex >= 1024);
		return m_PTEArr[PTEIndex];
	}

	dword GetPage(dword PTEIndex)
	{
		ErrIf(PTEIndex >= 1024);
		return m_PTEArr[PTEIndex].GetBase();
	}

	dword GetFreePTEIndex()
	{
		for (dword i = 0; i < 1024; i++)
			if (!m_PTEArr[i].IsPresent())
				return i;
		return -1;
	}
	
	dword GetFreePTECount()
	{
		dword Count = 0;
		for (dword i = 0; i < 1024; i++)
			// !!!!!!!!!!!!!!!!
			if (!m_PTEArr[i].IsPresent() && (m_PTEArr[i].GetAvail() == 0))
				Count++;
		return Count;
	}


	bool IsPTEPresent(dword PTEIndex)
	{
		ErrIf(PTEIndex >= 1024);
		return m_PTEArr[PTEIndex].IsPresent();
	}

	/*
	void WipePageInfoFromPTE(dword PTEIndex)
	{
		ErrIf(PTEIndex >= 1024);
		m_PTEArr[PTEIndex].SetBase(0, false);
		m_PTEArr[PTEIndex].SetPresent(false);
	}
	*/

private:
	CPTE m_PTEArr[1024];
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=