// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// S3Trio64Video.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CS3Trio64Video
{
public:
	CS3Trio64Video()
	{
		if (!Detect())
		{
			KeSetSymbol(SmVideo_Fail);
			return;
		}

		dword Width = 800;
		dword Height = 600;

		dword FrameBufSMID = KeAllocSharedMemAt(Width * Height * 4, m_FBBase);

		InitS3V();

		byte* FrameBuf = KeMapSharedMem(FrameBufSMID);
		for (dword i = 0; i < Width * Height; i++)
			((dword*)FrameBuf)[i] = 0;

		KeUnmapSharedMem(FrameBufSMID);

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
					RespBuf[0] = FrameBufSMID;
					RespBuf[1] = Width;
					RespBuf[2] = Height;
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

		byte Bus;
		byte Device;
		byte Function;
		if (!GetPCIDeviceByID(0x5333, 0x8811, Bus, Device, Function))
			return false;

		dword RegVal = ReadPCIConfDword(Bus, Device, Function, 0x10);
		m_FBBase = RegVal & 0xFFFFF000;
		return true;
	}

private:
	void InitS3V()
	{
		static byte ActlRegs[] = {0x01, 0x00, 0x0F, 0x00, 0x00};
		static byte SeqRegs[] =  {0x03, 0x01, 0x0F, 0x00, 0x06};
		static byte GraphRegs[] =
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF};

		// Reset Attribute Controller Flip-Flop
		KeInPortByte(0x3DA);
		// Set Attribute Controller Regs
		InitRegs(0x03C0, 0x03C0, ActlRegs, 0x10, sizeof(ActlRegs));
		// Set Sequencer Regs
		InitRegs(0x3C4, SeqRegs, 0, sizeof(SeqRegs));
		// Set Graphics Regs
		InitRegs(0x3CE, GraphRegs, 0, sizeof(GraphRegs));

		// Unlock Sequencer PLL Registers
		KeOutPortWord(0x3C4, 0x0608);
		// Unlock CRTC Registers 1
		KeOutPortWord(0x3D4, 0x4838);
		// Unlock CRTC Registers 2
		KeOutPortWord(0x3D4, 0xA539);

		// Linear Address Window Control Register
		byte CR58 = ReadReg(0x3D4, 0x58);
		// Enable Linear Addressing
		// Set Linear Address Window Size = 2 MB
		WriteReg(0x3D4, 0x58, (CR58 & 0xFC) | 0x12);

		// Extended Mode Register
		byte CR43 = ReadReg(0x3D4, 0x43);
		// Disable 64K Color Mode
		WriteReg(0x3D4, 0x43, CR43 & 0xF7);

		// System Configuration Register
		byte CR40 = ReadReg(0x3D4, 0x40);
		// Enable Enhanced Register Access
		WriteReg(0x3D4, 0x40, CR40 | 0x01);

		// Advanced Function Control Register
		// Enable Enhanced Functions
		KeOutPortByte(0x4AE8, 0x03);

		// Vertical Retrace End Register
		byte CR11 = ReadReg(0x3D4, 0x11);
		// Disable CRTC Registers Protect
		WriteReg(0x3D4, 0x11, CR11 & 0x7F);

		static byte CRTRegs[] = 
		{
			0x7B, 0x63, 0x63, 0x80, 0x67, 0x10, 0x6F, 0xF0,
			0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
			0x58, 0x8A, 0x57, 0x90, 0x00, 0x57, 0x00, 0xE3,
			0xFF
		};

		// Set CRTC Regs
		InitRegs(0x3D4, CRTRegs, 0, sizeof(CRTRegs));

		// Set Logical Screen Width Bits (8-9)
		KeOutPortWord(0x3D4, 0x1051);

		// Miscellaneous 1 Register
		// Enable Alternate Refresh Count Control
		// Enable 256 Color Enhanced Mode
		KeOutPortWord(0x3D4, 0x153A);

		// Memory Configuration Register
		// Enable Base Address Offset
		// Use Enhanced Mode Memory Mapping
		KeOutPortWord(0x3D4, 0x0931);

		// Sequencer: 32 BPP (?)
		KeOutPortWord(0x3C4, 0x4018);

		// Sequencer: (?)
		byte SR15 = ReadReg(0x3C4, 0x15);
		WriteReg(0x3C4, 0x15, SR15 & 0xAF);

		// Extended Miscellaneous Control Register 2
		byte CR67 = ReadReg(0x3D4, 0x67);
		// Set 32 BPP Pixel Format
		WriteReg(0x3D4, 0x67, (CR67 & 0x0F) | 0xD0);
	}

	byte ReadReg(dword IndexPort, dword RegIndex)
	{
		KeOutPortByte(IndexPort, RegIndex);
		return KeInPortByte(IndexPort + 1);
	}

	void WriteReg(dword IndexPort, dword RegIndex, byte RegValue)
	{
		KeOutPortWord(IndexPort, (RegValue << 8) | RegIndex);
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
	dword m_FBBase;
};

// ----------------------------------------------------------------------------
void Entry()
{
	CS3Trio64Video S3V;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=