// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Descriptor.h
#include "Library/Defs.h"
#pragma once

// ----------------------------------------------------------------------------
class CDescriptor
{
public:
	dword GetBase()
	{
		return BaseL | BaseM << 16 | BaseH << 24;
	}

	word LimitL;
	word BaseL;
	byte BaseM;
	byte Type;
	byte Access;
	byte BaseH;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=