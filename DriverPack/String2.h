// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// String2.h
//
// v 2.01 (2007.02.28)
// v 2.02 (2009.04.30)
//
#pragma once
#include "Defs.h"
#include "Array2.h"

// ----------------------------------------------------------------------------
//typedef unsigned short wchar_t;

// ----------------------------------------------------------------------------
template<bool U> class _chartype {public: typedef wchar_t char_t;};
template<> class _chartype<false> {public: typedef char char_t;};

// ----------------------------------------------------------------------------
template <bool U>
class CString
{
public:
	typedef typename _chartype<U>::char_t char_t;

	CString();
	CString(const char_t* Src);
	CString(const char_t* Src, dword SrcLen);

	void Add(const CString& Str);
	void Add(const char_t Char);
	dword Len() const;

	char_t GetCh(dword Index) const;
	void SetCh(dword Index, const char_t& Char);

	CString Left(dword Count) const;
	CString MidRel(dword Start, dword Count) const;
	CString MidAbs(dword Start, dword EndX) const;
	CString RightRel(dword Count) const;
	CString RightAbs(dword Start) const;

	void Trim(const char_t* CharSet);
	void TrimLeft(const char_t* CharSet);
	void TrimRight(const char_t* CharSet);

	const CString& ToLower();
/*
	const CString& ToUpper();

	int Find(char_t Char, dword Offset) const;
	int Find(const char_t* Substring, dword Offset) const;
	void Replace(const CString& What, const CString& ByWhat);
*/
	bool operator==(const CString& Str) const;
	bool operator<=(const CString& Str) const;
	bool operator>=(const CString& Str) const;
	bool operator!=(const CString& Str) const;
	bool operator<(const CString& Str) const;
	bool operator>(const CString& Str) const;

	char_t* _ptr();
	const char_t* _ptr() const;

private:
	CArray<char_t> m_Data;
};

// ----------------------------------------------------------------------------
typedef CString<false> CStringA;
typedef CString<true> CStringW;
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=