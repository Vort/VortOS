// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Am79C970.cpp
#include "API.h"

// ----------------------------------------------------------------------------
extern "C" void* memcpy(void* destination, const void* source, size_t num);
extern "C" void* memset(void* ptr, int value, size_t num);

// ----------------------------------------------------------------------------
#pragma pack(push, 1)
struct InitBlock
{
	word Mode;
	byte Res1:4;
	byte RLen:4;
	byte Res2:4;
	byte TLen:4;
	byte PAdr[6];
	word Res3;
	byte LAdr[8];
	dword RDRA;
	dword TDRA;
};
struct ReceiveDescriptor
{
	word MCNT:12;
	word Zeros:4;
	byte RPC;
	byte RCC;

	dword BCNT:12;
	dword Ones:4;
	dword Res1:4;
	dword BAM:1;
	dword LAFM:1;
	dword PAM:1;
	dword BPE:1;
	dword ENP:1;
	dword STP:1;
	dword BUFF:1;
	dword CRC:1;
	dword OFLO:1;
	dword FRAM:1;
	dword ERR:1;
	dword OWN:1;

	dword RBADR;

	dword Res2;
};
struct TransmitDescriptor
{
	dword TRC:4;
	dword Res1:8;
	dword TDR:14;
	dword RTRY:1;
	dword LCAR:1;
	dword LCOL:1;
	dword EXDEF:1;
	dword UFLO:1;
	dword BUFF:1;

	dword BCNT:12;
	dword Ones:4;
	dword Res2:7;
	dword BPE:1;
	dword ENP:1;
	dword STP:1;
	dword DEF:1;
	dword ONE:1;
	dword MORE_LTINT:1;
	dword ADD_NOFCS:1;
	dword ERR:1;
	dword OWN:1;

	dword TBADR;

	dword Res3;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------
class Am79C970
{
private:
	byte bus;
	byte device;
	byte function;
	byte irq;

	bool initDone;
	dword baseAddress;
	byte macAddr[6];
	InitBlock* initBlock;
	ReceiveDescriptor* receiveRing;
	TransmitDescriptor* transmitRing;
	byte* recvBuffers[16];
	byte* sendBuffers[4];

