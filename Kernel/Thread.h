// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Thread.h
#include "Library/SmartPtr.h"
#include "Library/Array2.h"
#include "List.h"

#include "Task.h"
#include "GDT.h"
#include "VirtMemManager.h"
#include "UserHeap.h"
#include "SingleMappedChain.h"

#include "Notification.h"
#include "CallRequest.h"
#include "NotificationQueue.h"

#include "FString.h"
#include "UniPtr.h"

#pragma once

// ----------------------------------------------------------------------------
class CProcHeader
{
public:
	dword m_Signature;
	dword m_Version;
	dword m_CRC32;
	dword m_EntryPoint;
	dword m_CodeVSize;
	dword m_RDataVSize;
	dword m_DataVSize;
	dword m_DataRawSize;
	word m_Priority;
	word m_AccessLevel;
};

// ----------------------------------------------------------------------------
class CBlockInfo
{
public:
	CBlockInfo(dword PageBase, dword PageCount)
	{
		m_PageBase = PageBase;
		m_PageCount = PageCount;
	}	

public:
	dword m_PageBase;
	dword m_PageCount;
};

// ----------------------------------------------------------------------------
enum EThreadWaitType
{
	ThreadWaitType_Nothing,
	ThreadWaitType_Notification,
	ThreadWaitType_CallRequest,
	ThreadWaitType_NotificationOrCallRequest,
	ThreadWaitType_CallResponse,
	ThreadWaitType_Ticks,
	ThreadWaitType_Symbol
};

// ----------------------------------------------------------------------------
class CThread
{
public:
	CThread(CPhysMemManager& PMM, CGDT& GDT,
		dword KernelTaskStateBase,
		dword IDTBase,
		dword IntHandleBase,
		bool IsKernel,
		const CUniPtr& Image,
		dword EntryPoint,
		dword CodeSize,
		dword RDataSize,
		dword DataVSize,
		dword DataRawSize,
		byte* ServiceFuncPage,
		byte Priority,
		byte AccessLevel,
		const CFString<128>& Name);
	~CThread();

	dword GetSwitchesCount();
	dword GetUsedPageCount();

	void OnTick(dword TickCount);
	void AddUserPerfData(dword User);
	void AddKernelPerfData(dword Kernel);
	void GetPerfData(dword* User, dword* Kernel);

	dword GetID();
	byte GetPriority();
	byte GetAccessLevel();

	dword GetGPEHandler();
	void SetGPEHandler(dword Address);
	void ResetStack();

	void* MemAlloc(dword ByteCount);
	void MemFree(void* Base);
	void* AllocLinearBlock(dword pageCount);

	byte* GetRing0Stack();
	CTask& GetTask();
	CSmartPtr<CVirtMemManager> GetVMM();
	CVirtMemManager& GetVMMRef();
	const CFString<128>& GetName();

	void WaitForCallResponse(CSmartPtr<CCallRequest> CallRequest);
	void WaitForNotifications();
	void WaitForCallRequests();
	void WaitForNotificationsOrCallRequests();
	void StopWaitingForCallResponse();
	void WaitForTicks(dword TickCount);
	void WaitForSymbol(dword symbol1, dword symbol2);

	void OnSymbol(dword Symbol);

	dword GetNotificationCount();
	dword GetCallRequestCount();

	bool GetFrontNotificationSize(dword& NotificationSize);
	bool GetFrontCallRequestSize(dword& CallRequestSize);
	CSmartPtr<CCallRequest> StartCallRequestProcessing();


	bool PopNotification(dword& SrcPID, dword& NfID, CUniPtr& NfData);

	//bool AddMessage(dword SrcPID, dword MsgID, dword MsgSize, byte* MsgData);
	bool AddNotification(dword SrcPID, dword NfID, dword NfSize, CUniPtr& NfData);



	bool TryAddCallRequest(CSmartPtr<CCallRequest> CallRequest);

	void EnableNotification(dword ID);
	void DisableNotification(dword ID);
	void EnableCallRequest(dword ID);

	void SetFPUStateChange();
	void LoadFPUState();
	void StoreFPUState();

	void SwitchTo();
	bool IsWaiting();

private:
	void* AllocPhysBlock(dword pageCount);
	void* AllocChainBlock(dword pageCount);
	void FreeVirtualBlock(dword VirtualBase);

private:
	dword m_ID;
	byte m_Priority;
	byte m_AccessLevel;

	CPhysMemManager& m_PMM;

	CSmartPtr<CUserHeap> m_UH;
	CSmartPtr<CVirtMemManager> m_VMM;
	CArray<CBlockInfo> m_AllocatedBlocks;
	CArray<CSingleMappedChain> m_MappedChains;

	CSmartPtr<CTask> m_Task;

	bool m_IsFPUStateChanged;
	bool m_IsFPUInitialized;
	byte m_FPUState[108];

	byte* m_Ring0Stack;
	byte* m_Ring3Stack;
	dword m_Ring3StackVBase;

	dword m_GPEHandler;

	static const dword c_StackPageCount = 4;

	CFString<128> m_Name;

	static const dword c_PerfDataSize = 128;
	dword m_UserPerfData[c_PerfDataSize];
	dword m_KernelPerfData[c_PerfDataSize];
	dword m_PerfIndex;

	dword m_SwitchesCount;

	dword m_TicksLeft;
	dword m_WaitingSymbol1;
	dword m_WaitingSymbol2;
	EThreadWaitType m_WaitingFor;
	CSmartPtr<CCallRequest> m_WaitingCallInfo;

	CArray<dword> m_EnabledNotificationIDs;
	CArray<dword> m_EnabledCallIDs;

	//CMessageQueue m_MessageQueue;


	CNotificationQueue m_NotificationQueue;
	CArray<CSmartPtr<CCallRequest> > m_CallRequests;

public: // <-- Temp
	dword m_SrvPID;
	dword m_CltRspBuf;
	dword m_CltRspBufSize;
	dword m_CltCallOutBuf;

	dword m_SymWaitOutBufVPtr;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=