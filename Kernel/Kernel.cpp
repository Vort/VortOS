// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Kernel.cpp
#include "Kernel.h"
#include "Global.h"
#include "FString.h"
#include "Intrinsics.h"
#include "CRC32.h"

// ----------------------------------------------------------------------------
static byte g_IdleImage[35] =
{
	0x56, 0x45, 0x78, 0x65,
	0x03, 0x00, 0x00, 0x00,
	0x98, 0xA6, 0x0A, 0x33,
	0x00, 0x10, 0x40, 0x00,
	0x03, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xFF, 0x00,
	0x00, 0x00,

	0xF4, 0xEB, 0xFD
};

// ----------------------------------------------------------------------------
static byte g_ServiceFuncPageData[14] =
{
	0x31, 0xC0, 0x40, 0x40, 0x31, 0xDB, 0x31, 0xC9,
	0x31, 0xF6, 0x31, 0xFF, 0xCD, 0x30
};

// ----------------------------------------------------------------------------
void RawOutString(const char* Msg, dword X, dword Y, byte Color);

// ----------------------------------------------------------------------------
CKernel::CKernel(CTask& KernelTask, CPhysMemManager& PMM, CIntManager& IM,
	CGDT& GDT, CIDT& IDT, dword BootType,
	CDriverInfo* DriverInfos, dword DriversCount)
	: m_KernelTask(KernelTask), m_PMM(PMM), m_GDT(GDT), m_IDT(IDT), m_IM(IM)
{
	m_TickCount = 0;
	m_IsRebootActivated = false;

	m_ServiceFuncPage = PMM.AllocPage();
	m_KeCallInDataBuf = PMM.AllocBlock(8);
	m_KeCallOutDataBuf = PMM.AllocBlock(8);
	m_TempBuf = PMM.AllocBlock(4);

	for (dword i = 0; i < 4096; i++)
		m_ServiceFuncPage[i] = 0xCC; // int3
	for (dword i = 0; i < sizeof(g_ServiceFuncPageData); i++)
		m_ServiceFuncPage[i] = g_ServiceFuncPageData[i];

	AddThread(true, g_IdleImage, sizeof(g_IdleImage), "[Idle]");

	m_BootType = BootType;
	m_PreloadedDriversCount = DriversCount;
	for (dword i = 0; i < DriversCount; i++)
	{
		InitPreloadedDrv(i + 1, DriverInfos[i]);
		PMM.ReleaseBlock(
			DriverInfos[i].GetImageBase(),
			DriverInfos[i].GetImagePageCount());
	}

	PMM.ReleaseBlock(PB(0xB8000), 0x8); // 0x0B8000 - Video RAM
	Loop();
}

// ----------------------------------------------------------------------------
void CKernel::InitPreloadedDrv(dword Index, CDriverInfo& DriverInfo)
{
	dword NameSize = 0;
	for (; DriverInfo.m_Name[NameSize] ;NameSize++);

	RawOutString("Checking '", 0, Index, 0xA);
	RawOutString(DriverInfo.m_Name, 10, Index, 0xA);
	RawOutString("'...", 10 + NameSize, Index, 0xA);
	if (AddThread(false, DriverInfo.GetImageBase(),
		DriverInfo.m_BytesSize, DriverInfo.m_Name))
	{
		RawOutString("OK", 10 + NameSize + 4, Index, 0xA);
	}
	else
		RawOutString("Fail", 10 + NameSize + 4, Index, 0xC);
}

// ----------------------------------------------------------------------------
bool CKernel::AddThread(bool IsKernel, const CUniPtr& Image,
	dword ImageSize, const CFString<128>& Name)
{
	if (ImageSize < sizeof(CProcHeader))
		return false;

	CProcHeader Header;
	Image.CopyUtoP(0, sizeof(CProcHeader), (byte*)(&Header));

	if (Header.m_Signature != 'exEV') return false;
	if (Header.m_Version != 3) return false;
	if (Header.m_CodeVSize + Header.m_DataVSize + Header.m_RDataVSize + sizeof(CProcHeader) != ImageSize)
		return false;

	if (CCRC32().GetCRC32(Image, sizeof(CProcHeader), ImageSize - sizeof(CProcHeader)) != Header.m_CRC32)
		return false;

	m_TL.AppendElement(new CThread(m_PMM, m_GDT,
		dword(&m_KernelTask.GetTSS().GetTaskState()),
		m_IDT.GetBase(), m_IM.GetHandlersBase(), IsKernel,
		Image, Header.m_EntryPoint, Header.m_CodeVSize, Header.m_DataVSize,
		Header.m_RDataVSize, m_ServiceFuncPage, Header.m_Priority, Header.m_AccessLevel, Name));
	return true;
}

// ----------------------------------------------------------------------------
void CKernel::RemoveActiveThread()
{
	if (m_ActiveThread)
	{
		dword ActiveThreadID = m_ActiveThread->GetID();
		BroadcastNotification(0, NfKe_ProcessExited, 4, CUniPtr(PB(&ActiveThreadID)));

		m_TL.ToLastElement();
		m_TL.RemoveCurrentElement();
		m_ActiveThread = null;
	}
}

