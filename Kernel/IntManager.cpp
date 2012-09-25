// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IntManager.cpp
#include "IntManager.h"
#include "Intrinsics.h"
#include "BitOp.h"
#include "MemMap.h"

// ----------------------------------------------------------------------------
CIntManager::CIntManager(CPhysMemManager& PMM, dword KernelTaskSelector)
	: m_IDT(PMM)
{
	// Init handlers
	m_IntHandlers = PB(PMM.AllocPage());

	for (dword i = 0; i < 32; i++)
	{
		m_IntHandlers[0  + i * 32] = 0x9C;
		m_IntHandlers[1  + i * 32] = 0x50;
		m_IntHandlers[2  + i * 32] = 0x0F;
		m_IntHandlers[3  + i * 32] = 0x00;
		m_IntHandlers[4  + i * 32] = 0xC8;
		m_IntHandlers[5  + i * 32] = 0x66;
		m_IntHandlers[6  + i * 32] = 0x83;
		m_IntHandlers[7  + i * 32] = 0xF8;
		m_IntHandlers[8  + i * 32] = KernelTaskSelector;
		m_IntHandlers[9  + i * 32] = 0x75;
		m_IntHandlers[10 + i * 32] = 0x01;
		m_IntHandlers[11 + i * 32] = 0xF4;
		m_IntHandlers[12 + i * 32] = 0x58;
		m_IntHandlers[13 + i * 32] = 0x9D;
		m_IntHandlers[14 + i * 32] = 0xEA;
		m_IntHandlers[15 + i * 32] = 0x00;
		m_IntHandlers[16 + i * 32] = 0x00;
		m_IntHandlers[17 + i * 32] = 0x00;
		m_IntHandlers[18 + i * 32] = 0x00;
		m_IntHandlers[19 + i * 32] = KernelTaskSelector;
		m_IntHandlers[20 + i * 32] = 0x00;
		if (i == 7)
		{
			m_IntHandlers[21 + i * 32] = 0x0F;
			m_IntHandlers[22 + i * 32] = 0x06;
			m_IntHandlers[23 + i * 32] = 0xCF;
		}
		m_IDT.CreateNewGateAt(i,
			dword(CMemMap::c_IntHandlersVBase + i * 32), 0x10, 0xEE);
	}

	for (dword i = 0; i < 224; i++)
	{
		m_IntHandlers[0x400 + 0 + i * 8] = 0xEA;
		m_IntHandlers[0x400 + 1 + i * 8] = 0x00;
		m_IntHandlers[0x400 + 2 + i * 8] = 0x00;
		m_IntHandlers[0x400 + 3 + i * 8] = 0x00;
		m_IntHandlers[0x400 + 4 + i * 8] = 0x00;
		m_IntHandlers[0x400 + 5 + i * 8] = KernelTaskSelector;
		m_IntHandlers[0x400 + 6 + i * 8] = 0x00;
		m_IntHandlers[0x400 + 7 + i * 8] = 0xCF;

		m_IDT.CreateNewGateAt(i + 32,
			dword(CMemMap::c_IntHandlersVBase + 0x400 + i * 8), 0x10, 0xEE);
	}

	// Relocate Base Interrupt Number - PIC1
	_outp(0x20, 0x11); // - ICW1
	_outp(0x21, 0x20); // - ICW2 (Base = int 0x20)
	_outp(0x21, 0x04); // - ICW3
	_outp(0x21, 0x01); // - ICW4
	// Relocate Base Interrupt Number - PIC2
	_outp(0xA0, 0x11); // - ICW1
	_outp(0xA1, 0x28); // - ICW2 (Base = int 0x28)
	_outp(0xA1, 0x02); // - ICW3
	_outp(0xA1, 0x01); // - ICW4

	// Mask Interrupts
	m_PIC1Mask = 0xFF;
	m_PIC2Mask = 0xFF;

	_outp(0xA1, m_PIC2Mask);
	_outp(0x21, m_PIC1Mask);
}

// ----------------------------------------------------------------------------
CIDT& CIntManager::GetIDT()
{
	return m_IDT;
}

// ----------------------------------------------------------------------------
dword CIntManager::GetHandlersBase()
{
	return dword(m_IntHandlers);
}

// ----------------------------------------------------------------------------
dword CIntManager::GetRequestIndex(dword ProcessEIP)
{
	dword IntNum = -1;
	dword Offset = ProcessEIP - CMemMap::c_IntHandlersVBase;
	if (Offset < 32 * 32 + 224 * 8)
	{
		if (Offset < 32 * 32)
			IntNum = Offset / 32;
		else
			IntNum = (Offset - 32 * 32) / 8 + 0x20;
	}
	return IntNum;
}

// ----------------------------------------------------------------------------
void CIntManager::UnmaskIRQ(dword IRQ)
{
	dword Index = IRQ;
	dword IndexT = Index & 0x7;

	if (Index & 0x8) // PIC2
	{
		ClearBit(m_PIC2Mask, IndexT);
		_outp(0xA1, m_PIC2Mask);
	}
	else // PIC1
	{
		ClearBit(m_PIC1Mask, IndexT);
		_outp(0x21, m_PIC1Mask);
	}
}

// ----------------------------------------------------------------------------
void CIntManager::MaskIRQ(dword IRQ)
{
	dword Index = IRQ;
	dword IndexT = Index & 0x7;

	if (Index & 0x8) // PIC2
	{
		SetBit(m_PIC2Mask, IndexT);
		_outp(0xA1, m_PIC2Mask);
	}
	else // PIC1
	{
		SetBit(m_PIC1Mask, IndexT);
		_outp(0x21, m_PIC1Mask);
	}
}

// ----------------------------------------------------------------------------
void CIntManager::EndOfInterrupt(dword IRQ)
{
	if (IRQ < 16)
	{
		if (IRQ >= 8)
			_outp(0xA0, 0x20);
		_outp(0x20, 0x20);
	}
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=