// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Array2.h
//
// v 2.0 (2006.11.28) (ProtMode)
//
#pragma once
#include "Defs.h"
#include "../OpNewDel.h"

// ----------------------------------------------------------------------------
template <class T>
class CArray
{
public:
	CArray()
	{
		m_AllocatedSize = 1;
		m_ElementsCount = 0;
		m_Data = _allocElementsMemory(1);
	}

	CArray(const T* Data, dword Size)
	{
		m_ElementsCount = Size;
		m_AllocatedSize = (Size == 0) ? 1 : Size;

		m_Data = _allocElementsMemory(m_AllocatedSize);
		_constructElements(m_Data, Data, m_ElementsCount);
	}

	CArray(const T& Filler, dword Size)
	{
		m_ElementsCount = Size;
		m_AllocatedSize = (Size == 0) ? 1 : Size;

		m_Data = _allocElementsMemory(m_AllocatedSize);
		_constructElements(m_Data, Filler, m_ElementsCount);
	}

	CArray(const CArray& Data)
	{
		m_ElementsCount = Data.m_ElementsCount;
		m_AllocatedSize = (Data.m_ElementsCount == 0) ? 1 : Data.m_ElementsCount;
	
		m_Data = _allocElementsMemory(m_AllocatedSize);
		_constructElements(m_Data, Data.m_Data, m_ElementsCount);
	}

	~CArray()
	{
		_destructElements(m_Data, m_ElementsCount);
		_freeElementsMemory(m_Data);
	}

	CArray& operator=(const CArray& Data)
	{
		if (this != &Data)
		{
			// Нет необходимости реализовывать оператор = для Т
			// Достаточно copy-конструктора
			this->~CArray();
			new (this) CArray(Data);
		}
		return *this;
	}

	bool operator==(const CArray& Array) const
	{
		if (m_ElementsCount != Array.m_ElementsCount)
			return false;
		for (dword i = 0; i < m_ElementsCount; i++)
		{
			if (m_Data[i] != Array.m_Data[i])
				return false;
		}
		return true;
	}

	bool operator!=(const CArray& Array) const
	{
		return !(*this == Array);
	}

	T* _ptr()
	{
		return m_Data;
	}

	const T* _ptr() const
	{
		return m_Data;
	}

	T* _ptr(dword Index)
	{
		ErrIf(Index >= m_ElementsCount);
		return &m_Data[Index];
	}

	const T* _ptr(dword Index) const
	{
		ErrIf(Index >= m_ElementsCount);
		return &m_Data[Index];
	}

	T& operator[](dword Index)
	{
		ErrIf(Index >= m_ElementsCount);
		return m_Data[Index];
	}

	const T& operator[](dword Index) const
	{
		ErrIf(Index >= m_ElementsCount);
		return m_Data[Index];
	}

	dword Size() const
	{
		return m_ElementsCount;
	}

	void Clear()
	{
		_destructElements(m_Data, m_ElementsCount);
		m_ElementsCount = 0;
	}

	T& PeekBack()
	{
		ErrIf(m_ElementsCount == 0);
		return m_Data[m_ElementsCount - 1];
	}

	T& PeekFront()
	{
		ErrIf(m_ElementsCount == 0);
		return m_Data[0];
	}

	void PushBack(const CArray& Array)
	{
		if (Array.m_ElementsCount == 0) return;

		_reserveStorage(m_ElementsCount + Array.m_ElementsCount);
		_constructElements(&m_Data[m_ElementsCount], 
			Array.m_Data, Array.m_ElementsCount);
		m_ElementsCount += Array.m_ElementsCount;
	}

	void PushFront(const CArray& Array)
	{
		if (Array.m_ElementsCount == 0) return;
		_insert(0, Array.m_Data, Array.m_ElementsCount);
	}

	void PushBack(const T& Element)
	{
		_reserveStorage(m_ElementsCount + 1);
		_constructElement(&m_Data[m_ElementsCount], Element);
		m_ElementsCount++;
	}

	void PushFront(const T& Element)
	{
		_insert(0, &Element, 1);
	}

	CArray PopBack(dword Count)
	{
		ErrIf(Count > m_ElementsCount);
		if (Count == 0) return CArray();
		return Extract(m_ElementsCount - Count, Count);
	}

	CArray PopFront(dword Count)
	{
		ErrIf(Count > m_ElementsCount);
		if (Count == 0) return CArray();
		return Extract(0, Count);
	}

	T PopBack()
	{
		ErrIf(m_ElementsCount == 0);
		T RetVal = m_Data[m_ElementsCount - 1];
		DeleteBack();
		return RetVal;
	}

	T PopFront()
	{
		ErrIf(m_ElementsCount == 0);
		T RetVal = m_Data[0];
		DeleteFront();
		return RetVal;
	}

	void Insert(dword Index, const CArray& Array)
	{
		_insert(Index, Array.m_Data, Array.m_ElementsCount);
	}