// ----------------------------------------------------------------------------
CThread* CKernel::GetThreadWithID(dword ID)
{
	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
	{
		CThread* T = m_TL.GetCurrentElement();
		if (T->GetID() == ID)
			return T;
	}
	return null;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeUnmaskIRQ()
{
	if (m_KeCallInDataSize != 1) return;
	if (m_KeCallOutDataSize != 0) return;

	byte IRQ = m_KeCallInDataBuf[0];
	if (IRQ < 0x10)
		m_IM.UnmaskIRQ(IRQ);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeExitProcess()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 0) return;

	RemoveActiveThread();
}

// ----------------------------------------------------------------------------
void CKernel::OnKeRequestReboot()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 0) return;

	StartReboot();
}

// ----------------------------------------------------------------------------
void CKernel::OnKeEnableNotification()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword NotificationID = (PD(m_KeCallInDataBuf))[0];
	m_ActiveThread->EnableNotification(NotificationID);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeWaitFor()
{
	if (m_KeCallInDataSize != 1) return;
	if (m_KeCallOutDataSize != 0) return;

	byte Key = m_KeCallInDataBuf[0];
	if (Key == 0)          // Nothing
	{
	}
	else if (Key == 1)     // Notification
	{
		if (m_ActiveThread->GetNotificationCount() == 0)
			m_ActiveThread->WaitForNotifications();
	}
	else if (Key == 2)     // Call
	{
		if (m_ActiveThread->GetCallRequestCount() == 0)
			m_ActiveThread->WaitForCallRequests();
	}
	else if (Key == 3)     // Notification + Call
	{
		if ((m_ActiveThread->GetNotificationCount() == 0) &&
			(m_ActiveThread->GetCallRequestCount() == 0))
		{
			m_ActiveThread->WaitForNotificationsOrCallRequests();
		}
	}
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetNotification()
{
	if (m_KeCallInDataSize != 8) return;
	if (m_KeCallOutDataSize != 13) return;

	// Out Buffer
	// 0       - Result
	// 1 2 3 4 - Notification Size
	// 5 6 7 8 - Notification ID
	// 9 A B C - Source PID

	dword NfBufSize = (PD(m_KeCallInDataBuf))[0];
	dword NfBufAddr = (PD(m_KeCallInDataBuf))[1];

	byte Result;
	if (m_ActiveThread->GetNotificationCount() == 0)
		Result = 1; // Notification list is empty
	else
	{
		dword NfSize = 0;
		m_ActiveThread->GetFrontNotificationSize(NfSize);
		*PD(&m_KeCallOutDataBuf[1]) = NfSize;

		if (NfBufSize >= NfSize)
		{
			CSmartPtr<CVirtMemManager> VMM = m_ActiveThread->GetVMM();
			if (!VMM->CheckArray(PB(NfBufAddr), NfSize))
				Result = 3; // Buf Error
			else
			{
				m_ActiveThread->PopNotification(
					*PD(&m_KeCallOutDataBuf[9]),
					*PD(&m_KeCallOutDataBuf[5]),
					CUniPtr(VMM, PB(NfBufAddr)));
				Result = 0; // Success
			}
		}
		else
			Result = 2; // Not enough buffer size
	}
	m_KeCallOutDataBuf[0] = Result;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeNotify()
{
	if (m_KeCallInDataSize != 12) return;
	if (m_KeCallOutDataSize != 1) return;

	dword NfBufSize = (PD(m_KeCallInDataBuf))[0];
	dword NfBufAddr = (PD(m_KeCallInDataBuf))[1];
	dword NfID = (PD(m_KeCallInDataBuf))[2];
	dword SrcPID = m_ActiveThread->GetID();

	byte Result = 0;
	bool BroadcastSuccess = true;

	if ((NfBufSize == 0) || (NfBufAddr == 0))
	{
		if (!BroadcastNotification(SrcPID, NfID))
			Result = 1; // Delivery failed
	}
	else if (NfBufSize <= 0x4000)
	{
		CSmartPtr<CVirtMemManager> VMM = m_ActiveThread->GetVMM();
		if (!VMM->CheckArray(PB(NfBufAddr), NfBufSize))
			Result = 2; // Copy error
		if (!BroadcastNotification(SrcPID, NfID, NfBufSize, CUniPtr(VMM, PB(NfBufAddr))))
			Result = 1; // Delivery failed
	}
	else
		Result = 3; // Buffer too big

	m_KeCallOutDataBuf[0] = Result;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeEnableCallRequest()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword CallID = (PD(m_KeCallInDataBuf))[0];
	m_ActiveThread->EnableCallRequest(CallID);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeRequestCall()
{
	if (m_KeCallInDataSize != 20) return;
	if (m_KeCallOutDataSize != 5) return;

	dword InBufferSize = (PD(m_KeCallInDataBuf))[0];
	dword InBufferAddr = (PD(m_KeCallInDataBuf))[1];
	dword OutBufferSize = (PD(m_KeCallInDataBuf))[2];
	dword OutBufferAddr = (PD(m_KeCallInDataBuf))[3];
	dword CallTypeID = (PD(m_KeCallInDataBuf))[4];

	byte Result = 0;
	if ((InBufferSize > 0x4000) || (OutBufferSize > 0x4000))
	{
		Result = 1; // Buffers too large
	}
	else
	{
		bool IsDestinationFound = false;
		dword SrcPID = m_ActiveThread->GetID();
		CSmartPtr<CCallRequest> CallRequest;
		if ((InBufferSize == 0) || (InBufferAddr == 0))
		{
			CallRequest = new CCallRequest(CallTypeID,
				SrcPID, null, 0);
		}
		else
		{
			if (!m_ActiveThread->GetVMMRef().MemCopyVirtToPhys(
				PB(InBufferAddr), m_TempBuf, InBufferSize))
			{
				Result = 2; // In Copy Error
			}
			else
			{
				CallRequest = new CCallRequest(CallTypeID, SrcPID,
					m_TempBuf, InBufferSize);
			}
		}
		
		if (Result != 2)
		{
			for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
				if (m_TL.GetCurrentElement()->TryAddCallRequest(CallRequest))
				{
					m_ActiveThread->m_SrvPID = m_TL.GetCurrentElement()->GetID();
					m_ActiveThread->m_CltRspBuf = OutBufferAddr;
					m_ActiveThread->m_CltRspBufSize = OutBufferSize;
					m_ActiveThread->m_CltCallOutBuf = dword(m_KeCallOutDataVPtr);

					m_ActiveThread->WaitForCallResponse(CallRequest);
					IsDestinationFound = true;
					break;
				}

			if (!IsDestinationFound)
				Result = 3; // Destination not found
		}
	}
	m_KeCallOutDataBuf[0] = Result;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeRespondCall()
{
	if (m_KeCallInDataSize != 12) return;
	if (m_KeCallOutDataSize != 0) return;

	dword SrvRspBufSize = (PD(m_KeCallInDataBuf))[0];
	dword SrvRspBuf     = (PD(m_KeCallInDataBuf))[1];
	dword CltPID        = (PD(m_KeCallInDataBuf))[2];

	CThread* T = GetThreadWithID(CltPID);
	if (T->m_SrvPID == m_ActiveThread->GetID())
	{
		dword CltRspBuf = T->m_CltRspBuf;
		dword CltRspBufSize = T->m_CltRspBufSize;
		dword CltCallOutBuf = T->m_CltCallOutBuf;

		byte CalleeResult = 0;
		if (
			(SrvRspBufSize == 0) || (SrvRspBuf == null) ||
			(CltRspBufSize == 0) || (CltRspBuf == null)
			)
		{
		}
		else if (CltRspBufSize >= SrvRspBufSize)
		{
			if (!m_ActiveThread->GetVMMRef().MemCopyVirtToPhys(
				PB(SrvRspBuf), m_TempBuf, SrvRspBufSize))
			{
				CalleeResult = 5; // Out Copy Error 1
			}
			else if (!T->GetVMMRef().MemCopyPhysToVirt(
				m_TempBuf, PB(CltRspBuf), SrvRspBufSize))
			{
				CalleeResult = 6; // Out Copy Error 2
			}
			else
			{

			}
		}
		else
			CalleeResult = 4; // Not enough out buffer size

		T->GetVMMRef().MemCopyPhysToVirt(
			&CalleeResult, PB(CltCallOutBuf), 1);

		if (CalleeResult == 0)
		{
			T->GetVMMRef().MemCopyPhysToVirt(
				PB(&SrvRspBufSize), PB(CltCallOutBuf + 1), 4);
		}
		T->StopWaitingForCallResponse();
	}
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetCallRequest()
{
	if (m_KeCallInDataSize != 8) return;
	if (m_KeCallOutDataSize != 13) return;

	dword RequestBufferSize = (PD(m_KeCallInDataBuf))[0];
	dword RequestBufferAddr = (PD(m_KeCallInDataBuf))[1];

	// Out Buffer
	// 0       - Result
	// 1 2 3 4 - Call Request Size
	// 5 6 7 8 - Call Request Type ID
	// 9 A B C - Client PID

	byte Result = 0;
	if (m_ActiveThread->GetCallRequestCount() == 0)
		Result = 1; // Call Request list is empty
	else
	{
		dword CallRequestSize = 0;
		m_ActiveThread->GetFrontCallRequestSize(CallRequestSize);
		*PD(&m_KeCallOutDataBuf[1]) = CallRequestSize;

		if (RequestBufferSize >= CallRequestSize)
		{
			CSmartPtr<CCallRequest> CallRequest =
				m_ActiveThread->StartCallRequestProcessing();
			*PD(&m_KeCallOutDataBuf[5]) = CallRequest->GetTypeID();
			*PD(&m_KeCallOutDataBuf[9]) = CallRequest->GetCltPID();

			if (m_ActiveThread->GetVMMRef().MemCopyPhysToVirt(
				CallRequest->GetData()._ptr(), PB(RequestBufferAddr),
				CallRequestSize))
			{
				Result = 0; // Success
			}
			else
				Result = 3; // Copy Error
		}
		else
		{
			Result = 2; // Not enough buffer size
		}
	}
	m_KeCallOutDataBuf[0] = Result;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeOutPortByte()
{
	if (m_KeCallInDataSize != 3) return;
	if (m_KeCallOutDataSize != 0) return;

	word Port = (PW(m_KeCallInDataBuf))[0];
	byte Byte = m_KeCallInDataBuf[2];

	_outp(Port, Byte);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeInPortByte()
{
	if (m_KeCallInDataSize != 2) return;
	if (m_KeCallOutDataSize != 1) return;

	word Port = (PW(m_KeCallInDataBuf))[0];
	byte Byte = _inp(Port) & 0xFF;

	m_KeCallOutDataBuf[0] = Byte;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeOutPortWord()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	word Port = (PW(m_KeCallInDataBuf))[0];
	word Word = (PW(m_KeCallInDataBuf))[1];

	_outpw(Port, Word);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeInPortWord()
{
	if (m_KeCallInDataSize != 2) return;
	if (m_KeCallOutDataSize != 2) return;

	word Port = (PW(m_KeCallInDataBuf))[0];
	word Word = _inpw(Port);

	*PW(&m_KeCallOutDataBuf[0]) = Word;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeWaitTicks()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword TickCount = (PD(m_KeCallInDataBuf))[0];
	m_ActiveThread->WaitForTicks(TickCount);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeIsSymbolSet()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 1) return;

	dword Symbol = (PD(m_KeCallInDataBuf))[0];
	byte IsSet = HasSymbol(Symbol) ? 1 : 0;
	m_KeCallOutDataBuf[0] = IsSet;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeSetSymbol()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 1) return;

	dword Symbol = (PD(m_KeCallInDataBuf))[0];
	byte Success = SetSymbol(Symbol) ? 1 : 0;
	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
		m_TL.GetCurrentElement()->OnSymbol(Symbol);

	m_KeCallOutDataBuf[0] = Success;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeWaitForSymbol()
{
	if (m_KeCallInDataSize == 4)
	{
		if (m_KeCallOutDataSize != 0)
			return;

		dword symbol = (PD(m_KeCallInDataBuf))[0];
		if (!HasSymbol(symbol))
		{
			m_ActiveThread->m_SymWaitOutBufVPtr = 0;
			m_ActiveThread->WaitForSymbol(symbol, symbol);
		}
	}
	else if (m_KeCallInDataSize == 8)
	{
		if (m_KeCallOutDataSize != 4)
			return;

		dword symbol1 = (PD(m_KeCallInDataBuf))[0];
		dword symbol2 = (PD(m_KeCallInDataBuf))[1];
		bool hasSymbol1 = HasSymbol(symbol1);
		bool hasSymbol2 = HasSymbol(symbol2);
		if (hasSymbol1)
			(PD(m_KeCallOutDataBuf))[0] = symbol1;
		else if (hasSymbol2)
			(PD(m_KeCallOutDataBuf))[0] = symbol2;
		else
		{
			(PD(m_KeCallOutDataBuf))[0] = 0;
			m_ActiveThread->m_SymWaitOutBufVPtr = dword(m_KeCallOutDataVPtr);
			m_ActiveThread->WaitForSymbol(symbol1, symbol2);
		}
	}
}

// ----------------------------------------------------------------------------
void CKernel::OnKeInPortWordArray()
{
	if (m_KeCallInDataSize != 8) return;

	dword Port = (PD(m_KeCallInDataBuf))[0];
	dword WordCount = (PD(m_KeCallInDataBuf))[1];

	if (WordCount > 4096)  return;
	if (m_KeCallOutDataSize < WordCount * 2) return;

	for (dword i = 0; i < WordCount; i++)
		(PW(m_KeCallOutDataBuf))[i] = _inpw(Port);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeCreateProcess()
{
	if (m_KeCallInDataSize != 12) return;
	if (m_KeCallOutDataSize != 1) return;

	dword Size = (PD(m_KeCallInDataBuf))[0];
	dword Image = (PD(m_KeCallInDataBuf))[1];
	dword Name = (PD(m_KeCallInDataBuf))[2];

	CSmartPtr<CVirtMemManager> VMM = m_ActiveThread->GetVMM();

	CFString<128> S = CFString<128>(CUniPtr(VMM, (byte*)Name));
	bool Result = AddThread(false, CUniPtr(VMM, (byte*)Image), Size, S);

	if (Result)
		m_KeCallOutDataBuf[0] = 0;
	else
		m_KeCallOutDataBuf[0] = 1;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetNotificationCount()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 4) return;

	(PD(m_KeCallOutDataBuf))[0] = m_ActiveThread->GetNotificationCount();
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetCallRequestCount()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 4) return;

	(PD(m_KeCallOutDataBuf))[0] = m_ActiveThread->GetCallRequestCount();
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetNfClCount()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 8) return;

	(PD(m_KeCallOutDataBuf))[0] = m_ActiveThread->GetNotificationCount();
	(PD(m_KeCallOutDataBuf))[1] = m_ActiveThread->GetCallRequestCount();
}

// ----------------------------------------------------------------------------
void CKernel::OnKeDisableNotification()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword NotificationID = (PD(m_KeCallInDataBuf))[0];
	m_ActiveThread->DisableNotification(NotificationID);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeMemAlloc()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 4) return;

	dword MemSize = (PD(m_KeCallInDataBuf))[0];
	dword MemBase = dword(m_ActiveThread->MemAlloc(MemSize));
	(PD(m_KeCallOutDataBuf))[0] = MemBase;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeMemFree()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword MemBase = (PD(m_KeCallInDataBuf))[0];
	m_ActiveThread->MemFree(PV(MemBase));
}

// ----------------------------------------------------------------------------
void CKernel::OnKeVirtToPhys()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 4) return;

	byte* Virt = PB((PD(m_KeCallInDataBuf))[0]);
	byte* Phys = null;
	m_ActiveThread->GetVMMRef().TranslateVirtToPhys(Virt, Phys);
	(PD(m_KeCallOutDataBuf))[0] = dword(Phys);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeAllocSharedMem()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 4) return;

	dword MemSize = (PD(m_KeCallInDataBuf))[0];
	CSmartPtr<CMultiMappedChain> MMC =
		new CMultiMappedChain(m_PMM, MemSize);
	m_MMChains.PushBack(MMC);
	dword SMID = dword(MMC->GetID());
	(PD(m_KeCallOutDataBuf))[0] = SMID;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeMapSharedMem()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 4) return;

	dword VirtualBase = 0;
	dword SMID = (PD(m_KeCallInDataBuf))[0];
	for (dword i = 0; i < m_MMChains.Size(); i++)
		if (m_MMChains[i]->GetID() == SMID)
		{
			VirtualBase = m_MMChains[i]->MapProcess(
				m_ActiveThread->GetID(),
				m_ActiveThread->GetVMM());
			break;
		}
	(PD(m_KeCallOutDataBuf))[0] = VirtualBase;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeReleaseSharedMem()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword SMID = (PD(m_KeCallInDataBuf))[0];
	for (dword i = 0; i < m_MMChains.Size(); i++)
		if (m_MMChains[i]->GetID() == SMID)
		{
			m_MMChains.Delete(i);
			break;
		}
}

