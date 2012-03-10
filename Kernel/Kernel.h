// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Kernel.h
#pragma once
#include "Library/Defs.h"
#include "Library/Array2.h"
#include "Library/SmartPtr.h"

#include "Task.h"
#include "Thread.h"

#include "IntManager.h"
#include "VirtMemManager.h"
#include "MultiMappedChain.h"

#include "UniPtr.h"

// ----------------------------------------------------------------------------
class CDriverInfo
{
public:
	byte* GetImageBase()
	{
		return PB(m_LoadPage * 0x1000);
	}

	dword GetImagePageCount()
	{
		return (m_BytesSize + 0xFFF) >> 12;
	}

public:
	dword m_BytesSize;
	dword m_LoadPage;
	char m_Name[0x20];
};

// ----------------------------------------------------------------------------
class CKernel
{
public:
	CKernel(CTask& KernelTask, CPhysMemManager& PMM, CIntManager& IM,
		CGDT& GDT, CIDT& IDT, dword BootType,
		CDriverInfo* DriverInfos, dword DriversCount);

	void InitPreloadedDrv(dword Index, CDriverInfo& DriverInfo);

	void OnKeCall(dword FunctionIndex);
	void ProcessHWIntRequest(dword Index);
	void ProcessInt30Request();

	bool SetSymbol(dword Symbol);
	bool HasSymbol(dword Symbol);

	bool AddThread(bool IsKernel, const CUniPtr& Image,
		dword ImageSize, const CFString<128>& Name);
	CThread* GetThreadWithID(dword ID);
	void RemoveActiveThread();
	void ActivateThread();

	void Loop();
	void StartReboot();
	void ProcessReboot();
	void NotifyTermination();
	void Reboot();

	bool BroadcastNotification(dword SrcPID, dword NfID);
	bool BroadcastNotification(dword SrcPID, dword NfID, dword NfSize, CUniPtr& NfData);


	void OnKeUnmaskIRQ();
	void OnKeExitProcess();
	void OnKeRequestReboot();
	void OnKeEnableNotification();
	void OnKeWaitFor();
	void OnKeGetNotification();
	void OnKeNotify();
	void OnKeEnableCallRequest();
	void OnKeRequestCall();
	void OnKeRespondCall();
	void OnKeGetCallRequest();
	void OnKeOutPortByte();
	void OnKeInPortByte();
	void OnKeOutPortWord();
	void OnKeInPortWord();
	void OnKeWaitTicks();
	void OnKeIsSymbolSet();
	void OnKeSetSymbol();
	void OnKeWaitForSymbol();
	void OnKeInPortWordArray();
	void OnKeCreateProcess();
	void OnKeGetNotificationCount();
	void OnKeGetCallRequestCount();
	void OnKeGetNfClCount();
	void OnKeDisableNotification();
	void OnKeMemAlloc();
	void OnKeMemFree();
	void OnKeVirtToPhys();
	void OnKeAllocSharedMem();
	void OnKeMapSharedMem();
	void OnKeReleaseSharedMem();
	void OnKeOutPortDword();
	void OnKeInPortDword();
	void OnKeGetMemInfo();
	void OnKeGetSharedMemSize();
	void OnKeAllocSharedMemAt();
	void OnKeGetPreloadedDriversCount();
	void OnKeGetTime();
	void OnKeGetNextProcessInfo();
	void OnKeGetBootType();

private:
	dword m_TickCount;

	bool m_IsRebootActivated;
	dword m_RebootLevel;
	dword m_RebootLevelStartTick;

	dword m_BootType;
	dword m_PreloadedDriversCount;

	CTask& m_KernelTask;
	CPhysMemManager& m_PMM;
	CIntManager& m_IM;
	CGDT& m_GDT;
	CIDT& m_IDT;

	CArray<CSmartPtr<CMultiMappedChain> > m_MMChains;

	byte* m_ServiceFuncPage;
	byte* m_KeCallInDataBuf;
	byte* m_KeCallOutDataBuf;
	byte* m_TempBuf;

	dword m_KeCallInDataSize;
	dword m_KeCallOutDataSize;
	byte* m_KeCallOutDataVPtr;
	dword m_KeCallRealOutDataSize;

	CThread* m_ActiveThread;
	CList<CThread> m_TL;

	CArray<dword> m_Symbols;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=