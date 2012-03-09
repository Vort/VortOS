// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SmartPtr.h
//
// v 1.01 (2006.11.21)
//
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
template <class T>
class CRefCounted
{
public:
	CRefCounted(T* Ptr)
	{
		ErrIf(Ptr == null);
		m_Data = Ptr;
		m_ReferenceCount = 1;
	}
	void AddRef() {m_ReferenceCount++;}
	T& operator*() const {return *m_Data;}
	operator CRefCounted<const T>&() {return *this;}
	void ReleaseRef() {if (!--m_ReferenceCount) delete this;}

private:
	CRefCounted& operator=(const CRefCounted&); // Protection
	CRefCounted(const CRefCounted&); // Protection
	~CRefCounted()
	{
		delete m_Data;
	}

	dword m_ReferenceCount;
	T* m_Data;
};

// ----------------------------------------------------------------------------
template <class T>
class CSmartPtr
{
public:
	CSmartPtr() {m_RefC = null;}

	CSmartPtr(T* Ptr)
	{
		if (Ptr) m_RefC = new CRefCounted<T>(Ptr);
		else m_RefC = null;
	}

	CSmartPtr(const CSmartPtr& Ptr)
	{
		m_RefC = Ptr.m_RefC;
		if (m_RefC) m_RefC->AddRef();
	}

	CSmartPtr& operator=(const CSmartPtr& SmartPtr)
	{
		if (m_RefC != SmartPtr.m_RefC)
		{
			if (m_RefC)
				m_RefC->ReleaseRef();
			m_RefC = SmartPtr.m_RefC;
			if (m_RefC) m_RefC->AddRef();
		}
		return *this;
	}

	operator CSmartPtr<const T>& () {return *this;}
	T& operator*() const {ErrIf(m_RefC == null); return **m_RefC;}
	T* operator->() const {ErrIf(m_RefC == null); return &**m_RefC;}
	bool operator!() const {return m_RefC == null;}
	operator bool() const {return m_RefC != null;}
	bool operator== (const CSmartPtr& Ptr) const {return m_RefC == Ptr.m_RefC;}
	bool operator!= (const CSmartPtr& Ptr) const {return m_RefC != Ptr.m_RefC;}

	~CSmartPtr()
	{
		if (m_RefC) m_RefC->ReleaseRef();
	}

private:
	operator void*(); // Protection

	CRefCounted<T>* m_RefC;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=