// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PTE.cpp
#include "PTE.h"
#include "Intrinsics.h"

// ----------------------------------------------------------------------------
CPTE::CPTE()
{
	m_Present = 0;
	m_Writable = 1;
	m_IsUser = 1;
	m_PWT = 0;
	m_PCD = 0;
	m_Accessed = 0;
	m_Dirty = 0;
	m_Reserved = 0;
	m_Avail = 0;
	m_Base = 0;
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
void CPTE::SetWritable(bool Enabled)
{
	m_Writable = Enabled;
}

// ----------------------------------------------------------------------------
void CPTE::SetBase(dword Base)
{
	ErrIf(Base << 20);
	m_Base = Base >> 12;
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
void CPTE::SetPresent(bool Present)
{
	m_Present = Present;
}

// ----------------------------------------------------------------------------
void CPTE::Invalidate()
{
	// TODO: Check if this is correct
	__invlpg((void*)GetBase());
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=