// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GeForceVideo.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class GeForceVideo
{
public:
	GeForceVideo()
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

		mmioBase = null;
		vramBase = null;
		KeUnmapSharedMem(vramSMID);
		KeReleaseSharedMem(mmioSMID);

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
		gpuVersion = 0x00;
		KeWaitForSymbol(SmPCI_Ready);

		byte bus;
		byte device;
		byte function;
		if (GetPCIDeviceByID(0x10DE, 0x0202, bus, device, function))
			gpuVersion = 0x20;
		else if (GetPCIDeviceByID(0x10DE, 0x0DC4, bus, device, function))
			gpuVersion = 0xC3;

		if (gpuVersion != 0x00)
		{
			bar0 = GetPCIDeviceReg(bus, device, function, 0x10) & ~0x3;
			bar1 = GetPCIDeviceReg(bus, device, function, 0x14) & ~0xF;
			return true;
		}
		return false;
	}

private:
	void InitGfV()
	{
		// Reset Attribute Controller Flip-Flop
		KeInPortByte(0x3DA);
		// Set Attribute Controller Regs
		static byte actlRegs[] = {0x01, 0x01, 0x0F, 0x13, 0x00};
		InitRegs(0x03C0, 0x03C0, actlRegs, 0x10, sizeof(actlRegs));

		// Set Sequencer Regs
		static byte seqRegs[] = {0x03, 0x01, 0x0F, 0x00, 0x0E};
		InitRegs(0x03C4, seqRegs, 0, sizeof(seqRegs));

		// Set Graphics Regs
		static byte graphRegs[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF};
		InitRegs(0x03CE, graphRegs, 0, sizeof(graphRegs));

		// Unlock CRTC Registers
		KeOutPortWord(0x03D4, 0x0011);
		if (gpuVersion == 0x20)
			KeOutPortWord(0x03D4, 0x571F);
		else if (gpuVersion == 0xC3)
			KeOutPortWord(0x03D4, 0x573F);

		// Set CRTC Regs
		static byte crtcRegs[] = 
		{
			0x7F, 0x63, 0x63, 0x83, 0x6A, 0x1A, 0x72, 0xF0,
			0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x59, 0x0D, 0x57, 0x90, 0x00, 0x57, 0x73, 0xE3,
			0xFF
		};
		InitRegs(0x3D4, crtcRegs, 0, sizeof(crtcRegs));
		if (gpuVersion == 0x20)
		{
			KeOutPortWord(0x03D4, 0x2019);
			KeOutPortWord(0x03D4, 0x3C1A);
			KeOutPortWord(0x03D4, 0x1C1C);
			KeOutPortWord(0x03D4, 0x2022);
			KeOutPortWord(0x03D4, 0x0328);
		}
		else if (gpuVersion == 0xC3)
		{
			KeOutPortWord(0x03D4, 0x361A);
			KeOutPortWord(0x03D4, 0x4C1B);
			KeOutPortWord(0x03D4, 0x0220);
			KeOutPortWord(0x03D4, 0x012D);
			KeOutPortWord(0x03D4, 0x013B);
			KeOutPortWord(0x03D4, 0x7F40);
			KeOutPortWord(0x03D4, 0x6342);
			KeOutPortWord(0x03D4, 0x6344);
			KeOutPortWord(0x03D4, 0x0346);
			KeOutPortWord(0x03D4, 0x6A48);
			KeOutPortWord(0x03D4, 0x3A4A);
			KeOutPortWord(0x03D4, 0x7250);
			KeOutPortWord(0x03D4, 0x0251);
			KeOutPortWord(0x03D4, 0x5752);
			KeOutPortWord(0x03D4, 0x0253);
			KeOutPortWord(0x03D4, 0x5754);
			KeOutPortWord(0x03D4, 0x0255);
			KeOutPortWord(0x03D4, 0x7356);
			KeOutPortWord(0x03D4, 0x5958);
			KeOutPortWord(0x03D4, 0x0259);
			KeOutPortWord(0x03D4, 0x0D5A);
			KeOutPortWord(0x03D4, 0x2290);
			KeOutPortWord(0x03D4, 0xA092);
			KeOutPortWord(0x03D4, 0x0F93);
		}

		// Misc Register Value
		KeOutPortByte(0x03C2, 0x2B);

		if (gpuVersion == 0x20)
		{
			// Write Mmio
			*(dword*)(mmioBase + 0x680508) = 0x0003F60B;
			*(dword*)(mmioBase + 0x68050c) = 0x10000700;
			*(dword*)(mmioBase + 0x680600) = 0x00100100;
		}
	}

	void InitRegs(word indexPort, word dataPort, byte* data, dword regOffset, dword regCount)
	{
		for (dword i = 0; i < regCount; i++)
		{
			KeOutPortByte(indexPort, regOffset + i);
			KeOutPortByte(dataPort, data[i]);
		}
	}

	void InitRegs(word indexPort, byte* data, dword regOffset, dword regCount)
	{
		InitRegs(indexPort, indexPort + 1, data, regOffset, regCount);
	}

private:
	dword bar0;
	dword bar1;
	dword mmioSMID;
	dword vramSMID;
	byte* mmioBase;
	byte* vramBase;

	byte gpuVersion;
};

// ----------------------------------------------------------------------------
void Entry()
{
	GeForceVideo gfv;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=