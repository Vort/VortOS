// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PDE.h
#pragma once
#include "Library/Defs.h"
#include "PT.h"

// ----------------------------------------------------------------------------
class CPDE
{
public:
	CPDE();

	CPT& GetPT();
	void SetBase(dword Base);
	dword GetBase();
	void SetPresent(bool Present);
	bool IsPresent();

private:
	dword m_Present : 1;
	dword m_RW : 1;
	dword m_IsUser : 1;
	dword m_Flags : 6;
	dword m_Avail : 3;
	dword m_Base  : 20;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=