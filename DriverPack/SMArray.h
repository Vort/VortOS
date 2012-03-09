// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SMArray.h
#include "Defs.h"
#include "KeCalls.h"
#pragma once

// ----------------------------------------------------------------------------
template <class T>
class CSMArray
{
public:
	CSMArray(dword MaxSize)
	{
		m_Size = 0;
		m_MaxSize = MaxSize;
		m_SMID = KeAllocSharedMem(MaxSize * sizeof(T));
		m_Data = (T*)KeMapSharedMem(m_SMID);
	}

	~CSMArray()
	{
		KeReleaseSharedMem(m_SMID);
	}

	dword GetSMID()
	{
		return m_SMID;
	}

	dword GetSize()
	{
		return m_Size;
	}

	void Clear()
	{
		m_Size = 0;
	}

	bool IsFull()
	{
		return m_Size == m_MaxSize;
	}

	void Push(const T& Val)
	{
		m_Data[m_Size] = Val;
		m_Size++;
	}

	/*
	T& operator[](dword Index)
	{
		return m_Data[Index];
	}
	*/

private:
	dword m_Size;
	dword m_MaxSize;
	dword m_SMID;
	T* m_Data;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=