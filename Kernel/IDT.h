// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IDT.h
#pragma once
#include "Library/Defs.h"
#include "PhysMemManager.h"
#include "Gate.h"

// ----------------------------------------------------------------------------
class CIDT
{
public:
	CIDT(CPhysMemManager& PMM);

	dword GetBase();
	void CreateNewGateAt(dword Index, dword Offset, word Selector, byte Type);
	void CreateNewTaskGateAt(dword Index, bool IsUser, dword Selector);
	void CreateNewTrapAt(byte Index, dword Offset);
	void DeleteGate(dword Index);

	bool IsOccupied(dword Index);
	CGate& GetGateByIndex(dword Index);

	static const dword Size = 256;
	static const dword c_IdtVBase = 0xFFFFC000;

private:
	CGate* m_Gates;
	bool m_IsOccupied[Size];
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=