// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Network.cpp
#include "API.h"

// ----------------------------------------------------------------------------
extern "C" void* memcpy(void* destination, const void* source, size_t num);
extern "C" void* memset(void* ptr, int value, size_t num);

// ----------------------------------------------------------------------------
class Network
{
private:
	byte broadcastMAC[6];
	byte selfMAC[6];
	byte selfIP[4];

public:
	Network()
	{
		KeWaitForSymbol(SmNetwork_Ready);

		KeEnableNotification(NfNetwork_RecvdPacket);

		memset(broadcastMAC, 0xFF, 6);

		selfIP[0] = 178;
		selfIP[1] = 150;
		selfIP[2] = 133;
		selfIP[3] = 127;

		KeRequestCall(ClNetwork_GetSelfMACAddress, null, 0, selfMAC, 6);

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
					ProcessPacket(N.GetBuf(), N.GetSize());
				}
			}
		}
	}

	bool IsMacEqual(byte* mac1, byte* mac2)
	{
		return 
			mac1[0] == mac2[0] &&
			mac1[1] == mac2[1] &&
			mac1[2] == mac2[2] &&
			mac1[3] == mac2[3] &&
			mac1[4] == mac2[4] &&
			mac1[5] == mac2[5];
	}

	bool IsIpEqual(byte* ip1, byte* ip2)
	{
		return 
			ip1[0] == ip2[0] &&
			ip1[1] == ip2[1] &&
			ip1[2] == ip2[2] &&
			ip1[3] == ip2[3];
	}

	void ReadMac(byte* buf, byte* mac)
	{
		memcpy(mac, buf, 6);
	}

	void ReadIp(byte* buf, byte* ip)
	{
		memcpy(ip, buf, 4);
	}

	word ReadWordBE(byte* buf)
	{
		return (buf[0] << 8) | buf[1];
	}

	void WriteMac(byte* buf, byte* mac)
	{
		memcpy(buf, mac, 6);
	}

	void WriteIp(byte* buf, byte* ip)
	{
		memcpy(buf, ip, 4);
	}

	void WriteWordBE(byte* buf, word data)
	{
		buf[0] = data >> 8;
		buf[1] = data & 0xFF;
	}

	void WriteEthernet(byte* buf, byte* dstMAC, byte* srcMAC, word protoType)
	{
		WriteMac(buf + 0x00, dstMAC);
		WriteMac(buf + 0x06, srcMAC);
		WriteWordBE(buf + 0x0C, protoType);
	}

	void ProcessPacket(byte* data, int len)
	{
		if (len < 0x000E)
			return;

		// Ethernet
		byte dstMAC[6];
		byte srcMAC[6];
		word ethProtoType;

		ReadMac(data + 0x00, dstMAC);
		ReadMac(data + 0x06, srcMAC);
		ethProtoType = ReadWordBE(data + 0x0C);

		if (ethProtoType == 0x0806) // ARP
		{
			if (len < 0x002A)
				return;

			word hwType;
			word arpProtoType;
			byte hwSize;
			byte protoSize;
			word opCode;

			hwType = ReadWordBE(data + 0x0E);
			arpProtoType = ReadWordBE(data + 0x10);
			hwSize = data[0x12];
			protoSize = data[0x13];
			opCode = ReadWordBE(data + 0x14);

			if (hwType != 0x0001)
				return;
			if (arpProtoType != 0x0800)
				return;
			if (hwSize != 0x06)
				return;
			if (protoSize != 0x04)
				return;

			if (opCode == 0x0001)
			{
				byte senderMAC[6];
				byte senderIP[4];
				byte targetMAC[6];
				byte targetIP[4];

				ReadMac(data + 0x16, senderMAC);
				ReadIp(data + 0x1C, senderIP);
				ReadMac(data + 0x20, targetMAC);
				ReadIp(data + 0x26, targetIP);

				if (!IsMacEqual(srcMAC, senderMAC))
					return;
				if (!IsMacEqual(dstMAC, broadcastMAC))
					return;
				
				if (IsIpEqual(targetIP, selfIP))
				{
					DebugOut("[arp req from:", 14);
					for (int i = 0; i < 4; i++)
					{
						DebugOut(senderIP[i]);
						DebugOut(".", 1);
					}
					DebugOut("]", 1);

					byte arpReply[0x2A];
					WriteEthernet(arpReply, srcMAC, selfMAC, ethProtoType);
					WriteWordBE(arpReply + 0x0E, hwType);
					WriteWordBE(arpReply + 0x10, arpProtoType);
					arpReply[0x12] = hwSize;
					arpReply[0x13] = protoSize;
					WriteWordBE(arpReply + 0x14, 0x0002);
					WriteMac(arpReply + 0x16, selfMAC);
					WriteIp(arpReply + 0x1C, selfIP);
					WriteMac(arpReply + 0x20, senderMAC);
					WriteIp(arpReply + 0x26, senderIP);

					KeNotify(NfNetwork_SendPacket, arpReply, sizeof(arpReply));
				}
			}
		}
		else if (ethProtoType == 0x0800) // IP
		{
			if (len < 0x0022)
				return;
			byte ipProtoType;
			ipProtoType = data[0x17];

			if (ipProtoType == 0x01) // ICMP
			{
				DebugOut("[icmp]", 6);
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