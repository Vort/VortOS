// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// OpNewDel.h
#pragma once
#include "Library/Defs.h"

// ----------------------------------------------------------------------------
void* operator new(size_t Count);
void operator delete(void* pBuffer);
void* operator new[](size_t Count);
void operator delete[](void* pBuffer);
// ----------------------------------------------------------------------------
inline void* operator new(size_t Count, void* pBuffer) {return pBuffer;}
inline void operator delete(void*, void*) {}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=