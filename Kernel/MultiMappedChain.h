// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// MultiMappedChain.h
#include "Library/Defs.h"
#include "Library/Array2.h"
#include "Library/SmartPtr.h"
#include "PageChain.h"
#include "ChainMapping.h"
#pragma once

// ----------------------------------------------------------------------------
class CMultiMappedChain
{
public:
	CMultiMappedChain(CPhysMemManager& PMM, dword Size)
	{
		SetID();
		m_Size = Size;
		dword PagesCount = (Size + 0xFFF) >> 12;
		m_Chain = new CPageChain(PMM, PagesCount);
	}

	CMultiMappedChain(CPhysMemManager& PMM, dword Size, dword PhysAddr)
	{
		SetID();
		m_Size = Size;
		dword PagesCount = (Size + 0xFFF) >> 12;
		m_Chain = new CPageChain(PMM, PagesCount, PhysAddr);
	}

	void SetID()
	{
		static dword g_ID = 0;
		g_ID++;
		m_ID = g_ID;
	}

	dword GetSize()
	{
		return m_Size;
	}

	dword GetID()
	{
		return m_ID;
	}

	dword MapProcess(dword PID, CSmartPtr<CVirtMemManager> VMM)
	{
		for (dword i = 0; i < m_Mappings.Size(); i++)
			if (m_Mappings[i]->GetPID() == PID)
				return m_Mappings[i]->GetVirtualBase();

		CSmartPtr<CChainMapping> CM =
			new CChainMapping(PID, m_Chain, VMM);
		m_Mappings.PushBack(CM);
		return CM->GetVirtualBase();
	}

	bool UnmapProcess(dword PID)
	{
		for (dword i = 0; i < m_Mappings.Size(); i++)
		{
			if (m_Mappings[i]->GetPID() == PID)
			{
				m_Mappings.Delete(i);
				return true;
			}
		}
		return false;
	}

private:
	dword m_ID;
	dword m_Size;
	CSmartPtr<CPageChain> m_Chain;
	CArray<CSmartPtr<CChainMapping> > m_Mappings;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=