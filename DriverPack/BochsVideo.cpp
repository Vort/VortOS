// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// BochsVideo.cpp
#include "API.h"
#include "GenericVideo.h"

// ----------------------------------------------------------------------------
class CBochsVideo : public CGenericVideo
{
public:
	CBochsVideo()
	{
		if (!Init(0x7BCE8DAD, 0))
			return;

		dword Width = 800;
		dword Height = 600;

		dword FrameBufSMID = KeAllocSharedMemAt(
			Width * Height * 4, 0xE0000000);
		byte* FB = KeMapSharedMem(FrameBufSMID);

		VBEWrite(0x4, 0x00); 
		VBEWrite(0x1, Width); 
		VBEWrite(0x2, Height); 
		VBEWrite(0x3, 32); // BPP
		VBEWrite(0x4, 0x01 | 0x40);

		for (dword i = 0; i < 800*600; i++)
			(PD(FB))[i] = 0;

		KeEnableCallRequest(ClVideo_GetFrameSurface);
		KeEnableCallRequest(ClVideo_GetCaps);
		KeEnableCallRequest(ClVideo_GetQuantSize);
		KeEnableNotification(NfKe_TerminateProcess);

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
		KeOutPortWord(0x01CE, 0); 
		dword R = KeInPortWord(0x01CF);
		if ((R >> 4) == 0xB0C)
			return true;
		return false;
	}

private:
	void VBEWrite(word Index, word Value) 
	{ 
		KeOutPortWord(0x01CE, Index); 
		KeOutPortWord(0x01CF, Value); 
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	CBochsVideo BV;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=