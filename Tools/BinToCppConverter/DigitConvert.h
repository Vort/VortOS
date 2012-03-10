// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// DigitConvert.h
//
// v 1.00 (2007.11.10)
//
#pragma once
#include "String2.h"
#include <stdlib.h>

// ----------------------------------------------------------------------------
namespace Lib {
// ----------------------------------------------------------------------------
class CDigitConvert
{
public:
	CStringA DWordToString(dword V)
	{
		char Buf[32];
		_itoa(V, Buf, 10);
		return Buf;
	}

	CStringA QWordToString(qword V)
	{
		char Buf[64];
		_i64toa(V, Buf, 10);
		return Buf;
	}

	dword StringToDWord(const char* V)
	{
		return atoi(V);
	}

	qword StringToQWord(char* V)
	{
		return _atoi64(V);
	}
};
// ----------------------------------------------------------------------------
} // namespace Lib
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=