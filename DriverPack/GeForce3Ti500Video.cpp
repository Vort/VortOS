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

		dword width = 800;
		dword height = 600;

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
	void InitGfV()
	{
		static byte seqRegs[] =  {0x03, 0x01, 0x0F, 0x00, 0x0E};
		static byte graphRegs[] =
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF};

		// Set Sequencer Regs
		InitRegs(0x03C4, seqRegs, 0, 5);
		// Set Graphics Regs
		InitRegs(0x03CE, graphRegs, 0, 9);

		// Unlock CRTC Registers 1
		KeOutPortWord(0x03D4, 0x0011);
		// Unlock CRTC Registers 2
		KeOutPortWord(0x03D4, 0x571F);

		static byte crtcRegs[] = 
		{
			0x7F, 0x63, 0x63, 0x83, 0x6A, 0x1A, 0x72, 0xF0,
			0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x59, 0x0D, 0x57, 0x90, 0x00, 0x57, 0x73, 0xE3,
			0xFF, 0x20, 0x3C
		};

		// Set CRTC Regs
		InitRegs(0x3D4, crtcRegs, 0, 0x1B);
		KeOutPortWord(0x03D4, 0x1C1C);
		KeOutPortWord(0x03D4, 0x2022);
		KeOutPortWord(0x03D4, 0x0328);

		// Misc Register Value
		KeOutPortByte(0x03C2, 0x2B);

		// Write Mmio
		*(dword*)(mmioBase + 0x680508) = 0x0003F60B;
		*(dword*)(mmioBase + 0x68050c) = 0x10000700;
		*(dword*)(mmioBase + 0x680600) = 0x00100100;
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