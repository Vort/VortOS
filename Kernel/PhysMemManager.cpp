// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PhysMemManager.cpp
#include "Library/Defs.h"
#include "MemMap.h"
#include "PhysMemManager.h"
#include "OpNewDel.h"
#include "Intrinsics.h"

// ----------------------------------------------------------------------------
CPhysMemManager::CPhysMemManager()
{
	GetFirstNonMappedPageIndex();
	m_TotalPagesCount = m_FirstNonMappedPageIndex - (0x60 + 1);
	m_UsedPagesCount = 4;

	m_PD = new((byte*)CMemMap::c_PmmPageDirectory) CPD();
	CPT* PT = new((byte*)CMemMap::c_PmmFirstPageTable) CPT();

	CPDE& PDE = m_PD->GetPDE(0);
	PDE.SetBase(CMemMap::c_PmmFirstPageTable);
	PDE.SetPresent(true);

	PT->GetPTE(1).SetBase(0x1000);
	PT->GetPTE(2).SetBase(0x2000);
	PT->GetPTE(3).SetBase(0x3000);
	PT->GetPTE(4).SetBase(0x4000);

	PT->GetPTE(1).SetPresent(true);   // 0x001000 - Kernel Stack
	PT->GetPTE(2).SetPresent(true);   // 0x002000 - Kernel Stack
	PT->GetPTE(3).SetPresent(true);   // 0x003000 - Page Directory
	PT->GetPTE(4).SetPresent(true);   // 0x004000 - Page Table

	m_PNANRPageCount = 1024 - GetReservedPageCountInPDE(0) - m_UsedPagesCount;
}

// ----------------------------------------------------------------------------
byte* CPhysMemManager::AllocPage()
{
	dword PTPagePDEIndex = 0;
	dword PTPagePTEIndex = 0;
	CPTE& PTE = FindPNANRPage(PTPagePDEIndex, PTPagePTEIndex);
	byte* PageBase = AllocPageAtPTE(PTE, PTPagePDEIndex, PTPagePTEIndex, true);

	if (m_PNANRPageCount == 1)
		ExpandPresentPageCount();

	return PageBase;
}

// ----------------------------------------------------------------------------
void CPhysMemManager::ReleasePage(byte* Base)
{
	ErrIf((dword(Base) & 0xFFF) != 0);

	dword PDEIndex = dword(Base) >> 22;
	dword PTEIndex = (dword(Base) >> 12) & 0x3FF;

	CPDE& PDE = m_PD->GetPDE(PDEIndex);
	ErrIf(!PDE.IsPresent());

	CPTE& PTE = PDE.GetPT().GetPTE(PTEIndex);
	ErrIf(!PTE.IsPresent());

	PTE.SetPresent(false);
	PTE.Invalidate();

	if (!IsPageReserved(Base))
	{
		m_UsedPagesCount--;
		m_PNANRPageCount++;
	}
}

