// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// UnicodeConvert2.h
//
// v 1.00 (2010.09.27)
//
#pragma once
#include "Defs.h"
#include "Array2.h"
#include "String2.h"

#include <winnls.h>

// ----------------------------------------------------------------------------
namespace Lib {
// ----------------------------------------------------------------------------
class CUnicodeConvert2
{
public:
	CStringW StringAToW(const CStringA& StringA)
	{
		CArray<wchar_t> R(L'\0', StringA.Len() + 1);
		MultiByteToWideChar(CP_ACP, 0, StringA._ptr(),
			StringA.Len(), R._ptr(), R.Size());
		return CStringW(R._ptr());
	}

	CStringA StringWToA(const CStringW& StringW)
	{
		CArray<char> R('\0', StringW.Len() + 1);
		WideCharToMultiByte(CP_ACP, 0, StringW._ptr(),
			StringW.Len(), R._ptr(), R.Size(), null, null);
		return CStringA(R._ptr());
	}

	CArray<wchar_t> UTF8ToUTF16LE(const char* Data, dword Size)
	{
		CArray<wchar_t> R(L'\0', Size);
		dword L = MultiByteToWideChar(CP_UTF8,
			0, Data, Size, R._ptr(), R.Size());
		R.Delete(L, Size - L);
		return R;
	}

	CArray<char> UTF16LEToUTF8(const wchar_t* Data, dword Size)
	{
		CArray<char> R('\0', Size * 4);
		dword L = WideCharToMultiByte(CP_UTF8,
			0, Data, Size, R._ptr(), R.Size(), null, null);
		R.Delete(L, R.Size() - L);
		return R;
	}
};
// ----------------------------------------------------------------------------
} // namespace Lib
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=