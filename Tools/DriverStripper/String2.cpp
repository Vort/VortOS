// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// String2.cpp
//
// v 2.02 (2007.11.10)
//

#include "Defs.h"
#include "String2.h"
#include <windows.h>

using namespace Lib;

// ----------------------------------------------------------------------------
template<class T> dword strlen(const T* Src)
{
	dword Len = 0;
	for (;Src[Len]; Len++);
	return Len;
}

// ----------------------------------------------------------------------------
template<bool U> CString<U>::CString()
: m_Data(char_t('\0'), 1)
{
}

// ----------------------------------------------------------------------------
template<bool U> CString<U>::CString(const char_t* Src)
: m_Data(Src, (dword)strlen(Src) + 1)
{
}

// ----------------------------------------------------------------------------
template<bool U> CString<U>::CString(const char_t* Src, dword SrcLen)
: m_Data(Src, SrcLen)
{
	ErrIf(SrcLen > strlen(Src));
	m_Data.PushBack('\0');
}

// ----------------------------------------------------------------------------
template<bool U> void CString<U>::Add(const CString& Str)
{
	m_Data.PopBack();
	m_Data.PushBack(Str.m_Data);
}

// ----------------------------------------------------------------------------
template<bool U> void CString<U>::Add(const char_t Char)
{
	if (Char == 0) return;
	m_Data.DeleteBack();
	m_Data.PushBack(Char);
	m_Data.PushBack(0);
}

// ----------------------------------------------------------------------------
template<bool U> dword CString<U>::Len() const
{
	ErrIf(m_Data.Size() == 0);
	return m_Data.Size() - 1;
}

// ----------------------------------------------------------------------------
template<bool U> typename _chartype<U>::char_t CString<U>::GetCh(dword Index) const
{
	ErrIf(Index >= m_Data.Size());
	return m_Data[Index];
}

// ----------------------------------------------------------------------------
template<bool U> void CString<U>::SetCh(dword Index, const char_t& Char)
{
	ErrIf(Char == '\0');
	ErrIf(Index >= m_Data.Size() - 1);
	m_Data[Index] = Char;
}

// ----------------------------------------------------------------------------
template<bool U> CString<U> CString<U>::Left(dword Count) const
{
	ErrIf(Count > m_Data.Size() - 1);
	return CString(m_Data._ptr(), Count);
}

// ----------------------------------------------------------------------------
template<bool U> CString<U> CString<U>::MidRel(dword Start, dword Count) const
{
	ErrIf(Start + Count > m_Data.Size() - 1);
	return CString(m_Data._ptr() + Start, Count);
}

// ----------------------------------------------------------------------------
template<bool U> CString<U> CString<U>::RightAbs(dword Start) const
{
	ErrIf(Start > m_Data.Size() - 1);
	return CString(m_Data._ptr() + Start);
}

// ----------------------------------------------------------------------------
template<bool U> CString<U> CString<U>::MidAbs(dword Start, dword EndX) const
{
	ErrIf(Start > EndX);
	ErrIf(EndX > m_Data.Size() - 1);
	ErrIf(Start > m_Data.Size() - 1);
	return CString(m_Data._ptr() + Start, EndX - Start);
}

// ----------------------------------------------------------------------------
template<bool U> CString<U> CString<U>::RightRel(dword Count) const
{
	ErrIf(Count > m_Data.Size() - 1);
	return CString(m_Data._ptr() + m_Data.Size() - 1 - Count);
}

// ----------------------------------------------------------------------------
template<bool U> void CString<U>::Trim(const char_t* CharSet)
{
	TrimLeft(CharSet);
	TrimRight(CharSet);
}

// ----------------------------------------------------------------------------
template<bool U> void CString<U>::TrimLeft(const char_t* CharSet)
{
	dword TrimStart = -1;
	dword CharsetSize = dword(strlen(CharSet));
	dword StringSize = m_Data.Size() - 1;
	if (CharsetSize == 0) return;
	if (StringSize == 0) return;

	for (dword i = 0; i < StringSize; i++)
	{
		bool IsValidChar = true;
		for (dword j = 0; j < CharsetSize; j++)
			if (m_Data[i] == CharSet[j])
			{
				IsValidChar = false;
				break;
			}

		if (IsValidChar)
		{
			TrimStart = i;
			break;
		}
	}
	if (TrimStart == 0) return;
	else if (TrimStart == -1)
	{
		*this = CString();
		return;
	}
	else *this = RightAbs(TrimStart);
}

// ----------------------------------------------------------------------------
template<bool U> void CString<U>::TrimRight(const char_t* CharSet)
{
	dword TrimEndX = -1;
	dword CharsetSize = dword(strlen(CharSet));
	dword StringSize = m_Data.Size() - 1;
	if (CharsetSize == 0) return;
	if (StringSize == 0) return;

	for (dword i = StringSize; i > 0; i--)
	{
		bool IsValidChar = true;
		for (dword j = 0; j < CharsetSize; j++)
			if (m_Data[i-1] == CharSet[j])
			{
				IsValidChar = false;
				break;
			}

		if (IsValidChar)
		{
			TrimEndX = i;
			break;
		}
	}
	if (TrimEndX == StringSize) return;
	else if (TrimEndX == -1)
	{
		*this = CString();
		return;
	}
	else *this = MidAbs(0, TrimEndX);
}

