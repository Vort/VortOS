// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Network.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class Network
{
public:
	Network()
	{
		byte pktData[42] =
		{
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB0, 0xC4,
			0x20, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x01,
			0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xB0, 0xC4,
			0x20, 0x00, 0x00, 0x01, 0xC0, 0xA8, 0x0A, 0x02,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xA8,
			0x0A, 0x01
		};

		KeEnableNotification(NfNetwork_RecvdPacket);
		KeNotify(NfNetwork_SendPacket, pktData, sizeof(pktData));

		CNotification<2048> N;
		for (;;)
		{
			KeWaitFor(1);
			dword NfCount = KeGetNotificationCount();

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfNetwork_RecvdPacket)
				{
					byte* packetBuf = N.GetBuf();
					DebugOut(N.GetSize());
					DebugOut(" ", 1);
					for (int i = 0; i < N.GetSize(); i++)
						DebugOut(packetBuf[i]);
				}
			}
		}
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	Network network;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=