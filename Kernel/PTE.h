// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PTE.h
#pragma once
#include "Library/Defs.h"

// ----------------------------------------------------------------------------
class CPTE
{
public:
	CPTE();

	void SetBase(dword Base);
	dword GetBase();

	void SetWritable(bool IsWritable);

	bool IsPresent();
	void SetPresent(bool Present);

	void SetAvail(byte Val);
	byte GetAvail();

	void Invalidate();

private:
	dword m_Present : 1;
	dword m_Writable : 1;
	dword m_IsUser : 1;
	dword m_PWT : 1;
	dword m_PCD : 1;
	dword m_Accessed : 1;
	dword m_Dirty : 1;
	dword m_Reserved : 2;
	dword m_Avail : 3;
	dword m_Base  : 20;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=