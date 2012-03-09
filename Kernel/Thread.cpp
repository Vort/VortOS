// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Thread.cpp
#include "Thread.h"

// ----------------------------------------------------------------------------
CThread::CThread(CPhysMemManager& PMM, CGDT& GDT, dword KernelTaskStateBase,
				 dword IDTBase, dword IntHandleBase, bool IsKernel,
				 const CUniPtr& Image, dword EntryPoint,
				 dword CodeSize, dword RDataSize, dword DataSize,
				 byte* ServiceFuncPage, byte Priority, byte AccessLevel,
				 const CFString<128>& Name)
		: m_PMM(PMM), m_Name(Name), m_NotificationQueue(PMM, 8) //, m_MessageQueue(PMM, 8)
{
	m_IsFPUInitialized = false;
	m_IsFPUStateChanged = false;

	m_SwitchesCount = 0;

	m_PerfIndex = 0;
	for (dword i = 0; i < c_PerfDataSize; i++)
	{
		m_UserPerfData[i] = 0;
		m_KernelPerfData[i] = 0;
	}

	m_VMM = new CVirtMemManager(PMM, c_VMMFreeVBase);

	static dword g_ID = 0;
	g_ID++;
	m_ID = g_ID;

	m_Priority = Priority;
	m_AccessLevel = AccessLevel;
	m_UH = new CUserHeap(m_PMM, *m_VMM, c_UserHeapVBase);

	m_WaitingFor = ThreadWaitType_Nothing;
	m_TicksLeft = 0;

	dword CodePageCount = (CodeSize + 4095) / 4096;
	dword RDataPageCount = (RDataSize + 4095) / 4096;
	dword DataPageCount = (DataSize + 4095) / 4096;

	m_Ring0Stack = (byte*)AllocBlock(1);
	byte* Stack = (byte*)AllocBlock(c_StackPageCount);
	byte* Code = (byte*)AllocBlock(CodePageCount);
	byte* RData = (byte*)AllocBlock(RDataPageCount);
	byte* Data = (byte*)AllocBlock(DataPageCount);

	dword GDTBase = GDT.GetBase();
	m_VMM->MapPhysPageToVirtPage(GDTBase, GDTBase);
	m_VMM->MapPhysPageToVirtPage(IDTBase, IDTBase);
	m_VMM->MapPhysPageToVirtPage(IntHandleBase, IntHandleBase);

	dword CodeVBase = c_ImageBase + 0x1000;
	dword RDataVBase = CodeVBase + CodePageCount * 0x1000;
	dword DataVBase = RDataVBase + RDataPageCount * 0x1000;

	m_VMM->MapPhysBlockToVirtBlock((dword)Code, CodeVBase, CodePageCount, false);
	m_VMM->MapPhysBlockToVirtBlock((dword)RData, RDataVBase, RDataPageCount, false);
	m_VMM->MapPhysBlockToVirtBlock((dword)Data, DataVBase, DataPageCount);
	m_VMM->MapPhysBlockToVirtBlock((dword)Stack, c_StackVBase, c_StackPageCount);

	m_VMM->MapPhysPageToVirtPage((dword)m_Ring0Stack, c_R0StackVBase);
	m_VMM->MapPhysPageToVirtPage((dword)ServiceFuncPage, c_ServiceFuncVBase, false);

	dword CodeImageBase = sizeof(CProcHeader);
	dword RDataImageBase = CodeImageBase + CodeSize;
	dword DataImageBase = RDataImageBase + RDataSize;
	Image.CopyUtoP(CodeImageBase, CodeSize, Code);
	Image.CopyUtoP(RDataImageBase, RDataSize, RData);
	Image.CopyUtoP(DataImageBase, DataSize, Data);
	*(dword*)(Stack + c_StackPageCount * 4096 - 4) = c_ServiceFuncVBase;

	// Create Task
	m_Task = new CTask(
		GDT, IsKernel,
		(void*)(EntryPoint),
		(byte*)(c_StackVBase),
		(byte*)(c_StackVBase + c_StackPageCount * 4096 - 4),
		(byte*)(c_R0StackVBase),
		(dword)(&m_VMM->GetPD()));

	dword TSPart1 = dword(&m_Task->GetTSS().GetTaskState()) & ~0xFFF;
	dword TSPart2 = TSPart1 + 0x1000;
	m_VMM->MapPhysPageToVirtPage(TSPart1, TSPart1);
	if ((TSPart2 != IntHandleBase) && (TSPart2 != IDTBase) && (TSPart2 != GDTBase))
		m_VMM->MapPhysPageToVirtPage(TSPart2, TSPart2);

	dword TSPart3 = KernelTaskStateBase & ~0xFFF;
	dword TSPart4 = TSPart3 + 0x1000;
	if ((TSPart3 != TSPart1) && (TSPart3 != TSPart2))
		m_VMM->MapPhysPageToVirtPage(TSPart3, TSPart3);
	if ((TSPart4 != TSPart1) && (TSPart4 != TSPart2))
		m_VMM->MapPhysPageToVirtPage(TSPart4, TSPart4);

	m_Task->SetInterrupts(true);
}

