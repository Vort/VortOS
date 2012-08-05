// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// KeApiTest.cpp
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
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=