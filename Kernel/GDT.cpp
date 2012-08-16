// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GDT.cpp
#include "GDT.h"

// ----------------------------------------------------------------------------
CGDT::CGDT(CPhysMemManager& PMM)
{
	m_Descriptors = (CDescriptor*)PMM.AllocPage();

	for (dword i = 0; i < Size; i++)
		m_IsOccupied[i] = false;

	m_IsOccupied[0] = true; // Null Descriptor

	byte GDTPseudoDescriptor[6];
	*PW(&GDTPseudoDescriptor[0]) = 0x07FF;
	*PD(&GDTPseudoDescriptor[2]) = dword(m_Descriptors);
	__asm lgdt GDTPseudoDescriptor
}

// ----------------------------------------------------------------------------
dword CGDT::GetBase()
{
	return dword(m_Descriptors);
}

// ----------------------------------------------------------------------------
dword CGDT::GetFreeIndex()
{
	for (dword i = 0; i < Size; i++)
		if (!m_IsOccupied[i])
			return i;
	ErrIf(true);
	return -1;
}

// ----------------------------------------------------------------------------
bool CGDT::IsOccupied(dword Index)
{
	return m_IsOccupied[Index];
}

// ----------------------------------------------------------------------------
dword CGDT::CreateNewDescriptor(dword Base, dword Limit,
						  byte Type, byte Granularity)
{
	CDescriptor NewDescr;
	NewDescr.BaseL = Base;
	NewDescr.BaseM = Base >> 16;
	NewDescr.BaseH = Base >> 24;
	NewDescr.LimitL = Limit;
	NewDescr.Type = Type;
	NewDescr.Access = 0x40 | (Granularity << 7) | (Limit >> 16);

	dword NewDescrIndex = GetFreeIndex();
	SetDescriptor(NewDescrIndex, NewDescr);
	return NewDescrIndex;
}

// ----------------------------------------------------------------------------
void CGDT::DeleteDescriptor(dword Index)
{
	ErrIf(Index >= Size);
	m_Descriptors[Index].Access = 0;
	m_Descriptors[Index].BaseH = 0;
	m_Descriptors[Index].BaseL = 0;
	m_Descriptors[Index].BaseM = 0;
	m_Descriptors[Index].LimitL = 0;
	m_Descriptors[Index].Type = 0;
	m_IsOccupied[Index] = false;
}

// ----------------------------------------------------------------------------
void CGDT::SetDescriptor(dword Index, const CDescriptor& Descr)
{
	ErrIf(Index >= Size);
	ErrIf(m_IsOccupied[Index]); // по этому индексу уже есть дескриптор
	m_Descriptors[Index] = Descr;
	m_IsOccupied[Index] = true;
}

// ----------------------------------------------------------------------------
CDescriptor& CGDT::GetDescriptorByIndex(dword Index)
{
	ErrIf(Index >= Size);
	return m_Descriptors[Index];
}

// ----------------------------------------------------------------------------
dword CGDT::GetSelectorByIndex(dword Index)
{
	ErrIf(Index >= Size);
	dword DPL = (m_Descriptors[Index].Type >> 5) & 0x03;
	return Index * 8 + DPL;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=