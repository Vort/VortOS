// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// main.cpp
#include "CRC32.h"

#include "MemMap.h"
#include "Kernel.h"
#include "Global.h"

#include "BitOp.h"
#include "Intrinsics.h"

// ----------------------------------------------------------------------------
void RawOutString(const char* Msg, dword X, dword Y, byte Color)
{
	if (X > 79) return;
	if (Y > 24) return;
	char* Buf = (char*)CMemMap::c_VideoRamTextBase;
	Buf += (Y * 80 + X) * 2;
	for (dword i = 0; Msg[i] != 0; i++)
	{
		*Buf++ = Msg[i];
		*Buf++ = Color;
	}
}

// ----------------------------------------------------------------------------
class CKernelHeader
{
public:
	byte* GetKernelCodeBase()
	{
		return (byte*)CMemMap::c_KernelCodeBase;
	}

	byte* GetKernelRDataBase()
	{
		return (byte*)(CMemMap::c_KernelCodeBase + (m_KernelCodePageCount << 12));
	}

	byte* GetKernelDataBase()
	{
		return (byte*)(GetKernelRDataBase() + (m_KernelRDataPageCount << 12));
	}

	dword GetKernelPagesCount()
	{
		return 
			m_KernelCodePageCount +
			m_KernelRDataPageCount +
			m_KernelDataPageCount;
	}

public:
	dword m_KernelMagic;
	dword m_KernelHeaderVer;
	dword m_KernelEntryPoint;
	dword m_KernelCodePageCount;
	dword m_KernelRDataPageCount;
	dword m_KernelDataPageCount;
	dword m_KernelImageCRC32;
};

// ----------------------------------------------------------------------------
bool IsKernelOK(CKernelHeader& KH, dword* BootInfo)
{
	if (KH.m_KernelMagic != 'nrKV')
		return false;

	if (KH.m_KernelHeaderVer != 2)
		return false;

	dword CRC32Comp = CCRC32().GetCRC32(
		KH.GetKernelCodeBase(), 0,
		KH.GetKernelPagesCount() * 4096);

	if (CRC32Comp != KH.m_KernelImageCRC32)
		return false;

	if (BootInfo[0] != 'fnil')
		return false;

	return true;
}

// ----------------------------------------------------------------------------
void SetupCR0()
{
	dword cr0 = __readcr0();
	SetBit(cr0, 31);   // Set PG 1 (Enable Paging)
	ClearBit(cr0, 30); // Set CD 0 (Enable Cache)
	ClearBit(cr0, 29); // Set NW 0 (Enable Writethrough)
	SetBit(cr0, 16);   // Set WP 1 (Enable Write Protect)
	SetBit(cr0, 5);    // Set NE 1 (Enable Internal FP Mode)
	__writecr0(cr0);
}

// ----------------------------------------------------------------------------
void InitPIT()
{
	dword Period = 1193180 / 20; // 20 Hz

	_outp(0x43, 0x36);
	_outp(0x40, (Period >> 0) & 0xFF);
	_outp(0x40, (Period >> 8) & 0xFF);
}

// ----------------------------------------------------------------------------
void Entry()
{
	// Move cursor off-screen
	_outpw(0x3D4, 0x070E);
	_outpw(0x3D4, 0xD00F);

	CKernelHeader* KH = (CKernelHeader*)CMemMap::c_KernelImageBase;
	dword* BootInfo = (dword*)(*(dword*)(CMemMap::c_KernelImageBase + 0x100));

	RawOutString("Checking Kernel...", 0, 0, 0xA);
	if (IsKernelOK(*KH, BootInfo))
	{
		RawOutString("OK", 18, 0, 0xA);
	}
	else
	{
		RawOutString("Fail", 18, 0, 0xC);
		ErrIf(true);
	}

	dword BootType = BootInfo[1];
	dword DriversCount = BootInfo[2] - 1;
	CDriverInfo DriverInfos[12];

	for (dword i = 0; i < DriversCount; i++)
	{
		DriverInfos[i].m_BytesSize = BootInfo[3 + (i + 1) * 3];
		DriverInfos[i].m_LoadPage  = BootInfo[4 + (i + 1) * 3];
		char* Name = PC(BootInfo[5 + (i + 1) * 3]);

		for (int j = 0;; j++)
		{
			DriverInfos[i].m_Name[j] = Name[j];
			if (Name[j] == 0)
				break;
		}
	}

	InitPIT();

	CPhysMemManager PMM;
	PMM.AllocBlockAt(KH->GetKernelCodeBase(), KH->m_KernelCodePageCount, false);
	PMM.AllocBlockAt(KH->GetKernelRDataBase(), KH->m_KernelRDataPageCount, false);
	PMM.AllocBlockAt(KH->GetKernelDataBase(), KH->m_KernelDataPageCount, true);
	PMM.AllocBlockAt((byte*)CMemMap::c_VideoRamTextBase, 8, true);
	__writecr3(CMemMap::c_PmmPageDirectory);
	SetupCR0();

	for (dword i = 0; i < DriversCount; i++)
	{
		PMM.AllocBlockAt(
			DriverInfos[i].GetImageBase(),
			DriverInfos[i].GetImagePageCount(), false);
	}

	CGDT GDT(PMM);
	GDT.CreateNewDescriptor(0, 0xFFFFF, 0x92, 1);
	GDT.CreateNewDescriptor(0, 0xFFFFF, 0x98, 1);
	GDT.CreateNewDescriptor(0, 0xFFFFF, 0xF2, 1);
	GDT.CreateNewDescriptor(0, 0xFFFFF, 0xF8, 1);

	static const dword HeapPageCount = 32;
	void* HeapBlock = PMM.AllocBlock(HeapPageCount);
	CHeap SysHeap(PB(HeapBlock), HeapPageCount * 4096);
	g_SysHeap = &SysHeap;

	CTask KernelTask(GDT, true, 0, 0, 0, 0, CMemMap::c_PmmPageDirectory);
	KernelTask._setActive();

	CIntManager IM(PMM, KernelTask.GetTSS().GetSelector());

	CKernel K(KernelTask, PMM, IM, GDT, IM.GetIDT(),
		BootType, DriverInfos, DriversCount);
}
// ----------------------------------------------------------------------------