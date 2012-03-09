// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Global.h
#pragma once
#include "Library/Defs.h"
#include "Heap.h"

// ----------------------------------------------------------------------------
static const dword g_KernelImageBase = 0x20000;
static const dword g_KernelCodeBase = g_KernelImageBase + 0x1000;

// ----------------------------------------------------------------------------
extern CHeap* g_SysHeap;

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=