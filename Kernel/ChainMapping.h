// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ChainMapping.h
#include "Library/Defs.h"
#include "Library/Array2.h"
#include "Library/SmartPtr.h"
#include "PageChain.h"
#include "VirtMemManager.h"
#pragma once

// ----------------------------------------------------------------------------
class CChainMapping
{
public:
	CChainMapping(dword PID, CSmartPtr<CPageChain> Chain,
		CSmartPtr<CVirtMemManager> VMM)
	{
		m_PID = PID;
		m_VMM = VMM;
		m_Chain = Chain;
		m_VirtualBase = 0;

		m_ChainSize = Chain->GetPageCount();
		dword* ChainBase = Chain->GetChainBase();

		if (m_ChainSize == 0)
			return;

		m_VirtualBase = VMM->MapBlock(ChainBase, m_ChainSize);
	}

	~CChainMapping()
	{
		m_VMM->UnmapBlock(m_VirtualBase, m_ChainSize);
	}

	dword GetVirtualBase()
	{
		return m_VirtualBase;
	}

	dword GetPID()
	{
		return m_PID;
	}

private:
	dword m_PID;
	dword m_VirtualBase;
	dword m_ChainSize;
	CSmartPtr<CVirtMemManager> m_VMM;
	CSmartPtr<CPageChain> m_Chain;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=