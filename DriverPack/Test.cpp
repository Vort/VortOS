// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Test.cpp
#include "API.h"

// ----------------------------------------------------------------------------
void Entry()
{
	for (int i = 0; i < 20; i++)
	{
		DebugOut("Line ");
		DebugOutDec(i);
		DebugOutLine();
	}
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=