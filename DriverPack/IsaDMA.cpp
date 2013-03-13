// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// IsaDMA.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class IsaDMA
{
public:
	IsaDMA()
	{
		dword SMID = KeAllocSharedMem(512);
		byte* DMAPageV = KeMapSharedMem(SMID);
		dword DMAPageP = KeVirtToPhys(DMAPageV);
		dword BufSizeM1 = 511;

		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableCallRequest(ClDMA_Reset);
		KeSetSymbol(SmDMA_Ready);

		CNotification<4> N;
		CCallRequest<4> CR;
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

				byte Mode = 0x46; // Read
				//byte Mode = 0x4A; // Write

				KeOutPortByte(0x0a, 0x06);   // mask chan 2
				KeOutPortByte(0x0c, 0xff);   // reset flip-flop
				KeOutPortByte(0x04, (DMAPageP >> 0) & 0xFF); //  - address low byte
				KeOutPortByte(0x04, (DMAPageP >> 8) & 0xFF); //  - address high byte
				KeOutPortByte(0x81, (DMAPageP >> 16) & 0xFF); // external page register
				KeOutPortByte(0x0c, 0xff);   // reset flip-flop
				KeOutPortByte(0x05, (BufSizeM1 >> 0) & 0xFF); //  - count low byte
				KeOutPortByte(0x05, (BufSizeM1 >> 8) & 0xFF); //  - count high byte
				KeOutPortByte(0x0b, Mode);   // set mode (see above)
				KeOutPortByte(0x0a, 0x02);   // unmask chan 2

				CR.Respond(SMID);
			}
		}
	}

private:
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_IsaDMA))
		return;
	IsaDMA D;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=