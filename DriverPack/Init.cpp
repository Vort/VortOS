// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Init.cpp
#include "API.h"

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Init))
		return;

	KeWaitForSymbol(SmCursor_Ready);
	KeWaitForSymbol(SmDesktop_Ready);
	KeWaitForSymbol(SmFont_Ready);
	KeSetSymbol(Sm_InitStage2);
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=