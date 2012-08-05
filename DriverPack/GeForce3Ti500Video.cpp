// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GeForce3Ti500Video.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class GeForce3Ti500Video
{
public:
	GeForce3Ti500Video()
	{
		if (!Detect())
		{
			KeSetSymbol(SmVideo_Fail);
			return;
		}

		dword width = 640;
		dword height = 480;

		mmioSMID = KeAllocSharedMemAt(0x1000000, bar0);
		vramSMID = KeAllocSharedMemAt(width * height * 4, bar1);
		mmioBase = KeMapSharedMem(mmioSMID);
		vramBase = KeMapSharedMem(vramSMID);

		InitGfV();

		for (dword i = 0; i < width * height; i++)
			((dword*)vramBase)[i] = 0;

		KeEnableCallRequest(ClVideo_GetFrameSurface);
		KeEnableCallRequest(ClVideo_GetCaps);
		KeEnableCallRequest(ClVideo_GetQuantSize);
		KeEnableNotification(NfKe_TerminateProcess);

		KeSetSymbol(SmVideo_OK);

		CCallRequest<4> CR;
		CNotification<4> N;
		for (;;)
		{
			KeWaitFor(3);
			dword NfCount;
			dword CallCount;
			KeGetNfClCount(NfCount, CallCount);

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfKe_TerminateProcess)
					return;
			}

			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClVideo_GetFrameSurface)
				{
					dword RespBuf[3];
					RespBuf[0] = vramSMID;
					RespBuf[1] = width;
					RespBuf[2] = height;
					CR.Respond(PB(RespBuf), 12);
				}
				else if (CR.GetTypeID() == ClVideo_GetCaps)
				{
					byte Caps[2];
					Caps[0] = 1; // Double Buffering
					Caps[1] = 0; // No Driver Update
					CR.Respond(Caps, 2);
				}
				else if (CR.GetTypeID() == ClVideo_GetQuantSize)
				{
					dword QuantSize = 32;
					CR.Respond(QuantSize);
				}
			}
		}
	}

	bool Detect()
	{
		KeWaitForSymbol(SmPCI_Ready);

		byte bus;
		byte device;
		byte function;
		if (!GetPCIDeviceByID(0x10DE, 0x0202, bus, device, function))
			return false;

		bar0 = GetPCIDeviceReg(bus, device, function, 0x10) & ~0x3;
		bar1 = GetPCIDeviceReg(bus, device, function, 0x14) & ~0xF;

		return true;
	}

private:
	void WriteCrtcReg(byte index, byte value)
	{
		mmioBase[0x6013D4] = index;
		mmioBase[0x6013D5] = value;
	}

	void InitGfV()
	{
		static byte actlRegs[] = {0x01, 0x00, 0x0F, 0x00, 0x00};
		static byte seqRegs[] =  {0x03, 0x01, 0x0F, 0x00, 0x0E};
		static byte graphRegs[] =
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF};

		// Reset Attribute Controller Flip-Flop
		KeInPortByte(0x03DA);
		// Set Attribute Controller Regs
		InitRegs(0x03C0, actlRegs, 0x10, 5);
		// Set Sequencer Regs
		InitRegs(0x03C4, seqRegs, 0, 5);
		// Set Graphics Regs
		InitRegs(0x03CE, graphRegs, 0, 9);

		// Unlock CRTC Registers 1
		KeOutPortWord(0x03D4, 0x0011);
		// Unlock CRTC Registers 2
		WriteCrtcReg(0x1F, 0x57);

		static byte CRTRegs[] = 
		{
			0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
			0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xEA, 0x8C, 0xDF, 0x40, 0x00, 0xE7, 0x04, 0xE3,
			0xFF
		};

		// Set CRTC Regs
		InitRegs(0x3D4, CRTRegs, 0, 25);
		WriteCrtcReg(0x19, 0x20);
		WriteCrtcReg(0x1A, 0x3C);
		WriteCrtcReg(0x1C, 0x1C);
		WriteCrtcReg(0x28, 0x03);

		// Misc Register Value
		KeOutPortByte(0x03C2, 0xE3);

		// Write Mmio
		*(dword*)(mmioBase + 0x68050c) = 0x10000500;
		*(dword*)(mmioBase + 0x680600) = 0x20100120;

		// Setup RAMDAC
		mmioBase[0x6813C8] = 0x00;
		for (int i = 0; i < 0x100; i++)
		{
			mmioBase[0x6813C9] = (byte)i;
			mmioBase[0x6813C9] = (byte)i;
			mmioBase[0x6813C9] = (byte)i;
		}
	}

	void InitRegs(word IndexPort, byte* Data, dword BaseReg, dword Size)
	{
		for (dword i = 0; i < Size; i++)
		{
			word W = (Data[i] << 8) | (BaseReg + i);
			KeOutPortWord(IndexPort, W);
		}
	}

private:
	dword bar0;
	dword bar1;
	dword mmioSMID;
	dword vramSMID;
	byte* mmioBase;
	byte* vramBase;
};

// ----------------------------------------------------------------------------
void Entry()
{
	GeForce3Ti500Video gf3v;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=