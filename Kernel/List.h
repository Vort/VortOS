// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// List.h
//
#pragma once
#include "Library/Defs.h"

// ----------------------------------------------------------------------------
template <class T>
class CListElement
{
public:
	CListElement()
	{
		m_Next = null;
		m_Prev = null;
		m_Data = null;
	}

	CListElement(T* Data)
	{
		m_Next = null;
		m_Prev = null;
		m_Data = Data;
	}

public:
	CListElement* m_Next;
	CListElement* m_Prev;
	T* m_Data;
};

// ----------------------------------------------------------------------------
template <class T>
class CList
{
public:
	CList()
	{
		m_FirstDummy = new CListElement<T>();
		m_LastDummy = new CListElement<T>();

		m_FirstDummy->m_Next = m_LastDummy;
		m_LastDummy->m_Prev = m_FirstDummy;
		m_CurrentElement = m_FirstDummy;

		m_ElementsCount = 0;
	}

	~CList()
	{
		ToFirstElement();
		while (m_ElementsCount)
			RemoveCurrentElement();
		delete m_FirstDummy;
		delete m_LastDummy;
	}

	T* GetCurrentElement()
	{
		return m_CurrentElement->m_Data;
	}

	void AppendElement(T* Data)
	{
		CListElement<T>* Element = new CListElement<T>(Data);

		m_LastDummy->m_Prev->m_Next = Element;
		Element->m_Prev = m_LastDummy->m_Prev;
		Element->m_Next = m_LastDummy;
		m_LastDummy->m_Prev = Element;

		m_ElementsCount++;
	}

	void RemoveCurrentElement()
	{
		if (m_CurrentElement == m_FirstDummy)
			return;
		if (m_CurrentElement == m_LastDummy)
			return;

		CListElement<T>* Element = m_CurrentElement;
		m_CurrentElement = m_CurrentElement->m_Next;

		Element->m_Prev->m_Next = Element->m_Next;
		Element->m_Next->m_Prev = Element->m_Prev;

		delete Element->m_Data;
		delete Element;

		m_ElementsCount--;
	}

	void MoveCurrentElementToEnd()
	{
		if (m_CurrentElement == m_FirstDummy)
			return;
		if (m_CurrentElement == m_LastDummy)
			return;
		if (m_CurrentElement->m_Next == m_LastDummy)
			return;

		CListElement<T>* Element = m_CurrentElement;
		m_CurrentElement = m_CurrentElement->m_Next;

		Element->m_Prev->m_Next = Element->m_Next;
		Element->m_Next->m_Prev = Element->m_Prev;

		m_LastDummy->m_Prev->m_Next = Element;
		Element->m_Prev = m_LastDummy->m_Prev;
		Element->m_Next = m_LastDummy;
		m_LastDummy->m_Prev = Element;
	}

	void ToFirstElement()
	{
		m_CurrentElement = m_FirstDummy->m_Next;
	}

	void ToLastElement()
	{
		m_CurrentElement = m_LastDummy->m_Prev;
	}

	void ToNextElement()
	{
		if (m_CurrentElement != m_LastDummy)
			m_CurrentElement = m_CurrentElement->m_Next;
	}

	bool IsAtLastDummy()
	{
		return m_CurrentElement == m_LastDummy;
	}

	dword GetElementsCount()
	{
		return m_ElementsCount;
	}

private:
	CListElement<T>* m_FirstDummy;
	CListElement<T>* m_LastDummy;
	CListElement<T>* m_CurrentElement;
	dword m_ElementsCount;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=