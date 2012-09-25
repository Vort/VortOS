// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Rtl8139.cpp
#include "API.h"

// ----------------------------------------------------------------------------
extern "C" void* memcpy(void* destination, const void* source, size_t num);
extern "C" void* memset(void* ptr, int value, size_t num);

// ----------------------------------------------------------------------------
#pragma pack(push, 1)
struct TransmitDescriptor
{
	dword SIZE:13;
	dword OWN:1;
	dword TUN:1;
	dword TOK:1;
	dword ERTXTH:6;
	dword Reserved:2;
	dword NCC:4;
	dword CDH:1;
	dword OWC:1;
	dword TABT:1;
	dword CRS:1;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------
class Rtl8139
{
private:
	byte bus;
	byte device;
	byte function;
	byte irq;

	byte hwVersionId;
	dword baseAddress;
	byte macAddr[6];

	byte* recvBuffer;
	byte* sendBuffers[4];

	dword nextTransmitIndex;
	dword nextRecvOffset;

public:
	Rtl8139()
	{
		if (!Detect())
			return;

		nextRecvOffset = 0;
		nextTransmitIndex = 0;

		KeEnableNotification(NfNetwork_SendPacket);
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableCallRequest(ClNetwork_GetSelfMACAddress);

		// Get I/O base address
		baseAddress = ReadPCIConfDword(bus, device, function, 0x10) & ~0x3;

		// Link IRQ
		irq = ReadPCIConfByte(bus, device, function, 0x3C);
		KeLinkIrq(irq);

		// Reset controller
		WriteRegisterByte(0x37, 1 << 4);

		// Waiting for reset completion
		while ((ReadRegisterByte(0x37) & (1 << 4)) != 0);

		// Get adapter version
		dword reg40 = ReadRegisterDword(0x40);
		hwVersionId = ((reg40 >> 22) & 3) | (((reg40 >> 26) & 0x1F) << 2);

		if ((hwVersionId != 0x76) && // RTL8139C+
			(hwVersionId != 0x75))   // RTL8139D
		{
			// Not supported
			return;
		}

		// Read MAC address
		for (int i = 0; i < 6; i++)
			macAddr[i] = ReadRegisterByte(i);

		// Init send buffers
		for (int i = 0; i < 2; i++)
		{
			sendBuffers[i * 2 + 0] = new byte[0x1000];
			sendBuffers[i * 2 + 1] = sendBuffers[i * 2 + 0] + 0x800;
		}

		for (int i = 0; i < 4; i++)
		{
			// Init transmit status registers
			TransmitDescriptor td;
			*(dword*)(&td) = 0x00000000;
			td.OWN = 1;
			WriteRegisterDword(0x10 + i * 4, *(dword*)(&td));

			// Init transmit start address registers
			WriteRegisterDword(0x20 + i * 4, KeVirtToPhys(sendBuffers[i]));
		}

		// Enable transmitter and receiver
		WriteRegisterByte(0x37, (1 << 2) | (1 << 3));

		// Set receive buffer address
		recvBuffer = KeAllocLinearBlock(3);
		WriteRegisterDword(0x30, KeVirtToPhys(recvBuffer));

		// Receive configuration register:
		// Wrap = 1
		// Accept broadcast
		// Accept physical match
		WriteRegisterDword(0x44, (1 << 7) | (1 << 3) | (1 << 1));

		// Unmask transmit ok interrupt, receive ok interrupt
		WriteRegisterWord(0x3C, (1 << 2) | (1 << 0));

		// Clear missed packet counter
		WriteRegisterDword(0x4C, 0);

		KeWaitForSymbol(SmNetwork_Waiting);
		KeSetSymbol(SmNetwork_Ready);

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

					TransmitDescriptor td;
					*(dword*)(&td) = ReadRegisterDword(0x10 + nextTransmitIndex * 4);
					if (td.OWN == 1)
					{
						word packetLen = N.GetSize();
						if (packetLen < 60)
						{
							// Padding
							memset(sendBuffers[nextTransmitIndex] + packetLen, 0x00, 60 - packetLen);
							packetLen = 60;
						}
						td.OWN = 0;
						td.SIZE = packetLen;
						memcpy(sendBuffers[nextTransmitIndex], N.GetBuf(), N.GetSize());
						WriteRegisterDword(0x10 + nextTransmitIndex * 4, *(dword*)(&td));
						nextTransmitIndex = (nextTransmitIndex + 1) % 4;
					}
					else
					{
						DebugOut("[tskip]", 7);
					}
				}
				else if (N.GetID() == NfKe_TerminateProcess)
				{
					// Disable transmitter and receiver
					WriteRegisterByte(0x37, 0);
					// Mask all interrupts
					WriteRegisterWord(0x3C, 0);
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
		word isr = ReadRegisterWord(0x3E);

		// transmit ok interrupt
		if ((isr & (1 << 2)) != 0)
			WriteRegisterWord(0x3E, 1 << 2);

		// receive ok interrupt
		if ((isr & (1 << 0)) != 0)
		{
			// While buffer not empty
			while ((ReadRegisterByte(0x37) & (1 << 0)) == 0)
			{
				word packetLen = *(word*)(recvBuffer + nextRecvOffset + 2);
				KeNotify(NfNetwork_RecvdPacket, recvBuffer + nextRecvOffset + 4, packetLen - 4);
				nextRecvOffset = (nextRecvOffset + (packetLen + 4 + 3) & ~3) % 8192;
				WriteRegisterWord(0x38, nextRecvOffset - 16);
			}
			WriteRegisterWord(0x3E, 1 << 0);
		}

		KeEndOfInterrupt(irq);
	}

	byte ReadRegisterByte(byte index)
	{
		return KeInPortByte(baseAddress + index);
	}

	word ReadRegisterWord(byte index)
	{
		return KeInPortWord(baseAddress + index);
	}

	dword ReadRegisterDword(byte index)
	{
		return KeInPortDword(baseAddress + index);
	}

	void WriteRegisterByte(byte index, byte value)
	{
		KeOutPortByte(baseAddress + index, value);
	}

	void WriteRegisterWord(byte index, word value)
	{
		KeOutPortWord(baseAddress + index, value);
	}

	void WriteRegisterDword(byte index, dword value)
	{
		KeOutPortDword(baseAddress + index, value);
	}

	bool Detect()
	{
		KeWaitForSymbol(SmPCI_Ready);
		if (!GetPCIDeviceByID(0x10EC, 0x8139, bus, device, function))
			return false;
		return true;
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_RTL8139))
		return;

	Rtl8139 rtl8139;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=