// ----------------------------------------------------------------------------
byte* CPhysMemManager::AllocBlock(dword PageCount)
{
	dword PTReserveSize = (PageCount >> 10) + 1;
	dword ReqPageCount = PageCount + PTReserveSize;

	dword ReqBlockBase = 0;
	dword ReqBlockFilledCount = 0;

	bool BlockFound = false;

	for (dword PDEIndex = 0; PDEIndex < 1024; PDEIndex++)
	{
		CPDE& PDE = m_PD->GetPDE(PDEIndex);
		if (PDE.IsPresent())
		{
			CPT& PT = PDE.GetPT();
			for (dword PTEIndex = 0; PTEIndex < 1024; PTEIndex++)
			{
				CPTE& PTE = PT.GetPTE(PTEIndex);
				dword Base = (PDEIndex << 22) | (PTEIndex << 12);
				if (IsPageReserved(PB(Base)) || PTE.IsPresent())
				{
					ReqBlockBase = Base + 0x1000;
					ReqBlockFilledCount = 0;
					continue;
				}
				else
				{
					ReqBlockFilledCount++;
					if (ReqBlockFilledCount >= ReqPageCount)
						BlockFound = true;
				}
				if (BlockFound)
					break;
			}
		}
		else
		{
			for (dword PTEIndex = 0; PTEIndex < 1024; PTEIndex++)
			{
				dword Base = (PDEIndex << 22) | (PTEIndex << 12);
				if (IsPageReserved(PB(Base)))
				{
					ReqBlockBase = Base + 0x1000;
					ReqBlockFilledCount = 0;
					continue;
				}
				else
				{
					ReqBlockFilledCount++;
					if (ReqBlockFilledCount >= ReqPageCount)
						BlockFound = true;
				}
				if (BlockFound)
					break;
			}
		}
		if (BlockFound)
			break;
	}

	ErrIf(!BlockFound);

	byte* ActualBlockBase = PB(ReqBlockBase + PTReserveSize * 0x1000);
	for (dword i = 0; i < PageCount; i++)
		AllocPageAt(ActualBlockBase + i * 0x1000, true);

	return ActualBlockBase;
}

// ----------------------------------------------------------------------------
void CPhysMemManager::AllocBlockAt(byte* Base, dword PageCount, bool IsWritable)
{
	for (dword i = 0; i < PageCount; i++)
		AllocPageAt(Base + i * 0x1000, IsWritable);
}

// ----------------------------------------------------------------------------
void CPhysMemManager::ReleaseBlock(byte* Base, dword PageCount)
{
	for (dword i = 0; i < PageCount; i++)
		ReleasePage(Base + i * 0x1000);
}

// ----------------------------------------------------------------------------
dword CPhysMemManager::GetTotalPagesCount()
{
	return m_TotalPagesCount;
}

// ----------------------------------------------------------------------------
dword CPhysMemManager::GetUsedPagesCount()
{
	return m_UsedPagesCount;
}

// ----------------------------------------------------------------------------
void CPhysMemManager::AllocPageAt(byte* Base, bool IsWritable)
{
	ErrIf((dword(Base) & 0xFFF) != 0);

	dword PDEIndex = dword(Base) >> 22;
	dword PTEIndex = (dword(Base) >> 12) & 0x3FF;

	CPDE& PDE = m_PD->GetPDE(PDEIndex);
	if (!PDE.IsPresent())
		 AllocPageTableAt(PDEIndex);

	CPTE& PTE = PDE.GetPT().GetPTE(PTEIndex);
	ErrIf(PTE.IsPresent());

	AllocPageAtPTE(PTE, PDEIndex, PTEIndex, IsWritable);

	if (m_PNANRPageCount == 1)
		ExpandPresentPageCount();
}

// ----------------------------------------------------------------------------
void CPhysMemManager::AllocPageTableAt(dword PDEIndex)
{
	if (m_PNANRPageCount == 1)
		ExpandPresentPageCount();

	CPDE& PDE = m_PD->GetPDE(PDEIndex);
	if (PDE.IsPresent())
		return;

	dword PTPagePDEIndex = 0;
	dword PTPagePTEIndex = 0;
	CPTE& PTE = FindPNANRPage(PTPagePDEIndex, PTPagePTEIndex);

	byte* PTPage = AllocPageAtPTE(PTE, PTPagePDEIndex, PTPagePTEIndex, true);
	CPT* PT = new(PTPage) CPT();

	PDE.SetBase(dword(PTPage));
	PDE.SetPresent(true);

	m_PNANRPageCount += 1024 - GetReservedPageCountInPDE(PDEIndex);
}

