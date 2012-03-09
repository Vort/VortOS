// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Test.cpp
#include "API.h"
#include "NumericConverter.h"

// ----------------------------------------------------------------------------
extern "C" unsigned __int64 __rdtsc();
#pragma intrinsic(__rdtsc)

// ----------------------------------------------------------------------------
void Entry()
{
	dword StartTime = KeGetTime();
	dword Count = 0;

	for (;;)
	{
		dword Time = KeGetTime();
		if ((Time - StartTime) > 5000)
			break;
		Count++;
	}

	char Buf[64] = {0};
	CNumericConverter NC;
	dword L = NC.DwordToString(Count / 5, Buf);

	DebugOut("Kernel calls per sec: ", 22);
	DebugOut(Buf, L);

	/*
	for (;;)
	{
		qword R = __rdtsc();
		DebugOut("[R:", 3);
		DebugOut(dword((R >> 32) & 0xFFFFFFFF));
		DebugOut(dword(R & 0xFFFFFFFF));
		DebugOut("]", 3);
		KeWaitTicks(20);
	}
	__asm ud2
	*/

	/*
	//__asm ud2
	char* A = "123";
	__asm
	{
		mov eax, A
		mov [eax], '5'
	}
	DebugOut(A, 3);
	*/
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=