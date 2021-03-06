// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PDE.cpp
#include "PDE.h"

// ----------------------------------------------------------------------------
CPDE::CPDE()
{
	*(dword*)this = 0;
	m_RW = 1;
	m_IsUser = 1;
}

// ----------------------------------------------------------------------------
CPT& CPDE::GetPT()
{
	return *(CPT*)(m_Base << 12);
}

// ----------------------------------------------------------------------------
void CPDE::SetBase(dword Base)
{
	ErrIf(Base << 20);
	m_Base = Base >> 12;
}

// ----------------------------------------------------------------------------
dword CPDE::GetBase()
{
	return m_Base << 12;
}

// ----------------------------------------------------------------------------
void CPDE::SetPresent(bool Present)
{
	m_Present = Present;
}

// ----------------------------------------------------------------------------
bool CPDE::IsPresent()
{
	return m_Present;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=