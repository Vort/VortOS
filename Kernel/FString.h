// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FString.h
#include "Library/Defs.h"
#include "UniPtr.h"
#pragma once

// ----------------------------------------------------------------------------
template <dword S>
class CFString
{
public:
	CFString(const char* Src)
	{
		Construct(CUniPtr((byte*)Src));
	}

	CFString(const CUniPtr& Src)
	{
		Construct(Src);
	}

	const char* GetPtr() const
	{
		return m_Data;
	}

	void CopyTo(char* Dst) const
	{
		for (dword i = 0; i < S; i++)
			Dst[i] = m_Data[i];
	}

	dword Len() const
	{
		return m_Len;
	}

private:
	void Construct(const CUniPtr& Src)
	{
		m_Len = 0;

		for (dword i = 0; i < S; i++)
			m_Data[i] = '\0';

		for (dword i = 0; i < S - 1; i++)
		{
			if (!Src.CopyUtoP(i, *(byte*)(m_Data + i)))
				break;
			if (m_Data[i] == '\0')
				break;
			m_Len++;
		}
	}

private:
	dword m_Len;
	char m_Data[S];
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=