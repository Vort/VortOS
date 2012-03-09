// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Tss.h
#pragma once
#include "Library/Defs.h"
#include "GDT.h"

// ----------------------------------------------------------------------------
class CTaskState
{
public:
	CTaskState()
	{
		BackLink = ESP0 = SS0 = ESP1 = SS1 = ESP2 = SS2 = CR3 =
			EIP = EFLAGS = EAX = ECX = EDX = EBX = ESP = EBP =
			ESI = EDI = ES = CS = SS = DS = FS = GS = LDT = 0;
		T = IOMAP = 0;
	}

	dword BackLink; dword ESP0; dword SS0; dword ESP1; dword SS1;
	dword ESP2;     dword SS2;  dword CR3; dword EIP;  dword EFLAGS;
	dword EAX;      dword ECX;  dword EDX; dword EBX;  dword ESP;
	dword EBP;      dword ESI;  dword EDI; dword ES;   dword CS;
	dword SS;       dword DS;   dword FS;  dword GS;   dword LDT;
	word  T;        word  IOMAP;
};

// ----------------------------------------------------------------------------
class CTSS
{
public:
	CTSS(CGDT& GDT);
	CTSS(CGDT& GDT, CTaskState* TaskState, dword DescriptorIndex);
	~CTSS();

	CTaskState& GetTaskState();
	dword GetDescriptorIndex();
	dword GetSelector();
	void JumpToTask();

private:
	void operator=(const CTSS& Src);
	CTSS(const CTSS& Src);

	CGDT& m_GDT;
	CTaskState* m_TaskState;
	dword m_DescriptorIndex;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=