	dword nextReceiveIndex;
	dword nextTransmitIndex;

public:
	Am79C970()
	{
		if (!Detect())
			return;

		initDone = false;
		nextReceiveIndex = 0;
		nextTransmitIndex = 0;

		KeEnableNotification(NfNetwork_SendPacket);
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableCallRequest(ClNetwork_GetSelfMACAddress);

		// Get I/O base address
		baseAddress = ReadPCIConfDword(bus, device, function, 0x10) & ~0x3;

		// Link IRQ
		irq = ReadPCIConfByte(bus, device, function, 0x3C);
		KeLinkIrq(irq);

		// Enable bus mastering
		word cmdReg = ReadPCIConfWord(bus, device, function, 0x04);
		WritePCIConfWord(bus, device, function, 0x04, cmdReg | (1 << 2));

		// Read MAC address from APROM
		for (int i = 0; i < 6; i++)
			macAddr[i] = KeInPortByte(baseAddress + i);

		// Switch to 32-bit I/O mode
		KeOutPortDword(baseAddress + 0x10, 0);

		// Wait until DWIO = 1
		while ((ReadBCR(18) & (1 << 7)) == 0);

		// SWSTYLE = 3
		WriteBCR(20, (ReadBCR(20) & ~0xFF) | 0x03);

		// BREADE = 1
		WriteBCR(18, ReadBCR(18) | (1 << 6));

		for (int i = 0; i < 8; i++)
		{
			recvBuffers[i * 2 + 0] = new byte[0x1000];
			recvBuffers[i * 2 + 1] = recvBuffers[i * 2 + 0] + 0x800;
		}
		for (int i = 0; i < 2; i++)
		{
			sendBuffers[i * 2 + 0] = new byte[0x1000];
			sendBuffers[i * 2 + 1] = sendBuffers[i * 2 + 0] + 0x800;
		}

		receiveRing = (ReceiveDescriptor*)new byte[0x1000];
		transmitRing = (TransmitDescriptor*)new byte[0x1000];
		memset(receiveRing, 0x00, sizeof(ReceiveDescriptor) * 16);
		memset(transmitRing, 0x00, sizeof(TransmitDescriptor) * 4);

		for (int i = 0; i < 16; i++)
		{
			receiveRing[i].BCNT = -2048;
			receiveRing[i].Ones = ~0;
			receiveRing[i].RBADR = KeVirtToPhys(recvBuffers[i]);
			receiveRing[i].OWN = 1;
		}
		for (int i = 0; i < 4; i++)
		{
			transmitRing[i].Ones = ~0;
			transmitRing[i].TBADR = KeVirtToPhys(sendBuffers[i]);
		}

		initBlock = (InitBlock*)new byte[0x1000];
		memset(initBlock, 0x00, sizeof(InitBlock));

		dword initBlockPA = KeVirtToPhys((byte*)initBlock);
		WriteCSR(1, initBlockPA & 0xFFFF);
		WriteCSR(2, initBlockPA >> 16);

		initBlock->Mode = 0x0000; // CSR15
		initBlock->RLen = 4; // 16 receive descriptors
		initBlock->TLen = 2; // 4 transmit descriptors
		memcpy(initBlock->PAdr, macAddr, 6);
		initBlock->RDRA = KeVirtToPhys((byte*)receiveRing);
		initBlock->TDRA = KeVirtToPhys((byte*)transmitRing);
		
		WriteCSR(0, 0x0043); // INIT = 1, STRT = 1, STOP = 0, IENA = 1

		KeWaitForSymbol(SmNetwork_Waiting);

		CCallRequest<4> CR;
		CNotification<2048> N;
		for (;;)
		{
			KeWaitFor(3);
			dword NfCount;
			dword CallCount;
			KeGetNfClCount(NfCount, CallCount);

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfKe_Irq)
				{
					ProcessIrq();
				}
				else if (N.GetID() == NfNetwork_SendPacket)
				{
					if (N.GetSize() > 2048)
						continue;

					if (transmitRing[nextTransmitIndex].OWN == 0)
					{
						transmitRing[nextTransmitIndex].BCNT = -N.GetSize();
						memcpy(sendBuffers[nextTransmitIndex], N.GetBuf(), N.GetSize());
						transmitRing[nextTransmitIndex].STP = 1;
						transmitRing[nextTransmitIndex].ENP = 1;
						transmitRing[nextTransmitIndex].OWN = 1;
						nextTransmitIndex = (nextTransmitIndex + 1) % 4;
					}
					else
					{
						DebugOut("[tskip]");
					}
				}
				else if (N.GetID() == NfKe_TerminateProcess)
				{
					// STOP = 1
					WriteCSR(0, 1 << 2);
					return;
				}
			}
			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClNetwork_GetSelfMACAddress)
				{
					CR.Respond(macAddr, 6);
				}
			}
		}
	}

	void ProcessIrq()
	{
		if (!initDone)
		{
			if ((ReadCSR(0) & (1 << 8)) != 0)
			{
				initDone = true;
				WriteCSR(0, (ReadCSR(0) & 0x80F7) | (1 << 8));
				DebugOut("[idone]");
				KeSetSymbol(SmNetwork_Ready);
			}
		}

		if (initDone)
		{
			// MISS
			if ((ReadCSR(0) & (1 << 12)) != 0)
			{
				DebugOut("[rcvmiss]");
				WriteCSR(0, (ReadCSR(0) & 0x80F7) | (1 << 12));
			}

			// RINT
			if ((ReadCSR(0) & (1 << 10)) != 0)
			{
				for (;;)
				{
					if (receiveRing[nextReceiveIndex].OWN == 0)
					{
						KeNotify(NfNetwork_RecvdPacket,
							recvBuffers[nextReceiveIndex],
							receiveRing[nextReceiveIndex].MCNT);
						receiveRing[nextReceiveIndex].OWN = 1;
						nextReceiveIndex = (nextReceiveIndex + 1) % 16;
					}
					else
					{
						break;
					}
				}
				WriteCSR(0, (ReadCSR(0) & 0x80F7) | (1 << 10));
			}

			// TINT
			if ((ReadCSR(0) & (1 << 9)) != 0)
			{
				WriteCSR(0, (ReadCSR(0) & 0x80F7) | (1 << 9));
			}
		}
		
		KeEndOfInterrupt(irq);
	}

	dword ReadCSR(byte index)
	{
		KeOutPortDword(baseAddress + 0x14, index); // RAP
		return KeInPortDword(baseAddress + 0x10); // RDP
	}

	void WriteCSR(word index, dword value)
	{
		KeOutPortDword(baseAddress + 0x14, index); // RAP
		KeOutPortDword(baseAddress + 0x10, value); // RDP
	}

	dword ReadBCR(byte index)
	{
		KeOutPortDword(baseAddress + 0x14, index); // RAP
		return KeInPortDword(baseAddress + 0x1C); // BDP
	}

	void WriteBCR(byte index, dword value)
	{
		KeOutPortDword(baseAddress + 0x14, index); // RAP
		KeOutPortDword(baseAddress + 0x1C, value); // BDP
	}

	bool Detect()
	{
		KeWaitForSymbol(SmPCI_Ready);
		if (!GetPCIDeviceByID(0x1022, 0x2000, bus, device, function))
			return false;
		return true;
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_AM79C970))
		return;

	Am79C970 am79C970;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=