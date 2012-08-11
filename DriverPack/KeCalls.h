// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// KeCalls.h
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
void GenericKeCall(dword FuncIndex, dword InDataSize, dword OutDataSize,
				   byte* InDataPtr, byte* OutDataPtr)
{
	__asm
	{
		mov eax, FuncIndex
		mov ebx, InDataSize
		mov ecx, OutDataSize
		mov esi, InDataPtr
		mov edi, OutDataPtr
		int 0x30
	}
}

// ----------------------------------------------------------------------------
void KeUnmaskIRQ(byte IRQ)
{
	GenericKeCall(1, 1, 0, &IRQ, 0);
}

// ----------------------------------------------------------------------------
void KeExitProcess()
{
	GenericKeCall(2, 0, 0, 0, 0);
}

// ----------------------------------------------------------------------------
void KeRequestReboot()
{
	GenericKeCall(3, 0, 0, 0, 0);
}

// ----------------------------------------------------------------------------
void KeEnableNotification(dword NotificationID)
{
	GenericKeCall(4, 4, 0, PB(&NotificationID), 0);
}

// ----------------------------------------------------------------------------
void KeWaitFor(byte WaitType)
{
	GenericKeCall(5, 1, 0, &WaitType, 0);
}

// ----------------------------------------------------------------------------
bool KeGetNotification(byte* Buffer, dword BufferSize, dword& NotificationID,
					   dword& NotificationSize, dword& SrcPID)
{
	dword In[2];
	In[0] = BufferSize;
	In[1] = dword(Buffer);
	byte Out[13];

	GenericKeCall(6, 8, 13, PB(In), Out);
	if (Out[0] == 0)
	{
		NotificationSize = *PD(&Out[1]);
		NotificationID = *PD(&Out[5]);
		SrcPID = *PD(&Out[9]);
		return true;
	}
	else
		return false;
}

// ----------------------------------------------------------------------------
bool KeNotify(dword NotificationID, byte* NotificationBuffer,
			  dword NotificationBufferSize)
{
	byte Out;
	dword In[3];
	In[0] = NotificationBufferSize;
	In[1] = dword(NotificationBuffer);
	In[2] = NotificationID;
	GenericKeCall(7, 12, 1, PB(In), &Out);
	return Out == 0;
}

// ----------------------------------------------------------------------------
void KeEnableCallRequest(dword CallTypeID)
{
	GenericKeCall(8, 4, 0, PB(&CallTypeID), 0);
}

// ----------------------------------------------------------------------------
bool KeRequestCall(dword CallTypeID, byte* InBuffer, dword InBufferSize,
				   byte* OutBuffer, dword OutBufferSize)
{
	dword In[5];
	In[0] = InBufferSize;
	In[1] = dword(InBuffer);
	In[2] = OutBufferSize;
	In[3] = dword(OutBuffer);
	In[4] = CallTypeID;

	byte Out[5];
	GenericKeCall(9, 20, 5, PB(In), Out);
	return Out[0] == 0;
}

// ----------------------------------------------------------------------------
bool KeRequestCall(dword CallTypeID, byte* InBuffer, dword InBufferSize,
				   byte* OutBuffer, dword OutBufferSize, dword& ResponseSize)
{
	dword In[5];
	In[0] = InBufferSize;
	In[1] = dword(InBuffer);
	In[2] = OutBufferSize;
	In[3] = dword(OutBuffer);
	In[4] = CallTypeID;

	byte Out[5];
	GenericKeCall(9, 20, 5, PB(In), Out);
	if (Out[0] != 0)
		return false;
	ResponseSize = *PD(Out + 1);
	return true;
}

