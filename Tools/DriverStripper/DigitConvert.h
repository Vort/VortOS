// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// DigitConvert.h
//
// v 1.00 (2007.11.10)
// v 1.01 (2008.10.26)
// v 1.02 (2000.02.02) - stdlib version
//
#pragma once
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
		_itoa_s(V, Buf, 32, 10);
		return Buf;
	}

	CStringA QWordToString(qword V)
	{
		char Buf[64];
		_i64toa_s(V, Buf, 64, 10);
		return Buf;
	}

	CStringA FloatToString(double V)
	{
		char Buf[64];
		_gcvt_s(Buf, 64, V, 10);
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

	double StringToFloat(char* V)
	{
		return atof(V);
	}
};
// ----------------------------------------------------------------------------
} // namespace Lib
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=