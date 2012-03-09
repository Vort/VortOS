// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GDT.h
#pragma once
#include "Library/Defs.h"
#include "Descriptor.h"
#include "PhysMemManager.h"

// ----------------------------------------------------------------------------
class CGDT
{
public:
	CGDT(CPhysMemManager& PMM);

	dword GetBase();
	dword GetFreeIndex();
	dword CreateNewDescriptor(dword Base, dword Limit, 
							  byte Type, byte Granularity);
	void DeleteDescriptor(dword Index);
	void SetDescriptor(dword Index, const CDescriptor& Descr);
	dword GetSelectorByIndex(dword Index);

	bool IsOccupied(dword Index);
	CDescriptor& GetDescriptorByIndex(dword Index);

	static const dword Size = 512;

private:
	CDescriptor* m_Descriptors;
	bool m_IsOccupied[Size];
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=