// ----------------------------------------------------------------------------
void CPhysMemManager::ExpandPresentPageCount()
{
	for (dword PDEIndex = 0; PDEIndex < 1024; PDEIndex++)
	{
		dword ReservedPageCountInPDE = GetReservedPageCountInPDE(PDEIndex);
		CPDE& PDE = m_PD->GetPDE(PDEIndex);
		if (!PDE.IsPresent() && (ReservedPageCountInPDE != 0x400))
		{
			dword PTPagePDEIndex = 0;
			dword PTPagePTEIndex = 0;
			CPTE& PTE = FindPNANRPage(PTPagePDEIndex, PTPagePTEIndex);

			byte* PTPage = AllocPageAtPTE(PTE, PTPagePDEIndex, PTPagePTEIndex, true);
			CPT* PT = new(PTPage) CPT();

			PDE.SetBase(dword(PTPage));
			PDE.SetPresent(true);

			m_PNANRPageCount += 1024 - ReservedPageCountInPDE;

			if (m_PNANRPageCount > 1)
				return;
		}
	}
	ErrIf(true);
}

// ----------------------------------------------------------------------------
byte* CPhysMemManager::AllocPageAtPTE(CPTE& PTE, dword PDEIndex, dword PTEIndex, bool IsWritable)
{
	ErrIf(PTE.IsPresent());

	dword Base = (PDEIndex << 22) | (PTEIndex << 12);

	PTE.SetBase(Base);
	PTE.SetWritable(IsWritable);
	PTE.SetPresent(true);
	PTE.Invalidate();

	if (!IsPageReserved(PB(Base)))
	{
		m_UsedPagesCount++;
		m_PNANRPageCount--;
	}

	return PB(Base);
}

// ----------------------------------------------------------------------------
CPTE& CPhysMemManager::FindPNANRPage(dword& OutPDEIndex, dword& OutPTEIndex)
{
	for (dword PDEIndex = 0; PDEIndex < 1024; PDEIndex++)
	{
		CPDE& PDE = m_PD->GetPDE(PDEIndex);
		// P
		if (PDE.IsPresent())
		{
			CPT& PT = PDE.GetPT();
			for (dword PTEIndex = 0; PTEIndex < 1024; PTEIndex++)
			{
				// NR
				if (!IsPageReserved(PDEIndex, PTEIndex))
				{
					CPTE& PTE = PT.GetPTE(PTEIndex);
					// NA
					if (!PTE.IsPresent())
					{
						OutPDEIndex = PDEIndex;
						OutPTEIndex = PTEIndex;
						return PTE;
					}
				}
			}
		}
	}
	ErrIf(true);
}

// ----------------------------------------------------------------------------
dword CPhysMemManager::GetReservedPageCountInPDE(dword PDEIndex)
{
	dword R = 0;
	dword Base = PDEIndex << 22;

	for (dword i = 0; i < 1024; i++)
		R += IsPageReserved(PB(Base + i * 0x1000));

	return R;
}

// ----------------------------------------------------------------------------
bool CPhysMemManager::IsPageReserved(byte* Base)
{
	dword DBase = (dword)Base;
	ErrIf((DBase & 0xFFF) != 0);

	if (DBase == 0)
		return true;

	if ((DBase >= 0x000A0000) && (DBase < 0x00100000))
		return true;

	if (DBase >= m_FirstNonMappedPageIndex * 0x1000)
		return true;

	return false;
}

// ----------------------------------------------------------------------------
bool CPhysMemManager::IsPageReserved(dword PDEIndex, dword PTEIndex)
{
	return IsPageReserved(PB((PDEIndex << 22) | (PTEIndex << 12)));
}

// ----------------------------------------------------------------------------
void CPhysMemManager::GetFirstNonMappedPageIndex()
{
	m_FirstNonMappedPageIndex = 256;
	volatile byte* Page;
	for (Page = PB(0x100000); ; Page += 0x1000)
	{
		byte OldValue = *Page;
		*Page ^= 0x5A;
		if (*Page == OldValue) break;
		*Page ^= 0x5A;
	}
	m_FirstNonMappedPageIndex = dword(Page) >> 12;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=