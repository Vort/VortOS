// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SysInit.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"

// ----------------------------------------------------------------------------
class SysInit
{
public:
	SysInit()
	{
		dword BootType = KeGetBootType();
		DebugOut((char*)&BootType, 4);
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_SysInit))
		return;
	SysInit SI;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=