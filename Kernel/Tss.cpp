// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Tss.cpp
#include "Tss.h"

// ----------------------------------------------------------------------------
CTSS::CTSS(CGDT& GDT) : m_GDT(GDT)
{
	m_TaskState = new CTaskState();
	m_DescriptorIndex = m_GDT.CreateNewDescriptor(
		(dword)m_TaskState, 0x67, 0x89, false);
}

// ----------------------------------------------------------------------------
CTSS::CTSS(CGDT& GDT, CTaskState* TaskState, dword DescriptorIndex) : m_GDT(GDT)
{
	m_TaskState = TaskState;
	m_DescriptorIndex = DescriptorIndex;
}

// ----------------------------------------------------------------------------
CTSS::~CTSS()
{
	m_GDT.DeleteDescriptor(m_DescriptorIndex);
	delete m_TaskState;
}

// ----------------------------------------------------------------------------
CTaskState& CTSS::GetTaskState()
{
	return *m_TaskState;
}

// ----------------------------------------------------------------------------
dword CTSS::GetDescriptorIndex()
{
	return m_DescriptorIndex;
}

// ----------------------------------------------------------------------------
dword CTSS::GetSelector()
{
	return m_GDT.GetSelectorByIndex(m_DescriptorIndex);
}

// ----------------------------------------------------------------------------
void CTSS::JumpToTask()
{
	byte JumpDestBuf[6];
	*PD(JumpDestBuf) = 0;
	*PW(JumpDestBuf + 4) = m_GDT.GetSelectorByIndex(m_DescriptorIndex);
	__asm jmp far ptr [JumpDestBuf]
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=