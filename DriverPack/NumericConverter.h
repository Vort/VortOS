// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// NumericConverter.h
#include "Defs.h"
#pragma once

// ----------------------------------------------------------------------------
class CNumericConverter
{
public:
	dword DwordToString(dword Value, char* OutBuf,
		dword Base = 10, dword PadSize = 1, char PadChar = '0')
	{
		dword L = 0;
		dword V = Value;
		for (;;)
		{
			if (V == 0)
				break;

			dword D = V % Base;
			V = V / Base;

			if (D < 10)
				OutBuf[L] = D + '0';
			else
				OutBuf[L] = D + 'A' - 10;
			L++;
		}
		for (;;)
		{
			if (L >= PadSize)
				break;
			OutBuf[L++] = PadChar;
		}

		OutBuf[L] = '\0';

		dword RC = L / 2;
		for (dword i = 0; i < RC; i++)
		{
			char T = OutBuf[i];
			OutBuf[i] = OutBuf[L - i - 1];
			OutBuf[L - i - 1] = T;
		}

		return L;
	}
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=