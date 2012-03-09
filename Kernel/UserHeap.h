// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// UserHeap.h
#include "Library/Defs.h"
#include "PhysMemManager.h"
#include "VirtMemManager.h"
#pragma once

// ----------------------------------------------------------------------------
class CUserHeap
{
public:
	CUserHeap(CPhysMemManager& PMM, CVirtMemManager& VMM, dword VirtualBase)
		: m_PMM(PMM), m_VMM(VMM), m_VirtualBase(VirtualBase)
	{
		m_FreeMap = (bool*)PMM.AllocPage();
		for (dword i = 0; i < 4096; i++)
			m_FreeMap[i] = false;
		for (dword i = 0; i < 11; i++)
			m_DataBlocks[i] = null;
	}

	~CUserHeap()
	{
		m_PMM.ReleasePage(PB(m_FreeMap));
		for (dword i = 0; i < 11; i++)
			if (m_DataBlocks[i])
				m_PMM.ReleasePage(m_DataBlocks[i]);
	}

	void TouchBlock(dword i)
	{
		if (m_DataBlocks[i] == null)
		{
			m_DataBlocks[i] = PB(m_PMM.AllocPage());
			m_VMM.MapPhysPageToVirtPage(dword(m_DataBlocks[i]),
				m_VirtualBase + 0x1000 * i);
		}
	}

	dword GetPagesCount()
	{
		dword R = 1;
		for (dword i = 0; i < 11; i++)
			if (m_DataBlocks[i])
			{
				R++;
			}
		return R;
	}

	byte* Alloc(dword Size)
	{
		byte* Chunk = PB(0);
		TryAlloc(Size, Chunk);
		return Chunk;
	}

	bool Free(byte* Chunk)
	{
		return TryFree(Chunk);
	}

private:
	bool TryFree(byte* Chunk)
	{
		dword BlockIndex = 0;
		if (!GetBlockIndexFromChunkBase(Chunk, BlockIndex))
			return false;

		dword FreeMapOffset = 2048 >> BlockIndex;
		dword FreeMapIndex = (dword(Chunk) & 0xFFF) >> (BlockIndex + 1);
		if (m_FreeMap[FreeMapOffset + FreeMapIndex] == false)
			return false;
		else
			m_FreeMap[FreeMapOffset + FreeMapIndex] = false;
		return true;
	}

	bool TryAlloc(dword Size, byte*& Chunk)
	{
		dword BlockIndex = 0;
		if (!GetBlockIndexFromSize(Size, BlockIndex))
			return false;

		dword FreeMapOffset = 2048 >> BlockIndex;
		dword FreeMapSize = FreeMapOffset;
		// 2048 - 4095     0   2      x 2048
		// 1024 - 2047     1   4      x 1024
		// 512  - 1023     2   8      x 512
		// 256  - 511      3   16     x 256
		// 128  - 255      4   32     x 128
		// 64   - 127      5   64     x 64
		// 32   - 63       6   128    x 32
		// 16   - 31       7   256    x 16
		// 8    - 15       8   512    x 8
		// 4    - 7        9   1024   x 4
		// 2    - 3        10  2048   x 2

		for (dword i = 0; i < FreeMapSize; i++)
		{
			if (m_FreeMap[FreeMapOffset + i] == false)
			{
				TouchBlock(BlockIndex);
				Chunk = PB(m_VirtualBase + 0x1000 * BlockIndex +
					(i << (BlockIndex + 1)));
				m_FreeMap[FreeMapOffset + i] = true;
				return true;
			}
		}
		return false;
	}

	bool GetBlockIndexFromChunkBase(byte* Chunk, dword& BlockIndex)
	{
		dword dChunk = dword(Chunk);
		if (dChunk < m_VirtualBase)
			return false;
		if (dChunk >= (m_VirtualBase + 11 * 0x1000))
			return false;

		BlockIndex = (dChunk - m_VirtualBase) >> 12;
		return true;
	}

	bool GetBlockIndexFromSize(dword Size, dword& BlockIndex)
	{
		if (Size == 0)
			return false;

		dword S = Size - 1;
		dword BI = 0;
		for (;;)
		{
			if (S < 2)
				break;
			S >>= 1;
			BI++;
		}
		if (BI > 10)
			return false;

		BlockIndex = BI;
		return true;
	}

private:
	CPhysMemManager& m_PMM;
	CVirtMemManager& m_VMM;
	dword m_VirtualBase;
	bool* m_FreeMap;
	byte* m_DataBlocks[11];
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=