// ----------------------------------------------------------------------------
CThread::~CThread()
{
	for (dword i = 0; i < m_AllocatedBlocks.Size(); i++)
	{
		m_PMM.ReleaseBlock(PB(m_AllocatedBlocks[i].m_PageBase),
			m_AllocatedBlocks[i].m_PageCount);
	}
}

// ----------------------------------------------------------------------------
dword CThread::GetUsedPageCount()
{
	dword R = 0;
	R += m_UH->GetPagesCount();
	R += m_VMM->GetPagesCount();
	R += m_NotificationQueue.GetUsedPageCount();
	for (dword i = 0; i < m_AllocatedBlocks.Size(); i++)
		R += m_AllocatedBlocks[i].m_PageCount;
	for (dword i = 0; i < m_MappedChains.Size(); i++)
		R += m_MappedChains[i].GetPageCount();
	return R;
}

// ----------------------------------------------------------------------------
dword CThread::GetSwitchesCount()
{
	return m_SwitchesCount;
}

// ----------------------------------------------------------------------------
void CThread::OnTick(dword TickCount)
{
	m_PerfIndex = (TickCount + 1) % c_PerfDataSize;
	m_UserPerfData[m_PerfIndex] = 0;
	m_KernelPerfData[m_PerfIndex] = 0;

	if (m_WaitingFor == ThreadWaitType_Ticks)
	{
		if (m_TicksLeft == 0)
			m_WaitingFor = ThreadWaitType_Nothing;
		else
			m_TicksLeft--;
	}
}

// ----------------------------------------------------------------------------
void CThread::AddUserPerfData(dword User)
{
	m_UserPerfData[m_PerfIndex] += User;
}

// ----------------------------------------------------------------------------
void CThread::AddKernelPerfData(dword Kernel)
{
	m_KernelPerfData[m_PerfIndex] += Kernel;
}

// ----------------------------------------------------------------------------
void CThread::GetPerfData(dword* User, dword* Kernel)
{
	for (dword i = 0; i < c_PerfDataSize; i++)
		User[i] = m_UserPerfData[i];
	for (dword i = 0; i < c_PerfDataSize; i++)
		Kernel[i] = m_KernelPerfData[i];
}

// ----------------------------------------------------------------------------
dword CThread::GetID()
{
	return m_ID;
}

// ----------------------------------------------------------------------------
void* CThread::MemAlloc(dword ByteCount)
{
	if (ByteCount == 0)
		return null;
	void* R = m_UH->Alloc(ByteCount);
	if (R == 0)
		R = AllocVirtualBlock((ByteCount + 0xFFF) >> 12);
	return R;
}

// ----------------------------------------------------------------------------
void CThread::MemFree(void* Base)
{
	if (!m_UH->Free(PB(Base)))
		FreeVirtualBlock(dword(Base));
}

// ----------------------------------------------------------------------------
void* CThread::AllocBlock(dword PageCount)
{
	void* BlockBase = m_PMM.AllocBlock(PageCount);
	m_AllocatedBlocks.PushBack(CBlockInfo(dword(BlockBase), PageCount));
	return BlockBase;
}

// ----------------------------------------------------------------------------
void* CThread::AllocVirtualBlock(dword PageCount)
{
	CSingleMappedChain MC(m_PMM, m_VMM, PageCount);
	m_MappedChains.PushBack(MC);
	return PV(MC.GetVirtualBase());
}

// ----------------------------------------------------------------------------
void CThread::FreeVirtualBlock(dword VirtualBase)
{
	for (dword i = 0; i < m_MappedChains.Size(); i++)
		if (m_MappedChains[i].GetVirtualBase() == VirtualBase)
		{
			m_MappedChains.Delete(i);
			return;
		}
}