// ----------------------------------------------------------------------------
void CKernel::OnKeOutPortDword()
{
	if (m_KeCallInDataSize != 6) return;
	if (m_KeCallOutDataSize != 0) return;

	word Port = (PW(m_KeCallInDataBuf))[0];
	dword Dword = *PD(&m_KeCallInDataBuf[2]);

	_outpd(Port, Dword);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeInPortDword()
{
	if (m_KeCallInDataSize != 2) return;
	if (m_KeCallOutDataSize != 4) return;

	word Port = (PW(m_KeCallInDataBuf))[0];
	dword Dword = _inpd(Port);

	*PD(&m_KeCallOutDataBuf[0]) = Dword;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetMemInfo()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 16) return;

	dword HeapUsed = g_SysHeap->GetUsedSize();
	dword HeapTotal = g_SysHeap->GetTotalSize();

	dword PageUsed = m_PMM.GetUsedPagesCount();
	dword PageTotal = m_PMM.GetTotalPagesCount();

	(PD(m_KeCallOutDataBuf))[0] = HeapUsed;
	(PD(m_KeCallOutDataBuf))[1] = HeapTotal;
	(PD(m_KeCallOutDataBuf))[2] = PageUsed;
	(PD(m_KeCallOutDataBuf))[3] = PageTotal;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetSharedMemSize()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 4) return;

	dword SMID = (PD(m_KeCallInDataBuf))[0];
	dword Size = 0;
	for (dword i = 0; i < m_MMChains.Size(); i++)
		if (m_MMChains[i]->GetID() == SMID)
		{
			Size = m_MMChains[i]->GetSize();
			break;
		}
	(PD(m_KeCallOutDataBuf))[0] = Size;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeAllocSharedMemAt()
{
	if (m_KeCallInDataSize != 8) return;
	if (m_KeCallOutDataSize != 4) return;

	dword PhysAddr = (PD(m_KeCallInDataBuf))[0];
	dword MemSize = (PD(m_KeCallInDataBuf))[1];
	CSmartPtr<CMultiMappedChain> MMC =
		new CMultiMappedChain(m_PMM, MemSize, PhysAddr);
	m_MMChains.PushBack(MMC);
	dword SMID = dword(MMC->GetID());
	(PD(m_KeCallOutDataBuf))[0] = SMID;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetPreloadedDriversCount()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 4) return;

	(PD(m_KeCallOutDataBuf))[0] = m_PreloadedDriversCount;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetTime()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 4) return;

	(PD(m_KeCallOutDataBuf))[0] = m_TickCount * 50; // * 50ms
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetNextProcessInfo()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 12 + 1024 + 128) return;

	dword PrevPID = (PD(m_KeCallInDataBuf))[0];
	dword NextPID = 0xFFFFFFFF;

	CThread* NextT = null;
	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
	{
		CThread* T = m_TL.GetCurrentElement(); 

		dword TID = T->GetID();
		if (TID > PrevPID)
			if (TID < NextPID)
			{
				NextPID = TID;
				NextT = T;
			}
	}

	(PD(m_KeCallOutDataBuf))[0] = NextPID;
	if (NextT != null)
	{
		(PD(m_KeCallOutDataBuf))[1] = NextT->GetUsedPageCount();
		(PD(m_KeCallOutDataBuf))[2] = NextT->GetNotificationCount();
		NextT->GetPerfData((dword*)(m_KeCallOutDataBuf + 12), (dword*)(m_KeCallOutDataBuf + 12 + 512));
		NextT->GetName().CopyTo((char*)(m_KeCallOutDataBuf + 12 + 1024));
	}
	else
	{
		m_KeCallRealOutDataSize = 4;
	}
}

