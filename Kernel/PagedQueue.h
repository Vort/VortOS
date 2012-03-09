// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PagedQueue.h
#include "Library/Defs.h"
#include "UniPtr.h"
#pragma once

// ----------------------------------------------------------------------------
class CPagedQueue
{
public:
	CPagedQueue(CPhysMemManager& PMM, dword MaxPageCount)
		: m_PMM(PMM)
	{
		m_TailDataOffset = 0;
		m_HeadDataOffset = 0;
		m_MaxPageCount = MaxPageCount;
		AddPage();
	}

	~CPagedQueue()
	{
		dword PageCount = m_Pages.Size();
		for (dword i = 0; i < PageCount; i++)
			RemovePage();
	}

	dword GetUsedPageCount()
	{
		return m_Pages.Size();
	}

	dword GetFreeSpace()
	{
		return m_MaxPageCount * m_PageSize - m_HeadDataOffset;
	}

 	bool AddData(CUniPtr& Data, dword DataSize)
	{
		if (DataSize == 0)
			return true;

		dword MaxDataSize = m_MaxPageCount * m_PageSize - m_HeadDataOffset;
		if (DataSize > MaxDataSize)
			return false;

		dword TotalDataLeft = DataSize;
		dword PageIndex = m_HeadDataOffset / m_PageSize;
		dword PageOffset = m_HeadDataOffset % m_PageSize;
		dword PageDataLeft = m_PageSize - PageOffset;

		dword SrcOffset = 0;
		byte* DstOffset = m_Pages[PageIndex] + PageOffset;

		for (;;)
		{
			if (TotalDataLeft < PageDataLeft)
				PageDataLeft = TotalDataLeft;
			
			if (!Data.CopyUtoP(SrcOffset, PageDataLeft, DstOffset))
				return false;
			TotalDataLeft -= PageDataLeft;

			m_HeadDataOffset += PageDataLeft;
			if (m_HeadDataOffset % m_PageSize == 0)
				AddPage();

			if (TotalDataLeft == 0)
				break;
			
			PageIndex++;
			SrcOffset += PageDataLeft;
			PageDataLeft = m_PageSize;
			DstOffset = m_Pages[PageIndex];
		}
		return true;
	}

	bool GetData(CUniPtr& Data, dword DataSize)
	{
		if (DataSize == 0)
			return true;

		dword MaxDataSize = m_HeadDataOffset - m_TailDataOffset;
		if (DataSize > MaxDataSize)
			return false;

		dword TotalDataLeft = DataSize;
		dword PageDataLeft = m_PageSize - m_TailDataOffset;

		byte* SrcOffset = m_Pages[0] + m_TailDataOffset;
		dword DstOffset = 0;

		for (;;)
		{
			if (TotalDataLeft < PageDataLeft)
				PageDataLeft = TotalDataLeft;
			
			if (!Data.CopyPtoU(SrcOffset, PageDataLeft, DstOffset))
				return false;

			TotalDataLeft -= PageDataLeft;

			m_TailDataOffset += PageDataLeft;
			if (m_TailDataOffset >= m_PageSize)
			{
				RemovePage();
				m_TailDataOffset -= m_PageSize;
				m_HeadDataOffset -= m_PageSize;
			}

			if (TotalDataLeft == 0)
				break;

			SrcOffset = m_Pages[0];
			DstOffset += PageDataLeft;
			PageDataLeft = m_PageSize;
		}
		return true;
	}

	bool PeekData(byte* Data, dword DataSize)
	{
		if (DataSize == 0)
			return true;

		dword MaxDataSize = m_HeadDataOffset - m_TailDataOffset;
		if (DataSize > MaxDataSize)
			return false;

		dword TotalDataLeft = DataSize;
		dword PageDataLeft = m_PageSize - m_TailDataOffset;
		dword ActivePage = 0;

		byte* SrcOffset = m_Pages[ActivePage] + m_TailDataOffset;
		byte* DstOffset = Data;

		for (;;)
		{
			if (TotalDataLeft < PageDataLeft)
				PageDataLeft = TotalDataLeft;
			
			for (dword i = 0; i < PageDataLeft; i++)
				DstOffset[i] = SrcOffset[i];
			TotalDataLeft -= PageDataLeft;

			if (TotalDataLeft == 0)
				break;

			ActivePage++;
			SrcOffset = m_Pages[ActivePage];
			DstOffset += PageDataLeft;
			PageDataLeft = m_PageSize;
		}
		return true;
	}

private:
	void AddPage()
	{
		m_Pages.PushBack(PB(m_PMM.AllocPage()));
	}

	void RemovePage()
	{
		m_PMM.ReleasePage(m_Pages.PopFront());
	}

private:
	dword m_MaxPageCount;
	dword m_TailDataOffset;
	dword m_HeadDataOffset;

	CArray<byte*> m_Pages;
	CPhysMemManager& m_PMM;

	static const dword m_PageSize = 4096;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=