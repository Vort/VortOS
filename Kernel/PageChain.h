// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PageChain.h
#include "Library/Defs.h"
#include "Library/Array2.h"
#include "PhysMemManager.h"
#pragma once

// ----------------------------------------------------------------------------
class CPageChain
{
public:
	CPageChain(CPhysMemManager& PMM, dword PageCount)
		: m_PMM(PMM)
	{
		m_ChainSize = PageCount;
		m_Chain = new dword[m_ChainSize];
		for (dword i = 0; i < m_ChainSize; i++)
			m_Chain[i] = dword(PMM.AllocPage());
	}

	CPageChain(CPhysMemManager& PMM, dword PageCount, dword PhysAddr)
		: m_PMM(PMM)
	{
		// bool R = 
		PMM.AllocBlockAt(PB(PhysAddr), PageCount, true);
		// if (R)
		// {
		m_ChainSize = PageCount;
		m_Chain = new dword[m_ChainSize];
		for (dword i = 0; i < m_ChainSize; i++)
			m_Chain[i] = PhysAddr + i * 0x1000;
		/*
		}
		else
		{
			m_ChainSize = 0;
			m_Chain = null;
		}
		*/
	}

	~CPageChain()
	{
		for (dword i = 0; i < m_ChainSize; i++)
			m_PMM.ReleasePage(PB(m_Chain[i]));
		delete m_Chain;
	}

	dword GetPageCount()
	{
		return m_ChainSize;
	}

	dword* GetChainBase()
	{
		return m_Chain;
	}

private:
	CPhysMemManager& m_PMM;
	dword* m_Chain;
	dword m_ChainSize;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=