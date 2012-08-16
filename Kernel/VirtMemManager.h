// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// VirtMemManager.h
#include "Library/Array2.h"

#include "PhysMemManager.h"
#include "OpNewDel.h"
#pragma once

// ----------------------------------------------------------------------------
class CVirtMemManager
{
public:
	CVirtMemManager(CPhysMemManager& PMM, dword allocBaseMin, dword allocBaseMax)
		: m_PMM(PMM)
	{
		m_PD = new (_allocPage()) CPD();
		m_AllocBaseMin = allocBaseMin;
		m_AllocBaseMax = allocBaseMax;
		ErrIf(allocBaseMin & 0x3FFFFF);
		ErrIf(allocBaseMax & 0x3FFFFF);
	}

	~CVirtMemManager()
	{
		for (dword i = 0; i < m_AllocatedPages.Size(); i++)
			m_PMM.ReleasePage(PB(m_AllocatedPages[i]));
	}

	dword GetPagesCount()
	{
		return m_AllocatedPages.Size();
	}

	void MapPageAt(dword physBase, dword virtBase, bool isWriteEnabled)
	{
		dword alPhysBase = physBase & ~0xFFF;
		dword virtPDEIndex = m_PD->ConvertBaseToPDEIndex(virtBase);
		dword virtPTEIndex = m_PD->ConvertBaseToPTEIndex(virtBase);

		CPDE& PDE = m_PD->GetPDE(virtPDEIndex);
		if (!PDE.IsPresent())
		{
			dword NewPageTableBase = (dword)_allocPage();
			new (PV(NewPageTableBase)) CPT();
			PDE.SetBase(NewPageTableBase);
			PDE.SetPresent(true);
		}
		CPTE& PTE = PDE.GetPT().GetPTE(virtPTEIndex);
		
		ErrIf(PTE.IsPresent());
		PTE.SetBase(physBase);
		PTE.SetPresent(true);
		PTE.SetWritable(isWriteEnabled);
	}

	void MapBlockAt(dword physBase, dword virtBase, dword pageCount, bool isWriteEnabled)
	{
		for (dword i = 0; i < pageCount; i++)
			MapPageAt(physBase + i * 4096, virtBase + i * 4096, isWriteEnabled);
	}

	dword MapBlock(dword* pages, dword pageCount)
	{
		dword minPdeIndex = m_PD->ConvertBaseToPDEIndex(m_AllocBaseMin);
		dword maxPdeIndex = m_PD->ConvertBaseToPDEIndex(m_AllocBaseMax);

		dword blockSize = -1;
		dword blockStartIndex = -1;

		for (int i = 0; i < 2; i++)
		{
			for (int pdeIndex = minPdeIndex; pdeIndex < maxPdeIndex; pdeIndex++)
			{
				CPDE& pde = m_PD->GetPDE(pdeIndex);
				for (int pteIndex = 0; pteIndex < 1024; pteIndex++)
				{
					int expandCount = -1;
					if (!pde.IsPresent())
					{
						if (i == 1)
							expandCount = 1024;
					}
					else
					{
						CPTE& pte = pde.GetPT().GetPTE(pteIndex);
						if (!pte.IsPresent())
							expandCount = 1;
					}
					if (expandCount == -1)
					{
						blockSize = -1;
						blockStartIndex = -1;
					}
					else
					{
						if (blockStartIndex == -1)
						{
							blockStartIndex = m_PD->GetGlobalIndexFromPDEPTEIndex(pdeIndex, pteIndex);
							blockSize = 0;
						}
						blockSize += expandCount;
						if (blockSize >= pageCount)
						{
							for (int j = 0; j < pageCount; j++)
							{
								MapPageAt(pages[j], (blockStartIndex + j) * 0x1000, true);
							}
							return blockStartIndex * 0x1000;
						}
					}
					if (!pde.IsPresent())
						break;
				}
			}
		}

		ErrIf(true);
	}

	void UnmapBlock(dword virtBase, dword pageCount)
	{
		dword alVirtBase = virtBase & ~0xFFF;
		for (dword i = 0; i < pageCount; i++)
		{
			dword virtPDEIndex = m_PD->ConvertBaseToPDEIndex(alVirtBase);
			dword virtPTEIndex = m_PD->ConvertBaseToPTEIndex(alVirtBase);
			CPDE& PDE = m_PD->GetPDE(virtPDEIndex);
			if (PDE.IsPresent())
				PDE.GetPT().GetPTE(virtPTEIndex).SetPresent(false);
			alVirtBase += 0x1000;
		}
	}

	bool MemCopyByteVirtToPhys(byte* VirtByte, byte& Byte)
	{
		dword HiAddr = dword(VirtByte) >> 12;
		dword LoAddr = dword(VirtByte) & 0xFFF;
		CPTE& PTE = m_PD->GetPTE(HiAddr);
		if (PTE.IsPresent())
		{
			Byte = *PB(PTE.GetBase() + LoAddr);
			return true;
		}
		else
			return false;
	}

	bool MemCopyVirtToPhys(byte* VirtData, byte* PhysData, dword Size)
	{
		return _interMemCopy(PhysData, VirtData, Size, true);
	}

	bool MemCopyPhysToVirt(byte* PhysData, byte* VirtData, dword Size)
	{
		return _interMemCopy(PhysData, VirtData, Size, false);
	}

