// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// MemMap.h
#include "Defs.h"
#pragma once

// ----------------------------------------------------------------------------
class CMemMap
{
public:
	// Virtual
	static const dword c_ImageBase         = 0x00400000;
	static const dword c_VmmAllocMinVBase  = 0x00800000;
	static const dword c_VmmAllocMaxVBase  = 0xF0000000;
	static const dword c_UserHeapVBase     = 0xFFFF1000;
	static const dword c_IdtVBase          = 0xFFFFC000;
	static const dword c_IntHandlersVBase  = 0xFFFFD000;
	static const dword c_R0StackVBase      = 0xFF0FE000;
	static const dword c_ServiceFuncVBase  = 0xFF0FF000;

	// Physical
	static const dword c_PmmPageDirectory  = 0x00003000;
	static const dword c_PmmFirstPageTable = 0x00004000;
	static const dword c_VideoRamTextBase  = 0x000B8000;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=