// ----------------------------------------------------------------------------
bool KeGetCallRequest(byte* Buffer, dword BufferSize,
					  dword& CallRequestTypeID, dword& CallRequestSize, dword& SrcPID)
{
	dword In[2];
	In[0] = BufferSize;
	In[1] = dword(Buffer);

	byte Out[1+12];
	GenericKeCall(11, 8, 13, PB(In), Out);

	byte Result = Out[0];
	if (Result == 0)
	{
		CallRequestSize = *PD(&Out[1]);
		CallRequestTypeID = *PD(&Out[5]);
		SrcPID = *PD(&Out[9]);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------
void KeRespondCall2(dword CltPID, byte* CallResponseBuffer,
				   dword CallResponseSize)
{
	dword In[3];
	In[0] = CallResponseSize;
	In[1] = dword(CallResponseBuffer);
	In[2] = CltPID;

	GenericKeCall(10, 12, 0, PB(In), 0);
}

// ----------------------------------------------------------------------------
void KeOutPortByte(word Port, byte Byte)
{
	byte Buf[3];
	*PW(&Buf[0]) = Port;
	Buf[2] = Byte;
	GenericKeCall(12, 3, 0, Buf, null);
}

// ----------------------------------------------------------------------------
byte KeInPortByte(word Port)
{
	byte Byte;
	GenericKeCall(13, 2, 1, PB(&Port), &Byte);
	return Byte;
}

// ----------------------------------------------------------------------------
void KeOutPortWord(word Port, word Word)
{
	byte Buf[4];
	*PW(&Buf[0]) = Port;
	*PW(&Buf[2]) = Word;
	GenericKeCall(14, 4, 0, Buf, null);
}

// ----------------------------------------------------------------------------
word KeInPortWord(word Port)
{
	word Word;
	GenericKeCall(15, 2, 2, PB(&Port), PB(&Word));
	return Word;
}

// ----------------------------------------------------------------------------
void KeWaitTicks(dword TickCountMinusOne)
{
	GenericKeCall(16, 4, 0, PB(&TickCountMinusOne), 0);
}

// ----------------------------------------------------------------------------
bool KeIsSymbolSet(dword Symbol)
{
	byte Success;
	GenericKeCall(17, 4, 1, PB(&Symbol), &Success);
	return Success;
}

// ----------------------------------------------------------------------------
bool KeSetSymbol(dword Symbol)
{
	byte Success;
	GenericKeCall(18, 4, 1, PB(&Symbol), &Success);
	return Success;
}

// ----------------------------------------------------------------------------
void KeWaitForSymbol(dword Symbol)
{
	GenericKeCall(19, 4, 0, PB(&Symbol), 0);
}

// ----------------------------------------------------------------------------
dword KeWaitForSymbol(dword symbol1, dword symbol2)
{
	dword in[2];
	in[0] = symbol1;
	in[1] = symbol2;
	dword triggeredSymbol;
	GenericKeCall(19, 8, 4, (byte*)in, (byte*)&triggeredSymbol);
	return triggeredSymbol;
}

// ----------------------------------------------------------------------------
void KeInPortWordArray(word Port, dword WordCount, word* Buf)
{
	dword ReqBuf[2];
	ReqBuf[0] = Port;
	ReqBuf[1] = WordCount;
	GenericKeCall(20, 8, WordCount * 2, PB(&ReqBuf), PB(Buf));
}

// ----------------------------------------------------------------------------
bool KeCreateProcess(byte* Image, dword Size, const char* Name)
{
	byte Result = 0xFF;
	dword ReqBuf[3];
	ReqBuf[0] = Size;
	ReqBuf[1] = dword(Image);
	ReqBuf[2] = dword(Name);
	GenericKeCall(21, 12, 1, PB(ReqBuf), &Result);
	return Result == 0;
}

// ----------------------------------------------------------------------------
dword KeGetNotificationCount()
{
	dword NotificationCount;
	GenericKeCall(22, 0, 4, 0, PB(&NotificationCount));
	return NotificationCount;
}

// ----------------------------------------------------------------------------
dword KeGetCallRequestCount()
{
	dword CallRequestCount;
	GenericKeCall(23, 0, 4, 0, PB(&CallRequestCount));
	return CallRequestCount;
}

// ----------------------------------------------------------------------------
void KeGetNfClCount(dword& NfCount, dword& ClCount)
{
	dword InBuf[2];
	GenericKeCall(24, 0, 8, 0, PB(InBuf));
	NfCount = InBuf[0];
	ClCount = InBuf[1];
}

// ----------------------------------------------------------------------------
void KeDisableNotification(dword NotificationID)
{
	GenericKeCall(27, 4, 0, PB(&NotificationID), 0);
}

// ----------------------------------------------------------------------------
dword KeVirtToPhys(byte* Virt)
{
	dword Phys;
	GenericKeCall(31, 4, 4, PB(&Virt), PB(&Phys));
	return Phys;
}

// ----------------------------------------------------------------------------
dword KeAllocSharedMem(dword MemSize)
{
	dword SMID;
	GenericKeCall(34, 4, 4, PB(&MemSize), PB(&SMID));
	return SMID;
}

// ----------------------------------------------------------------------------
byte* KeMapSharedMem(dword SMID)
{
	byte* VirtualBase;
	GenericKeCall(35, 4, 4, PB(&SMID), PB(&VirtualBase));
	return PB(VirtualBase);
}

// ----------------------------------------------------------------------------
void KeReleaseSharedMem(dword SMID)
{
	GenericKeCall(36, 4, 0, PB(&SMID), 0);
}

// ----------------------------------------------------------------------------
void KeOutPortDword(word Port, dword Dword)
{
	byte Buf[6];
	*PW(&Buf[0]) = Port;
	*PD(&Buf[2]) = Dword;
	GenericKeCall(37, 6, 0, Buf, null);
}

// ----------------------------------------------------------------------------
dword KeInPortDword(word Port)
{
	dword Dword;
	GenericKeCall(38, 2, 4, PB(&Port), PB(&Dword));
	return Dword;
}

// ----------------------------------------------------------------------------
void KeGetMemInfo(dword& HeapUsed, dword& HeapTotal, dword& PageUsed, dword& PageTotal)
{
	dword Buf[4];
	GenericKeCall(39, 0, 16, null, PB(Buf));
	HeapUsed = Buf[0];
	HeapTotal = Buf[1];
	PageUsed = Buf[2];
	PageTotal = Buf[3];
}

// ----------------------------------------------------------------------------
dword KeGetSharedMemSize(dword SMID)
{
	dword SMSize;
	GenericKeCall(40, 4, 4, PB(&SMID), PB(&SMSize));
	return SMSize;
}

// ----------------------------------------------------------------------------
dword KeAllocSharedMemAt(dword MemSize, dword PhysAddr)
{
	dword SMID;
	dword Buf[2];
	Buf[0] = PhysAddr;
	Buf[1] = MemSize;
	GenericKeCall(41, 8, 4, PB(Buf), PB(&SMID));
	return SMID;
}

// ----------------------------------------------------------------------------
dword KeGetPreloadedDriversCount()
{
	dword PreloadedDriversCount;
	GenericKeCall(42, 0, 4, 0, PB(&PreloadedDriversCount));
	return PreloadedDriversCount;
}

// ----------------------------------------------------------------------------
dword KeGetTime()
{
	dword Time;
	GenericKeCall(43, 0, 4, 0, PB(&Time));
	return Time;
}

// ----------------------------------------------------------------------------
dword KeGetNextProcessInfo(dword PrevPID, dword& UsedPageCount,
	dword& NotificationCount, dword* UserPerfData, dword* KernelPerfData, char* Name)
{
	#pragma pack(1)
	struct
	{
		dword NextPID;
		dword UsedPageCount;
		dword NotificationCount;
		dword UserPerfData[128];
		dword KernelPerfData[128];
		char Name[128];
	} Data;
	#pragma pack()

	GenericKeCall(44, 4, sizeof(Data), (byte*)(&PrevPID), (byte*)(&Data));
	
	if (Data.NextPID == 0xFFFFFFFF)
		return Data.NextPID;

	UsedPageCount = Data.UsedPageCount;
	NotificationCount = Data.NotificationCount;

	for (dword i = 0; i < 128; i++)
	{
		UserPerfData[i] = Data.UserPerfData[i];
		KernelPerfData[i] = Data.KernelPerfData[i];
		Name[i] = Data.Name[i];
	}

	return Data.NextPID;
}

// ----------------------------------------------------------------------------
dword KeGetBootType()
{
	dword BootType;
	GenericKeCall(45, 0, 4, 0, PB(&BootType));
	return BootType;
}

// ----------------------------------------------------------------------------
bool KeResetSymbol(dword Symbol)
{
	byte Success;
	GenericKeCall(46, 4, 1, PB(&Symbol), &Success);
	return Success;
}

// ----------------------------------------------------------------------------
void KeEndOfInterrupt(dword IRQ)
{
	GenericKeCall(47, 4, 0, PB(&IRQ), 0);
}

// ----------------------------------------------------------------------------
void KeSetGeneralProtectionExceptionHandler(void* Address)
{
	GenericKeCall(48, 4, 0, PB(&Address), 0);
}

// ----------------------------------------------------------------------------
void KeUnmapSharedMem(dword SMID)
{
	GenericKeCall(49, 4, 0, PB(&SMID), 0);
}
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=