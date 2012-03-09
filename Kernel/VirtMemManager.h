// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// VirtMemManager.h
#pragma once
#include "Library/Array2.h"

#include "PhysMemManager.h"
#include "OpNewDel.h"
#pragma once

// ----------------------------------------------------------------------------
class CVirtMemManager
{
public:
	CVirtMemManager(CPhysMemManager& PMM, dword FreeBlocksBase)
		: m_PMM(PMM)
	{
		m_PD = new (_allocPage()) CPD();
		m_FreeBlock = FreeBlocksBase;
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

	void MapPhysPageToVirtPage(dword PhysBase, dword VirtBase, bool IsWriteEnabled = true)
	{
		dword AlPhysBase = PhysBase & ~0xFFF;
		dword VirtPDEIndex = m_PD->ConvertBaseToPDEIndex(VirtBase);
		dword VirtPTEIndex = m_PD->ConvertBaseToPTEIndex(VirtBase);

		CPDE& PDE = m_PD->GetPDE(VirtPDEIndex);
		if (!PDE.IsPresent())
		{
			dword NewPageTableBase = (dword)_allocPage();
			new (PV(NewPageTableBase)) CPT();
			PDE.SetBase(NewPageTableBase);
			PDE.SetPresent(true);
		}
		CPTE& PTE = PDE.GetPT().GetPTE(VirtPTEIndex);
		
		ErrIf(PTE.IsPresent());
		PTE.SetBase(PhysBase, false);
		PTE.SetPresent(true, false);
		PTE.SetWrite(IsWriteEnabled, false);
	}

	void MapPhysBlockToVirtBlock(dword PhysBase, dword VirtBase,
		dword PageCount, bool IsWriteEnabled = true)
	{
		for (dword i = 0; i < PageCount; i++)
			MapPhysPageToVirtPage(PhysBase + i*4096, VirtBase + i*4096, IsWriteEnabled);
	}

	dword MapNextPage(dword PhysBase)
	{
		MapPhysPageToVirtPage(PhysBase, m_FreeBlock);
		dword R = m_FreeBlock;
		m_FreeBlock += 0x1000;
		return R;
	}

	void UnmapVirtBlock(dword VirtBase, dword PageCount)
	{
		dword AlVirtBase = VirtBase & ~0xFFF;
		for (dword i = 0; i < PageCount; i++)
		{
			dword VirtPDEIndex = m_PD->ConvertBaseToPDEIndex(AlVirtBase);
			dword VirtPTEIndex = m_PD->ConvertBaseToPTEIndex(AlVirtBase);
			CPDE& PDE = m_PD->GetPDE(VirtPDEIndex);
			if (PDE.IsPresent())
				PDE.GetPT().GetPTE(VirtPTEIndex).SetPresent(false, true);
			AlVirtBase += 0x1000;
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

	dword m_FreeBlock;

	CArray<dword> m_AllocatedPages;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=