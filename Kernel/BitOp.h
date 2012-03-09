// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BitOp.h
#include "Library/Defs.h"
#pragma once

// ----------------------------------------------------------------------------
inline void ClearBit(byte& Value, dword Index)
{
	Value = Value & ~(1 << Index);
}

// ----------------------------------------------------------------------------
inline void ClearBit(dword& Value, dword Index)
{
	Value = Value & ~(1 << Index);
}

// ----------------------------------------------------------------------------
inline void SetBit(byte& Value, dword Index)
{
	Value = Value | (1 << Index);
}

// ----------------------------------------------------------------------------
inline void SetBit(dword& Value, dword Index)
{
	Value = Value | (1 << Index);
}

// ----------------------------------------------------------------------------
inline bool TestBit(byte Value, dword Index)
{
	return Value & (1 << Index);
}

// ----------------------------------------------------------------------------
inline dword GetBitMask(dword Index)
{
	return 1 << Index;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=