// ----------------------------------------------------------------------------
void CKernel::OnKeGetBootType()
{
	if (m_KeCallInDataSize != 0) return;
	if (m_KeCallOutDataSize != 4) return;

	(PD(m_KeCallOutDataBuf))[0] = m_BootType;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeResetSymbol()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 1) return;

	dword Symbol = (PD(m_KeCallInDataBuf))[0];
	byte Success = ResetSymbol(Symbol) ? 1 : 0;

	m_KeCallOutDataBuf[0] = Success;
}

// ----------------------------------------------------------------------------
void CKernel::OnKeEndOfInterrupt()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword IRQ = ((dword*)m_KeCallInDataBuf)[0];
	m_IM.EndOfInterrupt(IRQ);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeSetGeneralProtectionExceptionHandler()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword Address = ((dword*)m_KeCallInDataBuf)[0];
	m_ActiveThread->SetGPEHandler(Address);
}

// ----------------------------------------------------------------------------
void CKernel::OnKeUnmapSharedMem()
{
	if (m_KeCallInDataSize != 4) return;
	if (m_KeCallOutDataSize != 0) return;

	dword PID = m_ActiveThread->GetID();
	dword SMID = (PD(m_KeCallInDataBuf))[0];
	for (dword i = 0; i < m_MMChains.Size(); i++)
	{
		if (m_MMChains[i]->GetID() == SMID)
		{
			m_MMChains[i]->UnmapProcess(PID);
			break;
		}
	}
}

