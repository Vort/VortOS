// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// NE2000.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class NE2000
{
private:
	byte bus;
	byte device;
	byte function;
	byte irq;
	dword baseAddress;

	byte macAddr[6];
	word packetBuf[1024];
	bool sending;
	byte nextPacketPage;

public:
	NE2000()
	{
		if (!Detect())
			return;

		sending = false;
		nextPacketPage = 0x48;

		KeEnableNotification(NfNetwork_SendPacket);
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableCallRequest(ClNetwork_GetSelfMACAddress);

		// Get I/O base address
		baseAddress = ReadPCIConfDword(bus, device, function, 0x10) & ~0x3;

		// Link IRQ
		irq = ReadPCIConfByte(bus, device, function, 0x3C);
		KeLinkIrq(irq);

		// page = 1, no DMA, start
		WriteRegisterByte(0x0, 0x62); // CR
		// current read pointer = page 72
		WriteRegisterByte(0x7, 0x48); // CURR
		// page = 0, no DMA, start
		WriteRegisterByte(0x0, 0x22); // CR
		// read ring = page 72 .. page 96
		WriteRegisterByte(0x1, 0x48); // PSTART
		WriteRegisterByte(0x2, 0x60); // PSTOP
		// last read packet at page 96
		WriteRegisterByte(0x3, 0x48); // BNRY
		// accept broadcast
		WriteRegisterByte(0xC, 0x04); // RCR
		// fifo = 2, no loopback, LE byte order, word DMA
		WriteRegisterByte(0xE, 0xC9); // DCR
		// unmask PTX and PRX
		WriteRegisterByte(0xF, 0x3);  // IMR

		// Retrieve MAC address
		WriteRegisterByte(0x0, 0x0A); // CR
		WriteRegisterByte(0x8, 0x00); // RSAR0
		WriteRegisterByte(0x9, 0x00); // RSAR1
		WriteRegisterByte(0xA, 0x0C); // RBCR0
		WriteRegisterByte(0xB, 0x00); // RBCR1

		for (int i = 0; i < 6; i++)
			macAddr[i] = KeInPortWord(baseAddress + 0x10);

		// page = 1, no DMA, start
		WriteRegisterByte(0x0, 0x62); // CR

		for (int i = 0; i < 6; i++)
			WriteRegisterByte(1 + i, macAddr[i]); // PAR0-5

		// page = 0, no DMA, start
		WriteRegisterByte(0x0, 0x22); // CR

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
					if (!sending)
					{
						SendPacket(N.GetBuf(), N.GetSize());
					}
					else
					{
						DebugOut("[tskip]");
					}
				}
				else if (N.GetID() == NfKe_TerminateProcess)
				{
					// page = 0, no DMA, stop
					WriteRegisterByte(0x0, 0x01); //CR
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
		byte isr = ReadRegisterByte(0x7);
		if (isr & 0x01) // rx
		{
			for (;;)
			{
				WriteRegisterByte(0x0, 0x4A); // CR
				byte currentPage = ReadRegisterByte(0x7);
				WriteRegisterByte(0x0, 0x0A); // CR

				if (nextPacketPage == currentPage)
					break;

				WriteRegisterByte(0x8, 0x00); // RSAR0
				WriteRegisterByte(0x9, nextPacketPage); // RSAR1
				WriteRegisterByte(0xA, 0x04); // RBCR0
				WriteRegisterByte(0xB, 0x00); // RBCR1

				nextPacketPage = KeInPortWord(baseAddress + 0x10) >> 8;
				word packetSize = KeInPortWord(baseAddress + 0x10) - 4;
				dword wordCount = (packetSize + 1) / 2;
				WriteRegisterByte(0xA, (wordCount * 2) & 0xFF); // RBCR0
				WriteRegisterByte(0xB, (wordCount * 2) >> 8); // RBCR1
				KeInPortWordArray(baseAddress + 0x10, wordCount, packetBuf);

				WriteRegisterByte(0x3, nextPacketPage); // BNRY

				KeNotify(NfNetwork_RecvdPacket, (byte*)packetBuf, packetSize);
			}

			WriteRegisterByte(0x7, 0x01);
		}
		if (isr & 0x02) // tx
		{
			sending = false;
			WriteRegisterByte(0x7, 0x02);
		}
		KeEndOfInterrupt(irq);
	}

	void SendPacket(byte* data, int len)
	{
		sending = true;
		WriteRegisterByte(0x8, 0x00); // RSAR0
		WriteRegisterByte(0x9, 0x40); // RSAR1
		WriteRegisterByte(0xA, len & 0xFF); // RBCR0
		WriteRegisterByte(0xB, len >> 8); // RBCR1
		WriteRegisterByte(0x0, 0x12); // CR

		for (int i = 0; i < len / 2; i++)
			KeOutPortWord(baseAddress + 0x10, ((word*)data)[i]);
		if (len & 1 == 1)
			KeOutPortWord(baseAddress + 0x10, data[len - 1]);

		WriteRegisterByte(0x4, 0x40); // TPSR
		WriteRegisterByte(0x5, len & 0xFF); // TBCR0
		WriteRegisterByte(0x6, len >> 8); // TBCR1
		WriteRegisterByte(0x0, 0x06); // CR
	}

	byte ReadRegisterByte(byte index)
	{
		return KeInPortByte(baseAddress + index);
	}

	void WriteRegisterByte(byte index, byte value)
	{
		KeOutPortByte(baseAddress + index, value);
	}

	bool Detect()
	{
		KeWaitForSymbol(SmPCI_Ready);
		if (!GetPCIDeviceByID(0x10EC, 0x8029, bus, device, function))
			return false;
		return true;
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_NE2000))
		return;

	NE2000 ne2000;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=