// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// UniPtr.h
#include "Library/Defs.h"
#include "Library/SmartPtr.h"
#include "VirtMemManager.h"
#pragma once

// ----------------------------------------------------------------------------
class CUniPtr
{
public:
	CUniPtr(byte* PhysVirt)
	{
		m_Ptr = PhysVirt;
	}

	CUniPtr(CSmartPtr<CVirtMemManager> VMM, byte* VirtPtr)
	{
		m_VMM = VMM;
		m_Ptr = VirtPtr;
	}

	bool CopyPtoU(byte* SrcPhysPtr, dword Size, dword DstOffset)
	{
		if (Size == 0)
			return true;

		if (!m_VMM)
		{
			byte* Start = m_Ptr + DstOffset;
			for (dword i = 0; i < Size; i++)
				Start[i] = SrcPhysPtr[i];
			return true;
		}
		else
		{
			return m_VMM->MemCopyPhysToVirt(SrcPhysPtr, m_Ptr + DstOffset, Size);
		}
	}

	bool CopyUtoP(dword SrcOffset, byte& DstByte) const
	{
		if (!m_VMM)
		{
			DstByte = m_Ptr[SrcOffset];
			return true;
		}
		else
		{
			return m_VMM->MemCopyByteVirtToPhys(m_Ptr + SrcOffset, DstByte);
		}
	}

	bool CopyUtoP(dword SrcOffset, dword Size, byte* DstPhysPtr) const
	{
		if (Size == 0)
			return true;

		if (!m_VMM)
		{
			byte* Start = m_Ptr + SrcOffset;
			for (dword i = 0; i < Size; i++)
				DstPhysPtr[i] = Start[i];
			return true;
		}
		else
		{
			return m_VMM->MemCopyVirtToPhys(m_Ptr + SrcOffset, DstPhysPtr, Size);
		}
	}


private:
	byte* m_Ptr;
	CSmartPtr<CVirtMemManager> m_VMM;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=