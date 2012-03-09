// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SingleMappedChain.h
#include "Library/Defs.h"
#include "Library/Array2.h"
#include "Library/SmartPtr.h"
#include "PageChain.h"
#include "ChainMapping.h"
#pragma once

// ----------------------------------------------------------------------------
class CSingleMappedChain
{
public:
	CSingleMappedChain(CPhysMemManager& PMM, CSmartPtr<CVirtMemManager> VMM,
		dword PageCount)
	{
		m_Chain = new CPageChain(PMM, PageCount);
		m_Mapping = new CChainMapping(0, m_Chain, VMM);
	}

	dword GetVirtualBase()
	{
		return m_Mapping->GetVirtualBase();
	}

	dword GetPageCount()
	{
		return m_Chain->GetPageCount();
	}

	~CSingleMappedChain()
	{
		m_Mapping = null;
		m_Chain = null;
	}

private:
	CSmartPtr<CPageChain> m_Chain;
	CSmartPtr<CChainMapping> m_Mapping;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=