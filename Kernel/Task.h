// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Task.h
#include "Library/Defs.h"
#include "Tss.h"
#pragma once

// ----------------------------------------------------------------------------
class CTask
{
public:
	CTask(CGDT& GDT, bool IsKernel, void* EntryPoint, byte* pStack,
		byte* pStackTop, byte* Ring0Stack, dword CR3);

	void SetInterrupts(bool Enable);
	void JumpTo();
	void _setActive();

	CTSS& GetTSS();

private:
	CTask(const CTask& Src);
	void operator=(const CTask& Src);

	CTSS m_TSS;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=