	void Insert(dword Index, const T& Element)
	{
		_insert(Index, &Element, 1);
	}

	CArray Extract(dword Index, dword Count)
	{
		if (Count == 0) return CArray();

		T* Extracted = _extract(Index, Count);
		CArray Result(Extracted, Count);
		_destructElements(Extracted, Count);
		_freeElementsMemory(Extracted);
		return Result;
	}

	T Extract(dword Index)
	{
		T* Extracted = _extract(Index, 1);
		T Result = *Extracted;
		_destructElement(Extracted);
		_freeElementsMemory(Extracted);
		return Result;
	}

	void Delete(dword Index, dword Count)
	{
		_delete(Index, Count);
	}

	void Delete(dword Index)
	{
		_delete(Index, 1);
	}

	void DeleteBack()
	{
		ErrIf(m_ElementsCount == 0);
		m_ElementsCount--;
		_destructElement(&m_Data[m_ElementsCount]);
	}

	void DeleteFront()
	{
		_delete(0, 1);
	}

private:
	void _expandStorage(dword ByCount)
	{
		if (ByCount == 0) return;

		m_AllocatedSize += ByCount;
		T* NewData = _allocElementsMemory(m_AllocatedSize);
		_constructElements(NewData, m_Data, m_ElementsCount);
		_destructElements(m_Data, m_ElementsCount);
		_freeElementsMemory(m_Data);
		m_Data = NewData;
	}

	dword _getExtraAllocCount(dword ExpandBy)
	{
		return m_AllocatedSize + ExpandBy;
	}

	void _reserveStorage(dword Count)
	{
		if (Count > m_AllocatedSize)
		{
			dword ExpandBy = Count - m_AllocatedSize;
			_expandStorage(ExpandBy + _getExtraAllocCount(ExpandBy));
		}
	}

	void _insert(dword Index, const T* Data, dword Count)
	{
		ErrIf(Index > m_ElementsCount);
		ErrIf(Data == null);
		if (Count == 0) return;
		
		dword Copy_Count = m_ElementsCount - Index;
		dword NewElementsCount = m_ElementsCount + Count;

		T* Copy_Data = null;
		if (Copy_Count)
		{
			Copy_Data = _allocElementsMemory(Copy_Count);
			_constructElements(Copy_Data, &m_Data[Index], Copy_Count);
			_destructElements(&m_Data[Index], Copy_Count);
			m_ElementsCount = Index;
		}
		
		_reserveStorage(NewElementsCount);
		_constructElements(&m_Data[Index], Data, Count);
		if (Copy_Count)
			_constructElements(&m_Data[Index + Count], Copy_Data, Copy_Count);

		m_ElementsCount = NewElementsCount;

		if (Copy_Count)
		{
			_destructElements(Copy_Data, Copy_Count);
			_freeElementsMemory(Copy_Data);
		}
	}

	void _delete(dword Index, dword Count)
	{
		ErrIf (Index + Count > m_ElementsCount);
		if (Count == 0) return;

		dword Copy_Count = m_ElementsCount - Index - Count;

		T* Copy_Data = null;
		if (Copy_Count)
		{
			Copy_Data = _allocElementsMemory(Copy_Count);
			_constructElements(Copy_Data, &m_Data[Index+Count], Copy_Count);
		}

		_destructElements(&m_Data[Index], Copy_Count + Count);
		if (Copy_Count)
		{
			_constructElements(&m_Data[Index], Copy_Data, Copy_Count);
			_destructElements(Copy_Data, Copy_Count);
			_freeElementsMemory(Copy_Data);
		}

		m_ElementsCount -= Count;
	}

	T* _extract(dword Index, dword Count)
	{
		ErrIf (Index + Count > m_ElementsCount);
		if (Count == 0) return null;

		T* Extracted = _allocElementsMemory(Count);
		_constructElements(Extracted, &m_Data[Index], Count);

		_delete(Index, Count);
		return Extracted;
	}

	static T* _allocElementsMemory(dword Count)
	{
		ErrIf(Count == 0);
		return (T*)operator new(sizeof(T) * Count);
	}

	static void _freeElementsMemory(T* Data)
	{
		ErrIf(Data == null);
		operator delete(Data);
	}

	static void _constructElements(T* At, const T* From, dword Count)
	{
		for (dword i = 0; i < Count; i++)
			new (&At[i]) T(From[i]);
	}

	static void _constructElements(T* At, const T& From, dword Count)
	{
		for (dword i = 0; i < Count; i++)
			new (&At[i]) T(From);
	}

	static void _constructElement(T* At, const T& From)
	{
		new (At) T(From);
	}

	static void _destructElements(T* At, dword Count)
	{
		for (dword i = 0; i < Count; i++)
			At[i].~T();
	}

	static void _destructElement(T* At)
	{
		At->~T();
	}

	T* m_Data;
	dword m_AllocatedSize;
	dword m_ElementsCount;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=