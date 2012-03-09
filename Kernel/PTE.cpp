// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PTE.cpp
#include "PTE.h"

// ----------------------------------------------------------------------------
CPTE::CPTE()
{
	*(dword*)this = 0;
	m_RW = 1;
	m_IsUser = 1;
}

// ----------------------------------------------------------------------------
void CPTE::SetAvail(byte Val)
{
	m_Avail = Val & 0x7;
}

// ----------------------------------------------------------------------------
byte CPTE::GetAvail()
{
	return m_Avail & 0x7;
}

// ----------------------------------------------------------------------------
void CPTE::SetWrite(bool Enable, bool Invd)
{
	m_RW = Enable;
	if (Invd) InvalidateEntry();
}

// ----------------------------------------------------------------------------
void CPTE::SetBase(dword Base, bool Invd)
{
	ErrIf(Base << 20);
	m_Base = Base >> 12;
	if (Invd) InvalidateEntry();
}

// ----------------------------------------------------------------------------
dword CPTE::GetBase()
{
	return m_Base << 12;
}

// ----------------------------------------------------------------------------
bool CPTE::IsPresent()
{
	return m_Present;
}

// ----------------------------------------------------------------------------
void CPTE::SetPresent(bool Present, bool Invd)
{
	m_Present = Present;
	if (Invd) InvalidateEntry();
}

// ----------------------------------------------------------------------------
void CPTE::InvalidateEntry()
{
	dword Base = GetBase();
	__asm
	{
		mov eax, Base
		invlpg [eax]
	}
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=