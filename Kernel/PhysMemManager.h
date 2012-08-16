// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PhysMemManager.h
#pragma once
#include "Library/Defs.h"
#include "PD.h"

// ----------------------------------------------------------------------------
class CPhysMemManager
{
public:
	CPhysMemManager();

	byte* AllocPage();
	void ReleasePage(byte* Base);

	byte* AllocBlock(dword PageCount);
	void AllocBlockAt(byte* Base, dword PageCount, bool IsWritable);
	void ReleaseBlock(byte* Base, dword PageCount);

	dword GetTotalPagesCount();
	dword GetUsedPagesCount();

private:
	void AllocPageAt(byte* Base, bool IsWritable);
	void AllocPageTableAt(dword PDEIndex);
	void ExpandPresentPageCount();

	byte* AllocPageAtPTE(CPTE& PTE, dword PDEIndex, dword PTEIndex, bool IsWritable);
	CPTE& FindPNANRPage(dword& OutPDEIndex, dword& OutPTEIndex);

	dword GetReservedPageCountInPDE(dword PDEIndex);
	bool IsPageReserved(byte* Base);
	bool IsPageReserved(dword PDEIndex, dword PTEIndex);

	void GetFirstNonMappedPageIndex();

private:
	CPD* m_PD;

	dword m_PNANRPageCount;
	dword m_FirstNonMappedPageIndex;

	dword m_UsedPagesCount;
	dword m_TotalPagesCount;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=