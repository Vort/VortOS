// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// DebugNew.cpp
//
// v 1.0 (2006.04.09)
//
#include <windows.h>
#include "Defs.h"

// ----------------------------------------------------------------------------
IfDebug(static dword AllocatedCount = 0);

// ----------------------------------------------------------------------------
void* operator new(size_t Count)
{
	IfNDebug(dword Flags = GMEM_FIXED);
	IfDebug(dword Flags = GMEM_FIXED | GMEM_ZEROINIT);
	void* Result = GlobalAlloc(Flags, Count);
	IfDebug(AllocatedCount += dword(Count ? Count : 1));
	return Result;
}

// ----------------------------------------------------------------------------
void* operator new[](size_t Count)
{
	return operator new(Count);
}

// ----------------------------------------------------------------------------
void operator delete(void* pBuffer)
{
	IfDebug(AllocatedCount -= (dword)HeapSize(GetProcessHeap(), 0, pBuffer));
	GlobalFree(pBuffer);
}

// ----------------------------------------------------------------------------
void operator delete[](void* pBuffer)
{
	return operator delete(pBuffer);
}

// ----------------------------------------------------------------------------
// Needs to be defined externally
void DumpMemoryLeaks()
{
	IfDebug(
		char Buffer[32];
		wsprintfA(Buffer, "Memory Leak: %d Bytes", AllocatedCount);
		MessageBoxA(null, Buffer, "Memory Leak Report",
			MB_OK | MB_ICONINFORMATION);
	)
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=