// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IntManager.h
#pragma once
#include "Library/Defs.h"
#include "PhysMemManager.h"
#include "IDT.h"

// ----------------------------------------------------------------------------
class CIntManager
{
public:
	CIntManager(CPhysMemManager& PMM, dword KernelTaskSelector);
	dword GetRequestIndex(dword ProcessEIP);
	void UnmaskIRQ(dword IRQ);
	void MaskIRQ(dword IRQ);
	void EndOfInterrupt(dword IRQ);

	dword GetHandlersBase();
	CIDT& GetIDT();

private:
	CIDT m_IDT;
	byte* m_IntHandlers;
	byte m_PIC1Mask;
	byte m_PIC2Mask;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=