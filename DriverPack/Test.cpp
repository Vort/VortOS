// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Test.cpp
#include "API.h"

// ----------------------------------------------------------------------------
void Entry()
{
	dword BAR0 = 0xD4000000;
	dword BAR1 = 0xDC000000;
	dword mmioSMID = KeAllocSharedMemAt(0x1000000, BAR0);
	dword vramSMID = KeAllocSharedMemAt(0x1000000, BAR1);
	byte* mmioBase = KeMapSharedMem(mmioSMID);
	byte* vramBase = KeMapSharedMem(vramSMID);
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=