// ----------------------------------------------------------------------------
byte CThread::GetPriority()
{
	return m_Priority;
}

// ----------------------------------------------------------------------------
byte CThread::GetAccessLevel()
{
	return m_AccessLevel;
}

// ----------------------------------------------------------------------------
const CFString<128>& CThread::GetName()
{
	return m_Name;
}

// ----------------------------------------------------------------------------
void CThread::SetFPUStateChange()
{
	m_IsFPUStateChanged = true;
}

// ----------------------------------------------------------------------------
void CThread::LoadFPUState()
{
	if (m_IsFPUStateChanged)
	{
		if (!m_IsFPUInitialized)
		{
			__asm
			{
				clts
				fninit
			}
			m_IsFPUInitialized = true;
		}
		else
		{
			byte* FPUState = m_FPUState;
			__asm
			{
				clts
				mov  eax, FPUState
				frstor [eax]
			}
		}
	}
}

// ----------------------------------------------------------------------------
void CThread::StoreFPUState()
{
	if (m_IsFPUStateChanged)
	{
		byte* FPUState = m_FPUState;
		__asm
		{
			clts
			mov  eax, FPUState
			fnsave [eax]
		}
		m_IsFPUStateChanged = false;
	}
}

// ----------------------------------------------------------------------------
void CThread::SwitchTo()
{
	m_SwitchesCount++;
	m_Task->JumpTo();
}

// ----------------------------------------------------------------------------
void CThread::OnSymbol(dword Symbol)
{
	if (m_WaitingFor == ThreadWaitType_Symbol)
		if (m_WaitingSymbol == Symbol)
			m_WaitingFor = ThreadWaitType_Nothing;
}

// ----------------------------------------------------------------------------
void CThread::WaitForSymbol(dword Symbol)
{
	m_WaitingSymbol = Symbol;
	m_WaitingFor = ThreadWaitType_Symbol;
}

// ----------------------------------------------------------------------------
void CThread::WaitForTicks(dword TickCount)
{
	m_TicksLeft = TickCount;
	m_WaitingFor = ThreadWaitType_Ticks;
}

// ----------------------------------------------------------------------------
void CThread::WaitForNotifications()
{
	if (m_NotificationQueue.GetNotificationCount() == 0)
		if (m_WaitingFor == ThreadWaitType_Nothing)
			m_WaitingFor = ThreadWaitType_Notification;
}

// ----------------------------------------------------------------------------
void CThread::WaitForNotificationsOrCallRequests()
{
	if ((m_NotificationQueue.GetNotificationCount() == 0) || (m_CallRequests.Size() == 0))
		if (m_WaitingFor == ThreadWaitType_Nothing)
			m_WaitingFor = ThreadWaitType_NotificationOrCallRequest;
}

// ----------------------------------------------------------------------------
void CThread::WaitForCallRequests()
{
	if (m_CallRequests.Size() == 0)
		if (m_WaitingFor == ThreadWaitType_Nothing)
			m_WaitingFor = ThreadWaitType_CallRequest;
}

// ----------------------------------------------------------------------------
void CThread::WaitForCallResponse(CSmartPtr<CCallRequest> CallRequest)
{
	m_WaitingFor = ThreadWaitType_CallResponse;
	m_WaitingCallInfo = CallRequest;
}

// ----------------------------------------------------------------------------
void CThread::StopWaitingForCallResponse()
{
	if (m_WaitingFor == ThreadWaitType_CallResponse)
	{
		m_WaitingFor = ThreadWaitType_Nothing;
		m_WaitingCallInfo = null;
	}
}

// ----------------------------------------------------------------------------
dword CThread::GetNotificationCount()
{
	return m_NotificationQueue.GetNotificationCount();
}

// ----------------------------------------------------------------------------
dword CThread::GetCallRequestCount()
{
	return m_CallRequests.Size();
}

// ----------------------------------------------------------------------------
bool CThread::GetFrontNotificationSize(dword& NotificationSize)
{
	if (m_NotificationQueue.GetNotificationCount() == 0)
		return false;
	NotificationSize = m_NotificationQueue.GetFrontNotificationSize();
	return true;
}

// ----------------------------------------------------------------------------
bool CThread::GetFrontCallRequestSize(dword& CallRequestSize)
{
	if (m_CallRequests.Size() == 0)
		return false;
	CallRequestSize = m_CallRequests[0]->GetSize();
	return true;
}