// ----------------------------------------------------------------------------
void CKernel::OnKeCall(dword FunctionIndex)
{
	m_KeCallRealOutDataSize = m_KeCallOutDataSize;
	switch (FunctionIndex)
	{
	case 1: OnKeUnmaskIRQ(); break;
	case 2: OnKeExitProcess(); break;
	case 3: OnKeRequestReboot(); break;
	case 4: OnKeEnableNotification(); break;
	case 5: OnKeWaitFor(); break;
	case 6: OnKeGetNotification(); break;
	case 7: OnKeNotify(); break;
	case 8: OnKeEnableCallRequest(); break;
	case 9: OnKeRequestCall(); break;
	case 10: OnKeRespondCall(); break;
	case 11: OnKeGetCallRequest(); break;
	case 12: OnKeOutPortByte(); break;
	case 13: OnKeInPortByte(); break;
	case 14: OnKeOutPortWord(); break;
	case 15: OnKeInPortWord(); break;
	case 16: OnKeWaitTicks(); break;
	case 17: OnKeIsSymbolSet(); break;
	case 18: OnKeSetSymbol(); break;
	case 19: OnKeWaitForSymbol(); break;
	case 20: OnKeInPortWordArray(); break;
	case 21: OnKeCreateProcess(); break;
	case 22: OnKeGetNotificationCount(); break;
	case 23: OnKeGetCallRequestCount(); break;
	case 24: OnKeGetNfClCount(); break;
	case 27: OnKeDisableNotification(); break;
	case 29: OnKeMemAlloc(); break;
	case 30: OnKeMemFree(); break;
	case 31: OnKeVirtToPhys(); break;
	case 34: OnKeAllocSharedMem(); break;
	case 35: OnKeMapSharedMem(); break;
	case 36: OnKeReleaseSharedMem(); break;
	case 37: OnKeOutPortDword(); break;
	case 38: OnKeInPortDword(); break;
	case 39: OnKeGetMemInfo(); break;
	case 40: OnKeGetSharedMemSize(); break;
	case 41: OnKeAllocSharedMemAt(); break;
	case 42: OnKeGetPreloadedDriversCount(); break;
	case 43: OnKeGetTime(); break;
	case 44: OnKeGetNextProcessInfo(); break;
	case 45: OnKeGetBootType(); break;
	case 46: OnKeResetSymbol(); break;
	case 47: OnKeEndOfInterrupt(); break;
	case 48: OnKeSetGeneralProtectionExceptionHandler(); break;
	case 49: OnKeUnmapSharedMem(); break;
	}
}

