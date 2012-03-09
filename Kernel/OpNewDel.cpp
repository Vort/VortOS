// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// OpNewDel.cpp
#include "Global.h"

// ----------------------------------------------------------------------------
void* operator new(size_t Count)
{
	byte* Result = 0;
	bool R = g_SysHeap->Alloc(Count, Result);
	ErrIf(!R);
	return Result;
}

// ----------------------------------------------------------------------------
void operator delete(void* pBuffer)
{
	bool R = g_SysHeap->Free(PB(pBuffer));
	ErrIf(!R);
}

// ----------------------------------------------------------------------------
void* operator new[](size_t Count)
{
	byte* Result = 0;
	bool R = g_SysHeap->Alloc(Count, Result);
	ErrIf(!R);
	return Result;
}

// ----------------------------------------------------------------------------
void operator delete[](void* pBuffer)
{
	bool R = g_SysHeap->Free(PB(pBuffer));
	ErrIf(!R);
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=