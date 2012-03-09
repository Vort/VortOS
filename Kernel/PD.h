// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PD.h
#pragma once
#include "Library/Defs.h"
#include "PDE.h"

// ----------------------------------------------------------------------------
class CPD
{
public:
	CPD()
	{
	}

	dword ConvertBaseToPDEIndex(dword Base)
	{
		return Base >> 22;
	}

	dword ConvertBaseToPTEIndex(dword Base)
	{
		return (Base >> 12) & 0x3FF;
	}

	dword ConvertBaseToGlobalIndex(dword Base)
	{
		return Base >> 12;
	}

	dword GetPDEIndexFromGlobalIndex(dword GlobalIndex)
	{
		return GlobalIndex >> 10;
	}

	dword GetPTEIndexFromGlobalIndex(dword GlobalIndex)
	{
		return GlobalIndex & 0x3FF;
	}

	dword GetGlobalIndexFromPDEPTEIndex(dword PDEIndex, dword PTEIndex)
	{
		return (PDEIndex << 10) | PTEIndex;
	}


	CPDE& GetPDE(dword PDEIndex)
	{
		ErrIf(PDEIndex >= 1024);
		return m_PDEArr[PDEIndex];
	}

	CPTE& GetPTE(dword GlobalIndex)
	{
		ErrIf(GlobalIndex >= 1024*1024);
		return m_PDEArr[GetPDEIndexFromGlobalIndex(GlobalIndex)].GetPT().GetPTE(
			GetPTEIndexFromGlobalIndex(GlobalIndex));
	}

	CPT& GetPageTable(dword PDEIndex)
	{
		return GetPDE(PDEIndex).GetPT();
	}

	dword GetPage(dword GlobalIndex)
	{
		return GetPTE(GlobalIndex).GetBase();
	}

	dword GetFreePDEIndex()
	{
		for (dword i = 0; i < 1024; i++)
			if (!m_PDEArr[i].IsPresent())
				return i;
		return -1;
	}

	dword GetFreePTEGlobalIndex()
	{
		for (dword i = 0; i < 1024; i++)
			if (m_PDEArr[i].GetPT().GetFreePTECount())
				return GetGlobalIndexFromPDEPTEIndex(i, m_PDEArr[i].GetPT().GetFreePTEIndex());
		return -1;
	}

	dword GetFreePTECountInAllChildPTs()
	{
		dword Count = 0;
		for (dword i = 0; i < 1024; i++)
			if (m_PDEArr[i].IsPresent())
				Count += m_PDEArr[i].GetPT().GetFreePTECount();
		return Count;
	}

	bool IsPDEPresent(dword PDEIndex)
	{
		ErrIf(PDEIndex >= 1024);
		return m_PDEArr[PDEIndex].IsPresent();
	}

	bool IsPTEPresent(dword GlobalIndex)
	{
		ErrIf(GlobalIndex >= 1024*1024);
		if (GetPDE(GetPDEIndexFromGlobalIndex(GlobalIndex)).IsPresent())
			if (GetPTE(GlobalIndex).IsPresent())
				return true;
		return false;
	}


	void WipePTInfoFromPDE(dword PDEIndex)
	{
		ErrIf(PDEIndex >= 1024);
		m_PDEArr[PDEIndex].SetBase(0);
		m_PDEArr[PDEIndex].SetPresent(false);
	}

	/*
	void WipePageInfoFromPTE(dword GlobalIndex)
	{
		ErrIf(!IsPTEPresent(GlobalIndex));
		GetPageTable(GetPDEIndexFromGlobalIndex(GlobalIndex)).WipePageInfoFromPTE(
			GetPTEIndexFromGlobalIndex(GlobalIndex));
	}
	*/

private:
	CPDE m_PDEArr[1024];
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=