// ----------------------------------------------------------------------------
template<> const CString<false>& CString<false>::ToUpper()
{
	CharUpperA(m_Data._ptr());
	return *this;
}

// ----------------------------------------------------------------------------
template<> const CString<false>& CString<false>::ToLower()
{
	CharLowerA(m_Data._ptr());
	return *this;
}

// ----------------------------------------------------------------------------
template<> const CString<true>& CString<true>::ToUpper()
{
	CharUpperW(m_Data._ptr());
	return *this;
}

// ----------------------------------------------------------------------------
template<> const CString<true>& CString<true>::ToLower()
{
	CharLowerW(m_Data._ptr());
	return *this;
}

// ----------------------------------------------------------------------------
template<> int CString<false>::Find(char Char, dword Offset) const
{
	ErrIf(Offset > m_Data.Size() - 1);
	const char* Result = strchr(m_Data._ptr() + Offset, Char);
	if (!Result) return -1;
	return int(Result - m_Data._ptr());
}

// ----------------------------------------------------------------------------
template<> int CString<false>::Find(const char* Substring, dword Offset) const
{
	ErrIf(Offset > m_Data.Size() - 1);
	const char* Result = strstr(m_Data._ptr() + Offset, Substring);
	if (!Result) return -1;
	return int(Result - m_Data._ptr());
}

// ----------------------------------------------------------------------------
template<> int CString<true>::Find(wchar_t Char, dword Offset) const
{
	ErrIf(Offset > m_Data.Size() - 1);
	const wchar_t* Result = wcschr(m_Data._ptr() + Offset, Char);
	if (!Result) return -1;
	return int(Result - m_Data._ptr());
}

// ----------------------------------------------------------------------------
template<> int CString<true>::Find(const wchar_t* Substring, dword Offset) const
{
	ErrIf(Offset > m_Data.Size() - 1);
	const wchar_t* Result = wcsstr(m_Data._ptr() + Offset, Substring);
	if (!Result) return -1;
	return int(Result - m_Data._ptr());
}

// ----------------------------------------------------------------------------
template<bool U> void CString<U>::Replace(const CString& What,
										  const CString& ByWhat)
{
	CString Result;

	dword Delta = 0;
	dword Start = 0;
	while ((Delta = Find(What._ptr(), Delta)) != -1)
	{
		Result.Add(CString(m_Data._ptr() + Start, Delta - Start));
		Result.Add(ByWhat);

		Delta += What.Len();
		Start = Delta;
	}
	Result.Add(CString(m_Data._ptr() + Start));
	*this = Result;
}

// ----------------------------------------------------------------------------
template<bool U> bool CString<U>::operator==(const CString& Str) const
{
	if (m_Data.Size() != Str.m_Data.Size()) return false;
	for (dword i = 0; i < m_Data.Size(); i++)
		if (m_Data[i] != Str.m_Data[i]) return false;
	return true;
}

// ----------------------------------------------------------------------------
template<bool U> bool CString<U>::operator<=(const CString& Str) const
{
	dword CompLen = min(m_Data.Size(), Str.m_Data.Size());
	for (dword i = 0; i < CompLen; i++)
	{
		if (m_Data[i] < Str.m_Data[i]) return true;			// <
		else if (m_Data[i] > Str.m_Data[i]) return false;	// >
	}
	return true;											// =
}

// ----------------------------------------------------------------------------
template<bool U> bool CString<U>::operator>=(const CString& Str) const
{
	dword CompLen = min(m_Data.Size(), Str.m_Data.Size());
	for (dword i = 0; i < CompLen; i++)
	{
		if (m_Data[i] < Str.m_Data[i]) return false;		// <
		else if (m_Data[i] > Str.m_Data[i]) return true ;	// >
	}
	return true;											// =
}

// ----------------------------------------------------------------------------
template<bool U> bool CString<U>::operator!=(const CString& Str) const
{
	return !(*this == Str);
}

// ----------------------------------------------------------------------------
template<bool U> bool CString<U>::operator<(const CString& Str) const
{
	return !(*this >= Str);
}

// ----------------------------------------------------------------------------
template<bool U> bool CString<U>::operator>(const CString& Str) const
{
	return !(*this <= Str);
}

// ----------------------------------------------------------------------------
template<bool U> typename _chartype<U>::char_t* CString<U>::_ptr()
{
	return m_Data._ptr();
}

// ----------------------------------------------------------------------------
template<bool U> typename const _chartype<U>::char_t* CString<U>::_ptr() const
{
	return m_Data._ptr();
}

// ----------------------------------------------------------------------------
template CString<true>;
template CString<false>;
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=