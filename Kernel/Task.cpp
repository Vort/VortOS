// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Task.cpp
#include "Task.h"

// ----------------------------------------------------------------------------
CTask::CTask(CGDT& GDT, bool IsKernel, void* EntryPoint, byte* pStack,
			 byte* pStackTop, byte* Ring0Stack, dword CR3)
	: m_TSS(GDT)
{
	m_TSS.GetTaskState().EIP = dword(EntryPoint);
	m_TSS.GetTaskState().ESP = dword(pStackTop);

	byte SelCKe = 0x10;
	byte SelDKe = 0x08;
	byte SelCUs = 0x20+3;
	byte SelDUs = 0x18+3;

	byte SelC;
	byte SelD;
	if (IsKernel)
	{
		SelC = SelCKe;
		SelD = SelDKe;
	}
	else
	{
		SelC = SelCUs;
		SelD = SelDUs;
	}

	m_TSS.GetTaskState().SS0 = SelDKe;
	m_TSS.GetTaskState().ESP0 = dword(Ring0Stack) + 0x1000;

	m_TSS.GetTaskState().CS = SelC;
	m_TSS.GetTaskState().DS =
		m_TSS.GetTaskState().ES =
		m_TSS.GetTaskState().SS =
		m_TSS.GetTaskState().GS =
		m_TSS.GetTaskState().FS = SelD;

	m_TSS.GetTaskState().CR3 = CR3;
	m_TSS.GetTaskState().IOMAP = 0xFFFF;
}

// ----------------------------------------------------------------------------
void CTask::_setActive()
{
	word Sel = m_TSS.GetSelector();
	__asm ltr Sel
}

// ----------------------------------------------------------------------------
void CTask::JumpTo()
{
	m_TSS.JumpToTask();
}

// ----------------------------------------------------------------------------
void CTask::SetInterrupts(bool Enable)
{
	if (Enable) m_TSS.GetTaskState().EFLAGS |= 0x200;
	else m_TSS.GetTaskState().EFLAGS &= ~0x200;
}

// ----------------------------------------------------------------------------
CTSS& CTask::GetTSS()
{
	return m_TSS;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=