// ----------------------------------------------------------------------------
bool CThread::PopNotification(dword& SrcPID, dword& NfID, CUniPtr& NfData)
{
	if (m_NotificationQueue.GetNotificationCount() == 0)
		return false;
	return m_NotificationQueue.PopNotification(SrcPID, NfID, NfData);
}

// ----------------------------------------------------------------------------
CSmartPtr<CCallRequest> CThread::StartCallRequestProcessing()
{
	if (m_CallRequests.Size() == 0)
		return null;
	CSmartPtr<CCallRequest> CallRequest = m_CallRequests.PopFront();
	return CallRequest;
}

/*
// ----------------------------------------------------------------------------
bool CThread::AddMessage(dword SrcPID, dword MsgID, dword MsgSize, byte* MsgData)
{
	// пока что Nf Enable, потом - Msg
	bool IsEnabled = false;
	for (dword i = 0; i < m_EnabledNotificationIDs.Size(); i++)
		if (m_EnabledNotificationIDs[i] == MsgID)
			IsEnabled = true;

	if (!IsEnabled)
		return true;

	if (!m_MessageQueue.AddMessage(SrcPID, MsgID, MsgSize, MsgData))
		return false;

	return true;
}
*/

// ----------------------------------------------------------------------------
bool CThread::AddNotification(dword SrcPID, dword NfID, dword NfSize, CUniPtr& NfData)
{
	bool IsEnabled = false;
	for (dword i = 0; i < m_EnabledNotificationIDs.Size(); i++)
		if (m_EnabledNotificationIDs[i] == NfID)
			IsEnabled = true;

	if (!IsEnabled)
		return true;

	if (!m_NotificationQueue.AddNotification(SrcPID, NfID, NfSize, NfData))
		return false;

	if (m_WaitingFor == ThreadWaitType_Notification)
		m_WaitingFor = ThreadWaitType_Nothing;
	else if (m_WaitingFor == ThreadWaitType_NotificationOrCallRequest)
		m_WaitingFor = ThreadWaitType_Nothing;
	return true;
}

// ----------------------------------------------------------------------------
bool CThread::TryAddCallRequest(CSmartPtr<CCallRequest> CallRequest)
{
	bool IsEnabled = false;
	dword CallTypeID = CallRequest->GetTypeID();
	for (dword i = 0; i < m_EnabledCallIDs.Size(); i++)
		if (m_EnabledCallIDs[i] == CallTypeID)
			IsEnabled = true;

	if (!IsEnabled)
		return false;

	if (m_CallRequests.Size() < 128)
		m_CallRequests.PushBack(CallRequest);

	if (m_WaitingFor == ThreadWaitType_CallRequest)
		m_WaitingFor = ThreadWaitType_Nothing;
	else if (m_WaitingFor == ThreadWaitType_NotificationOrCallRequest)
		m_WaitingFor = ThreadWaitType_Nothing;

	return true;
}

// ----------------------------------------------------------------------------
void CThread::EnableNotification(dword ID)
{
	for (dword i = 0; i < m_EnabledNotificationIDs.Size(); i++)
		if (m_EnabledNotificationIDs[i] == ID)
			return;
	m_EnabledNotificationIDs.PushBack(ID);
}

// ----------------------------------------------------------------------------
void CThread::DisableNotification(dword ID)
{
	for (dword i = 0; i < m_EnabledNotificationIDs.Size(); i++)
		if (m_EnabledNotificationIDs[i] == ID)
		{
			m_EnabledNotificationIDs.Delete(i);
			return;
		}
}

// ----------------------------------------------------------------------------
void CThread::EnableCallRequest(dword ID)
{
	for (dword i = 0; i < m_EnabledCallIDs.Size(); i++)
		if (m_EnabledCallIDs[i] == ID)
			return;
	m_EnabledCallIDs.PushBack(ID);
}

// ----------------------------------------------------------------------------
bool CThread::IsWaiting()
{
	return m_WaitingFor != ThreadWaitType_Nothing;
}

// ----------------------------------------------------------------------------
byte* CThread::GetRing0Stack()
{
	return m_Ring0Stack;
}

// ----------------------------------------------------------------------------
CTask& CThread::GetTask()
{
	return *m_Task;
}

// ----------------------------------------------------------------------------
CSmartPtr<CVirtMemManager> CThread::GetVMM()
{
	return m_VMM;
}

// ----------------------------------------------------------------------------
CVirtMemManager& CThread::GetVMMRef()
{
	return *m_VMM;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=