// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PTE.h
#pragma once
#include "Library/Defs.h"

// ----------------------------------------------------------------------------
class CPTE
{
public:
	CPTE();

	void SetBase(dword Base, bool Invd = true);
	dword GetBase();

	void SetWrite(bool Enable, bool Invd = true);

	bool IsPresent();
	void SetPresent(bool Present, bool Invd = true);

	void SetAvail(byte Val);
	byte GetAvail();

private:
	void InvalidateEntry();

	dword m_Present : 1;
	dword m_RW : 1;
	dword m_IsUser : 1;
	dword m_Flags : 6;
	dword m_Avail : 3;
	dword m_Base  : 20;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=