	bool TranslateVirtToPhys(byte* Virt, byte*& Phys)
	{
		dword VirtGI = m_PD->ConvertBaseToGlobalIndex(dword(Virt));
		CPTE& PTE = m_PD->GetPTE(VirtGI);
		if (PTE.IsPresent())
		{
			Phys = PTE.GetBase() + (Virt - (VirtGI << 12));
			return true;
		}
		else
			return false;
	}

	bool CheckArray(byte* VirtPtr, dword Size)
	{
		if (Size == 0)
			return true;

		byte* FirstVirtByte = VirtPtr;
		byte* LastVirtByte = VirtPtr + Size - 1;
		dword FirstVirtBytePage = dword(FirstVirtByte) >> 12;
		dword LastVirtBytePage = dword(LastVirtByte) >> 12;
		dword FirstVirtByteOffset = dword(FirstVirtByte) & 0xFFF;
		dword LastVirtByteOffset = dword(LastVirtByte) & 0xFFF;

		if (FirstVirtBytePage == LastVirtBytePage)
		{
			CPTE& PTE = m_PD->GetPTE(FirstVirtBytePage);
			if (!PTE.IsPresent())
				return false;
		}
		else
		{
			dword FullPagesCount = LastVirtBytePage - FirstVirtBytePage - 1;

			CPTE& FPTE = m_PD->GetPTE(FirstVirtBytePage);
			dword FirstBytesCount = 4096 - FirstVirtByteOffset;
			if (!FPTE.IsPresent())
				return false;

			CPTE& LPTE = m_PD->GetPTE(LastVirtBytePage);
			if (!LPTE.IsPresent())
				return false;

			for (dword p = 0; p < FullPagesCount; p++)
			{
				CPTE& MPTE = m_PD->GetPTE(FirstVirtBytePage + p + 1);
				if (!MPTE.IsPresent())
					return false;
			}
		}
		return true;
	}


	CPD& GetPD()
	{
		return *m_PD;
	}

private:
	bool _interMemCopy(byte* PhysData, byte* VirtData,
		dword Size, bool IsVirtToPhys)
	{
		if (Size == 0)
			return true;

		byte* FirstVirtByte = VirtData;
		byte* LastVirtByte = VirtData + Size - 1;
		dword FirstVirtBytePage = dword(FirstVirtByte) >> 12;
		dword LastVirtBytePage = dword(LastVirtByte) >> 12;
		dword FirstVirtByteOffset = dword(FirstVirtByte) & 0xFFF;
		dword LastVirtByteOffset = dword(LastVirtByte) & 0xFFF;

		if (FirstVirtBytePage == LastVirtBytePage)
		{
			CPTE& PTE = m_PD->GetPTE(FirstVirtBytePage);
			if (PTE.IsPresent())
			{
				byte* FirstVirtPhysByte = PB(PTE.GetBase() + FirstVirtByteOffset);
				if (IsVirtToPhys)
				{
					for (dword i = 0; i < Size; i++)
						PhysData[i] = FirstVirtPhysByte[i];
				}
				else
				{
					for (dword i = 0; i < Size; i++)
						FirstVirtPhysByte[i] = PhysData[i];
				}
			}
			else
				return false;
		}
		else
		{
			dword FullPagesCount = LastVirtBytePage - FirstVirtBytePage - 1;
			CPTE& FPTE = m_PD->GetPTE(FirstVirtBytePage);
			dword FirstBytesCount = 4096 - FirstVirtByteOffset;
			if (FPTE.IsPresent())
			{
				byte* FirstVirtPhysByte = PB(FPTE.GetBase() + FirstVirtByteOffset);
				if (IsVirtToPhys)
				{
					for (dword i = 0; i < FirstBytesCount; i++)
						PhysData[i] = FirstVirtPhysByte[i];
				}
				else
				{
					for (dword i = 0; i < FirstBytesCount; i++)
						FirstVirtPhysByte[i] = PhysData[i];
				}
			}
			else
				return false;
			CPTE& LPTE = m_PD->GetPTE(LastVirtBytePage);
			if (LPTE.IsPresent())
			{
				byte* LastVirtPhysByte = PB(LPTE.GetBase());
				dword BytesCount = LastVirtByteOffset + 1;
				dword Delta = FirstBytesCount + 4096 * FullPagesCount;
				if (IsVirtToPhys)
				{
					for (dword i = 0; i < BytesCount; i++)
						PhysData[Delta+i] = LastVirtPhysByte[i];
				}
				else
				{
					for (dword i = 0; i < BytesCount; i++)
						LastVirtPhysByte[i] = PhysData[Delta+i];
				}
			}
			else
				return false;
			for (dword p = 0; p < FullPagesCount; p++)
			{
				CPTE& MPTE = m_PD->GetPTE(FirstVirtBytePage + p + 1);
				if (MPTE.IsPresent())
				{
					byte* MiddleVirtPhysByte = PB(MPTE.GetBase());
					dword Delta = FirstBytesCount + 4096 * p;
					if (IsVirtToPhys)
					{
						for (dword i = 0; i < 4096; i++)
							PhysData[Delta+i] = MiddleVirtPhysByte[i];
					}
					else
					{
						for (dword i = 0; i < 4096; i++)
							MiddleVirtPhysByte[i] = PhysData[Delta+i];
					}
				}
				else
					return false;
			}
		}
		return true;
	}

	void* _allocPage()
	{
		void* Page = m_PMM.AllocPage();
		m_AllocatedPages.PushBack(dword(Page));
		return Page;
	}

private:
	CPhysMemManager& m_PMM;
	CPD* m_PD;

	dword m_AllocBaseMin;
	dword m_AllocBaseMax;

	CArray<dword> m_AllocatedPages;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=