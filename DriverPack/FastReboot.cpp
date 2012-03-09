// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FastReboot.cpp
#include "API.h"

// ----------------------------------------------------------------------------
void Entry()
{
	// Set CMOS
	KeOutPortByte(0x70, 0x8F); // 0xF "Reset Code", 0x80 "NMI Disabled"
	KeOutPortByte(0x71, 0x00); // "Software reset or unexpected reset"

	// Keyb Reboot
	KeOutPortByte(0x64, 0xFE);
	KeOutPortByte(0x64, 0xFF);

	for (;;) {}
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=