// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// OpNewDel.cpp
#include "Defs.h"

// ----------------------------------------------------------------------------
void GenericKeCall_ND(dword FuncIndex, dword InDataSize, dword OutDataSize,
				   byte* InDataPtr, byte* OutDataPtr)
{
	__asm
	{
		mov eax, FuncIndex
		mov ebx, InDataSize
		mov ecx, OutDataSize
		mov esi, InDataPtr
		mov edi, OutDataPtr
		int 0x30
	}
}

// ----------------------------------------------------------------------------
byte* KeMemAlloc(dword MemSize)
{
	dword MemBase;
	GenericKeCall_ND(29, 4, 4, PB(&MemSize), PB(&MemBase));
	return PB(MemBase);
}

// ----------------------------------------------------------------------------
void KeMemFree(byte* MemBase)
{
	GenericKeCall_ND(30, 4, 0, PB(&MemBase), null);
}

// ----------------------------------------------------------------------------
void* operator new(size_t Count)
{
	return KeMemAlloc(Count);
}

// ----------------------------------------------------------------------------
void operator delete(void* pBuffer)
{
	KeMemFree(PB(pBuffer));
}

// ----------------------------------------------------------------------------
void* operator new[](size_t Count)
{
	return KeMemAlloc(Count);
}

// ----------------------------------------------------------------------------
void operator delete[](void* pBuffer)
{
	KeMemFree(PB(pBuffer));
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=