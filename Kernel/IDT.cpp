// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IDT.cpp
#include "IDT.h"

// ----------------------------------------------------------------------------
extern "C" void __lidt(void *Source);
#pragma intrinsic(__lidt)

// ----------------------------------------------------------------------------
CIDT::CIDT(CPhysMemManager& PMM)
{
	m_Gates = (CGate*)PMM.AllocPage();

	for (dword i = 0; i < Size; i++)
		m_IsOccupied[i] = false;

	byte IDTPseudoDescriptor[6];
	*PW(&IDTPseudoDescriptor[0]) = 0x07FF;
	*PD(&IDTPseudoDescriptor[2]) = dword(m_Gates);
//	qword IDTPseudoDescriptor = (dword(m_Gates) << 16) | 0x07FF;
	__lidt(&IDTPseudoDescriptor);
}

// ----------------------------------------------------------------------------
dword CIDT::GetBase()
{
	return dword(m_Gates);
}

// ----------------------------------------------------------------------------
bool CIDT::IsOccupied(dword Index)
{
	return m_IsOccupied[Index];
}

// ----------------------------------------------------------------------------
CGate& CIDT::GetGateByIndex(dword Index)
{
	ErrIf(Index >= Size);
	return m_Gates[Index];
}

// ----------------------------------------------------------------------------
void CIDT::CreateNewGateAt(dword Index, dword Offset,
						   word Selector, byte Type)
{
	ErrIf(Index >= Size);
	ErrIf(m_IsOccupied[Index]); // по этому индексу уже есть шлюз

	CGate NewGate;
	NewGate.OffsetL = Offset;
	NewGate.OffsetH = Offset >> 16;
	NewGate.Selector = Selector;
	NewGate.Type = Type;

	m_Gates[Index] = NewGate;
	m_IsOccupied[Index] = true;
}

// ----------------------------------------------------------------------------
void CIDT::CreateNewTaskGateAt(dword Index, bool IsUser, dword Selector)
{
	byte Type = 0x85;
	if (IsUser) Type = 0xE5;
	CreateNewGateAt(Index, 0, Selector, Type);	
}

// ----------------------------------------------------------------------------
void CIDT::CreateNewTrapAt(byte Index, dword Offset)
{
	CreateNewGateAt(Index, Offset, 0x18, 0x8F);
}

// ----------------------------------------------------------------------------
void CIDT::DeleteGate(dword Index)
{
	ErrIf(Index >= Size);
	m_IsOccupied[Index] = false;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=