// ----------------------------------------------------------------------------
bool CKernel::SetSymbol(dword Symbol)
{
	if (!HasSymbol(Symbol))
	{
		m_Symbols.PushBack(Symbol);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------
bool CKernel::ResetSymbol(dword Symbol)
{
	for (dword i = 0; i < m_Symbols.Size(); i++)
	{
		if (m_Symbols[i] == Symbol)
		{
			m_Symbols.Delete(i);
			return true;
		}
	}
	return false;
}

// ----------------------------------------------------------------------------
bool CKernel::HasSymbol(dword Symbol)
{
	for (dword i = 0; i < m_Symbols.Size(); i++)
		if (m_Symbols[i] == Symbol)
			return true;
	return false;
}

// ----------------------------------------------------------------------------
void CKernel::ProcessHWIntRequest(dword Index)
{
	if (Index == -1) return;

	if ((Index >= 0) && (Index <= 0x1F))
	{
		CThread* T = m_ActiveThread;
		if (Index == 7)
		{
			T->SetFPUStateChange();
		}
		else
		{
			bool fail = true;
			if ((Index == 0xD) && (T->GetGPEHandler() != 0))
				fail = false;
			if (fail)
			{
				(PD(m_TempBuf))[0] = T->GetID();
				(PD(m_TempBuf))[1] = Index;
				(PD(m_TempBuf))[2] = *PD(&T->GetRing0Stack()[0xFEC]);

				const CFString<128>& Name = T->GetName();
				Name.CopyTo((char*)(m_TempBuf + 12));

				BroadcastNotification(0, NfKe_ExceptionInfo,
					12 + Name.Len(), CUniPtr(m_TempBuf));
				RemoveActiveThread();
			}
			else
			{
				T->ResetStack();
				T->GetTask().GetTSS().GetTaskState().EIP = T->GetGPEHandler();
				T->SetGPEHandler(0);
			}
		}
	}
	else if (Index == 0x20)
	{
		m_TickCount++;
		for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
			m_TL.GetCurrentElement()->OnTick(m_TickCount);

		if (m_IsRebootActivated)
			ProcessReboot();

		BroadcastNotification(0, NfKe_IRQ0);
		m_IM.EndOfInterrupt(0);
	}
	else if (Index == 0x21)
		BroadcastNotification(0, NfKe_IRQ1);
	else if (Index == 0x24)
		BroadcastNotification(0, NfKe_IRQ4);
	else if (Index == 0x26)
		BroadcastNotification(0, NfKe_IRQ6);
	else if (Index == 0x29)
		BroadcastNotification(0, NfKe_IRQ9);
	else if (Index == 0x2C)
		BroadcastNotification(0, NfKe_IRQ12);
}

// ----------------------------------------------------------------------------
void CKernel::ProcessInt30Request()
{
	CTSS& TSS = m_ActiveThread->GetTask().GetTSS();
	CTaskState& TS = TSS.GetTaskState();

	dword FuncIndex = TS.EAX;
	m_KeCallInDataSize = TS.EBX;
	m_KeCallOutDataSize = TS.ECX;
	byte* InDataVPtr = PB(TS.ESI);
	m_KeCallOutDataVPtr = PB(TS.EDI);

	if (m_KeCallInDataSize > 32768) return;
	if (m_KeCallOutDataSize > 32768) return;

	CVirtMemManager& VMM = m_ActiveThread->GetVMMRef();
	if (InDataVPtr != 0)
		VMM.MemCopyVirtToPhys(InDataVPtr, m_KeCallInDataBuf, m_KeCallInDataSize);

	OnKeCall(FuncIndex);

	if (m_KeCallOutDataVPtr != 0)
		VMM.MemCopyPhysToVirt(m_KeCallOutDataBuf, m_KeCallOutDataVPtr, m_KeCallRealOutDataSize);
}

// ----------------------------------------------------------------------------
void CKernel::ActivateThread()
{
	bool IsThreadFound = false;
	CThread* FoundThread = null;
	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
	{
		CThread* T = m_TL.GetCurrentElement();
		if (!T->IsWaiting())
		{
			bool ToReplace = true;
			if (IsThreadFound)
				if (FoundThread->GetPriority() < T->GetPriority())
					ToReplace = false;
			if (ToReplace)
			{
				IsThreadFound = true;
				FoundThread = T;
			}
		}
	}

	ErrIf(!IsThreadFound); // Shit happens

	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
	{
		if (m_TL.GetCurrentElement() == FoundThread)
		{
			m_ActiveThread = FoundThread;
			m_TL.MoveCurrentElementToEnd();
			return;
		}
	}
}

// ----------------------------------------------------------------------------
void CKernel::Loop()
{
	m_IM.UnmaskIRQ(0);
	m_IM.UnmaskIRQ(2);
	qword TS1, TS2, TS3, TS4;
	for (;;)
	{
		ActivateThread();

		m_ActiveThread->LoadFPUState();
		TS1 = __rdtsc();
		m_ActiveThread->SwitchTo();
		TS2 = __rdtsc();
		m_ActiveThread->StoreFPUState();
		if (TS2 > TS1)
			m_ActiveThread->AddUserPerfData((dword)(TS2 - TS1));

		dword ReqIndex = m_IM.GetRequestIndex(
			m_ActiveThread->GetTask().GetTSS().GetTaskState().EIP);
		if (ReqIndex == 0x30)
		{
			TS3 = __rdtsc();
			ProcessInt30Request();
			TS4 = __rdtsc();

			if (m_ActiveThread)
				if (TS4 > TS3)
					m_ActiveThread->AddKernelPerfData((dword)(TS4 - TS3));
		}
		else
			ProcessHWIntRequest(ReqIndex);
	}
}

// ----------------------------------------------------------------------------
bool CKernel::BroadcastNotification(dword SrcPID, dword NfID, dword NfSize, CUniPtr& NfData)
{
	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
		if (!m_TL.GetCurrentElement()->AddNotification(SrcPID, NfID, NfSize, NfData))
			return false;
	return true;
}

// ----------------------------------------------------------------------------
bool CKernel::BroadcastNotification(dword SrcPID, dword NfID)
{
	return BroadcastNotification(SrcPID, NfID, 0, CUniPtr(0));
}

// ----------------------------------------------------------------------------
void CKernel::StartReboot()
{
	if (!m_IsRebootActivated)
	{
		m_IsRebootActivated = true;
		m_RebootLevelStartTick = m_TickCount;
		m_RebootLevel = 2;
		NotifyTermination();
	}
}

// ----------------------------------------------------------------------------
void CKernel::ProcessReboot()
{
	dword TimeDelta = m_TickCount - m_RebootLevelStartTick;

	bool IsTerminated = true;
	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
	{
		CThread* T = m_TL.GetCurrentElement();
		if (T->GetID() != 1)
			if (T->GetAccessLevel() >= m_RebootLevel)
			{
				IsTerminated = false;
				break;
			}
	}

	if (IsTerminated || (TimeDelta > 80))
	{
		if (m_RebootLevel == 0)
		{
			Reboot();
		}
		else
		{
			if (!IsTerminated)
			{
				CArray<dword> TermIDs;
				m_TL.ToFirstElement();
				for (;;)
				{
					if (m_TL.IsAtLastDummy())
						break;

					CThread* T = m_TL.GetCurrentElement();
					if (T->GetID() != 1)
						if (T->GetAccessLevel() >= m_RebootLevel)
						{
							TermIDs.PushBack(T->GetID());
							if (T == m_ActiveThread)
								m_ActiveThread = null;
							m_TL.RemoveCurrentElement();
							continue;
						}
					m_TL.ToNextElement();
				}
				for (dword i = 0; i < TermIDs.Size(); i++)
				{
					dword TID = TermIDs[i];
					BroadcastNotification(0, NfKe_ProcessExited, 4, CUniPtr(PB(&TID)));
				}
			}

			m_RebootLevelStartTick = m_TickCount;
			m_RebootLevel--;
			NotifyTermination();
		}
	}
}

// ----------------------------------------------------------------------------
void CKernel::NotifyTermination()
{
	for (m_TL.ToFirstElement(); !m_TL.IsAtLastDummy(); m_TL.ToNextElement())
	{
		CThread* T = m_TL.GetCurrentElement();
		if (T->GetID() != 1)
			if (T->GetAccessLevel() == m_RebootLevel)
				T->AddNotification(0, NfKe_TerminateProcess, 0, CUniPtr(0));
	}
}

// ----------------------------------------------------------------------------
void CKernel::Reboot()
{
	// Set CMOS
	_outp(0x70, 0x8F); // 0xF "Reset Code", 0x80 "NMI Disabled"
	_outp(0x71, 0x00); // "Software reset or unexpected reset"

	// Write Command Byte
	while (_inp(0x64) & 2);
	_outp(0x64, 0x60);

	// Enable Scan Code Translation
	// Disable Mouse Interface
	// Enable Keyboard Interface
	// Set System Flag (!)
	// Enable Interrupts
	while (_inp(0x64) & 2);
	_outp(0x60, 0x67);

	// Keyb Reboot
	while (_inp(0x64) & 2);
	_outp(0x64, 0xFE);

	for (;;);
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=