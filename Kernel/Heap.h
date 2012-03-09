// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Heap.h
#include "Library/Defs.h"
#pragma once

// ----------------------------------------------------------------------------
class CHeapBlock
{
public:
	void SetMagic()
	{
		m_Magic = 0xF4F4;
	}

	void ClearMagic()
	{
		m_Magic = 0x0000;
	}

	bool CheckMagic()
	{
		return m_Magic == 0xF4F4;
	}

public:
	bool m_IsAllocated;
	bool m_Reserved;
	word m_Magic;
	dword m_DataSize;
	CHeapBlock* m_Next;
	CHeapBlock* m_Prev;
};

// ----------------------------------------------------------------------------
class CHeap
{
public:
	CHeap(byte* Base, dword Size)
	{
		ErrIf(Base == null);
		ErrIf(Size < 0x80);

		m_CurBlock = (CHeapBlock*)Base;
		m_CurBlock->m_IsAllocated = false;
		m_CurBlock->m_DataSize = Size - sizeof(CHeapBlock);
		m_CurBlock->m_Next = m_CurBlock;
		m_CurBlock->m_Prev = m_CurBlock;
		m_CurBlock->SetMagic();

		m_TotalSize = Size;
		m_UsedSize = sizeof(CHeapBlock);
	}

	bool Alloc(dword Size, byte*& Data)
	{
		dword NewBlockSize = Size + sizeof(CHeapBlock);
		CHeapBlock* Marker = m_CurBlock;
		CHeapBlock* Slide = m_CurBlock;
		for (;;)
		{
			if (!Slide->m_IsAllocated)
			{
				if (Slide->m_DataSize == Size)
				{
					Slide->m_IsAllocated = true;

					m_UsedSize += Size;

					m_CurBlock = Slide->m_Next;
					Data = PB(Slide + 1);
					return true;
				}
				else if (Slide->m_DataSize >= NewBlockSize)
				{
					CHeapBlock* NewBlock = (CHeapBlock*)(PB(Slide) + NewBlockSize);
					NewBlock->m_IsAllocated = false;
					NewBlock->m_DataSize = Slide->m_DataSize - NewBlockSize;
					NewBlock->m_Next = Slide->m_Next;
					NewBlock->m_Prev = Slide;
					NewBlock->SetMagic();

					Slide->m_IsAllocated = true;
					Slide->m_DataSize = Size;
					Slide->m_Next->m_Prev = NewBlock;
					Slide->m_Next = NewBlock;

					m_UsedSize += NewBlockSize;

					m_CurBlock = NewBlock;
					Data = PB(Slide + 1);
					return true;
				}
			}

			Slide = Slide->m_Next;
			if (Slide == Marker)
				break;
		}
		return false;
	}

	bool Free(byte* Data)
	{
		CHeapBlock* FreeBlock = ((CHeapBlock*)Data) - 1;
		if (!FreeBlock->CheckMagic())
			return false;
		if (!FreeBlock->m_IsAllocated)
			return false;

		m_UsedSize -= FreeBlock->m_DataSize;

		bool CurBlockChange = 
			(m_CurBlock == FreeBlock) ||
			(m_CurBlock == FreeBlock->m_Prev) ||
			(m_CurBlock == FreeBlock->m_Next);

		CHeapBlock* PrevBlock = FreeBlock->m_Prev;
		if (!PrevBlock->m_IsAllocated)
			if (PrevBlock != FreeBlock)
			{
				FreeBlock->ClearMagic();
				FreeBlock->m_Next->m_Prev = PrevBlock;
				PrevBlock->m_Next = FreeBlock->m_Next;
				PrevBlock->m_DataSize += sizeof(CHeapBlock) + FreeBlock->m_DataSize;
				FreeBlock = PrevBlock;

				m_UsedSize -= sizeof(CHeapBlock);
			}

		CHeapBlock* NextBlock = FreeBlock->m_Next;
		if (!NextBlock->m_IsAllocated)
			if (NextBlock != FreeBlock)
			{
				NextBlock->ClearMagic();
				NextBlock->m_Next->m_Prev = FreeBlock;
				FreeBlock->m_Next = NextBlock->m_Next;
				FreeBlock->m_DataSize += sizeof(CHeapBlock) + NextBlock->m_DataSize;

				m_UsedSize -= sizeof(CHeapBlock);
			}

		FreeBlock->m_IsAllocated = false;
		if (CurBlockChange)
			m_CurBlock = FreeBlock;
		return true;
	}

	dword GetTotalSize()
	{
		return m_TotalSize;
	}

	dword GetUsedSize()
	{
		return m_UsedSize;
	}

private:
	CHeapBlock* m_CurBlock;
	dword m_TotalSize;
	dword m_UsedSize;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=