// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// NE2000.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class NE2000
{
private:
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

		KeUnmaskIRQ(9);
		KeEnableNotification(NfKe_IRQ9);
		KeEnableNotification(NfNetwork_SendPacket);
		KeEnableCallRequest(ClNetwork_GetSelfMACAddress);

		// page = 1, no DMA, start
		KeOutPortByte(baseAddress + 0x0, 0x62); // CR
		// current read pointer = page 72
		KeOutPortByte(baseAddress + 0x7, 0x48); // CURR
		// page = 0, no DMA, start
		KeOutPortByte(baseAddress + 0x0, 0x22); // CR
		// read ring = page 72 .. page 96
		KeOutPortByte(baseAddress + 0x1, 0x48); // PSTART
		KeOutPortByte(baseAddress + 0x2, 0x60); // PSTOP
		// last read packet at page 96
		KeOutPortByte(baseAddress + 0x3, 0x48); // BNRY
		// accept broadcast
		KeOutPortByte(baseAddress + 0xC, 0x04); // RCR
		// fifo = 2, no loopback, LE byte order, word DMA
		KeOutPortByte(baseAddress + 0xE, 0xC9); // DCR
		// unmask PTX and PRX
		KeOutPortByte(baseAddress + 0xF, 0x3); // IMR

		// Retrieve MAC address
		KeOutPortByte(baseAddress + 0x0, 0x0A); // CR
		KeOutPortByte(baseAddress + 0x8, 0x00); // RSAR0
		KeOutPortByte(baseAddress + 0x9, 0x00); // RSAR1
		KeOutPortByte(baseAddress + 0xA, 0x0E); // RBCR0
		KeOutPortByte(baseAddress + 0xB, 0x00); // RBCR1

		for (int i = 0; i < 6; i++)
		{
			macAddr[i] = KeInPortWord(baseAddress + 0x10);
		}

		if (KeInPortWord(baseAddress + 0x10) != 0x5757)
			return;

		// page = 1, no DMA, start
		KeOutPortByte(baseAddress + 0x0, 0x62); // CR

		KeOutPortByte(baseAddress + 0x1, macAddr[0]); // PAR0
		KeOutPortByte(baseAddress + 0x2, macAddr[1]); // PAR1
		KeOutPortByte(baseAddress + 0x3, macAddr[2]); // PAR2
		KeOutPortByte(baseAddress + 0x4, macAddr[3]); // PAR3
		KeOutPortByte(baseAddress + 0x5, macAddr[4]); // PAR4
		KeOutPortByte(baseAddress + 0x6, macAddr[5]); // PAR5

		// page = 0, no DMA, start
		KeOutPortByte(baseAddress + 0x0, 0x22); // CR

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
				if (N.GetID() == NfKe_IRQ9)
				{
					ProcessIRQ9();
				}
				else if (N.GetID() == NfNetwork_SendPacket)
				{
					if (!sending)
						SendPacket(N.GetBuf(), N.GetSize());
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

	void ProcessIRQ9()
	{
		byte isr = KeInPortByte(baseAddress + 0x7);
		if (isr & 0x01) // rx
		{
			for (;;)
			{
				KeOutPortByte(baseAddress + 0x0, 0x4A); // CR
				byte currentPage = KeInPortByte(baseAddress + 0x7);
				KeOutPortByte(baseAddress + 0x0, 0x0A); // CR

				if (nextPacketPage == currentPage)
					break;

				KeOutPortByte(baseAddress + 0x8, 0x00); // RSAR0
				KeOutPortByte(baseAddress + 0x9, nextPacketPage); // RSAR1
				KeOutPortByte(baseAddress + 0xA, 0x04); // RBCR0
				KeOutPortByte(baseAddress + 0xB, 0x00); // RBCR1

				nextPacketPage = KeInPortWord(baseAddress + 0x10) >> 8;
				word packetSize = KeInPortWord(baseAddress + 0x10) - 4;
				dword wordCount = (packetSize + 1) / 2;
				KeOutPortByte(baseAddress + 0xA, (wordCount * 2) & 0xFF); // RBCR0
				KeOutPortByte(baseAddress + 0xB, (wordCount * 2) >> 8); // RBCR1
				KeInPortWordArray(baseAddress + 0x10, wordCount, packetBuf);

				KeOutPortByte(baseAddress + 0x3, nextPacketPage); // BNRY

				KeNotify(NfNetwork_RecvdPacket, (byte*)packetBuf, packetSize);
			}

			KeOutPortByte(baseAddress + 0x7, 0x01);
		}
		if (isr & 0x02) // tx
		{
			sending = false;
			KeOutPortByte(baseAddress + 0x7, 0x02);
		}
		KeEndOfInterrupt(9);
	}

	void SendPacket(byte* data, int len)
	{
		sending = true;
		KeOutPortByte(baseAddress + 0x8, 0x00); // RSAR0
		KeOutPortByte(baseAddress + 0x9, 0x40); // RSAR1
		KeOutPortByte(baseAddress + 0xA, len & 0xFF); // RBCR0
		KeOutPortByte(baseAddress + 0xB, len >> 8); // RBCR1
		KeOutPortByte(baseAddress + 0x0, 0x12); // CR

		for (int i = 0; i < len / 2; i++)
			KeOutPortWord(baseAddress + 0x10, ((word*)data)[i]);
		if (len & 1 == 1)
			KeOutPortWord(baseAddress + 0x10, data[len - 1]);

		KeOutPortByte(baseAddress + 0x4, 0x40); // TPSR
		KeOutPortByte(baseAddress + 0x5, len & 0xFF); // TBCR0
		KeOutPortByte(baseAddress + 0x6, len >> 8); // TBCR1
		KeOutPortByte(baseAddress + 0x0, 0x06); // CR
	}

	bool Detect()
	{
		KeWaitForSymbol(SmPCI_Ready);

		byte bus;
		byte device;
		byte function;
		if (!GetPCIDeviceByID(0x10EC, 0x8029, bus, device, function))
			return false;

		baseAddress = GetPCIDeviceReg(bus, device, function, 0x10) & ~0x3;
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