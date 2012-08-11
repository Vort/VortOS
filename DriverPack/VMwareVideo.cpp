// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// VMwareVideo.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CVMwareVideo
{
public:
	CVMwareVideo()
	{
		if (!Detect())
		{
			KeSetSymbol(SmVideo_Fail);
			return;
		}

		m_ValuePort = m_IndexPort + 1;

		dword Width = 800;
		dword Height = 600;
		dword FB = InitSVGA();

		dword FrameBufSMID = KeAllocSharedMemAt(
			Width * Height * 4, FB);
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
		if (!GetPCIDeviceByID(0x15AD, 0x0405, Bus, Device, Function))
			return false;

		dword RegVal = GetPCIDeviceReg(Bus, Device, Function, 0x10);
		m_IndexPort = RegVal & (~3);
		return true;
	}

private:
	dword InitSVGA()
	{
		for (int i = 0; i < 3; i++)
		{
			dword SID = (0x900000 << 8) | (2 - i);
			WriteSVGARegister(0, SID);
			dword RID = ReadSVGARegister(0);
			if (SID == RID)
				break;
		}

		dword FrameBuf = ReadSVGARegister(13);

		WriteSVGARegister(2, 800); // W
		WriteSVGARegister(3, 600); // H
		WriteSVGARegister(7, 32);  // Bpp
		WriteSVGARegister(1, 1);   // Enable
		return FrameBuf;
	}

	void WriteSVGARegister(dword Register, dword Value)
	{
		KeOutPortDword(m_IndexPort, Register);
		KeOutPortDword(m_ValuePort, Value);
	}

	dword ReadSVGARegister(dword Register)
	{
		KeOutPortDword(m_IndexPort, Register);
		return KeInPortDword(m_ValuePort);
	}

private:
	dword m_IndexPort;
	dword m_ValuePort;
};

// ----------------------------------------------------------------------------
void Entry()
{
	CVMwareVideo WV;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=