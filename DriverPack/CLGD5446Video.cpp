// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CLGD5446Video.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CUpdateInfo
{
public:
	dword m_X;
	dword m_Y;
	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
class CCLGD5446Video
{
public:
	CCLGD5446Video()
	{
		if (!Detect())
		{
			KeSetSymbol(SmVideo_Fail);
			return;
		}

		dword Width = 800;
		dword Height = 600;

		dword FrameBufSMID = KeAllocSharedMemAt(
			Width * Height * 4, m_FBBase);
		byte* FrameBuf = KeMapSharedMem(FrameBufSMID);
		for (dword i = 0; i < Width * Height; i++)
			((dword*)FrameBuf)[i] = 0;

		KeUnmapSharedMem(FrameBufSMID);

		InitCLV();

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
		if (!GetPCIDeviceByID(0x1013, 0x00B8, Bus, Device, Function))
			return false;

		dword RegVal = ReadPCIConfDword(Bus, Device, Function, 0x10);
		m_FBBase = RegVal & 0xFF000000;
		return true;
	}

	void InitRegs(word Index, word* Data, dword Size)
	{
		for (dword i = 0; i < Size; i++)
			KeOutPortWord(Index, Data[i]);
	}

	void InitCLV()
	{
		static word SeqRegs[] = 
		{
			0x0300,0x2101,0x0f02,0x0003,0x0e04,
			0x1907,0x230b,0x230c,0x230d,0x230e,
			0x0412,0x0013,0x2017,0x141b,0x141c,
			0x141d,0x141e
		};

		static word GraphRegs[] =
		{
			0x0000,0x0001,0x0002,0x0003,0x0004,
			0x4005,0x0506,0x0f07,0xff08,0x0009,
			0x000a,0x000b
		};

		static word CRTRegs[] = 
		{
			0x2311,0x7d00,0x6301,0x6302,0x8003,
			0x6b04,0x1a05,0x9806,0xf007,0x6009,
			0x000c,0x000d,0x7d10,0x5712,0x9013,
			0x4014,0x5715,0x9816,0xc317,0xff18,
			0x001a,0x321b,0x001d
		};

		KeOutPortWord(0x3C4, 0x1206);
		InitRegs(0x3C4, SeqRegs, 17);
		InitRegs(0x3CE, GraphRegs, 12);
		InitRegs(0x3D4, CRTRegs, 23);

		KeOutPortByte(0x3C6, 0);
		KeInPortByte(0x3C6);
		KeInPortByte(0x3C6);
		KeInPortByte(0x3C6);
		KeInPortByte(0x3C6);
		KeOutPortByte(0x3C6, 0xE5); // HiddenDAC
		KeOutPortByte(0x3C6, 0xFF);
	}

private:
	dword m_FBBase;
};

// ----------------------------------------------------------------------------
void Entry()
{
	CCLGD5446Video CLV;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=