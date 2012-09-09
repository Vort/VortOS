// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Network.cpp
#include "API.h"

// ----------------------------------------------------------------------------
extern "C" void* memcpy(void* destination, const void* source, size_t num);
extern "C" void* memset(void* ptr, int value, size_t num);

// ----------------------------------------------------------------------------
#pragma pack(push, 1)
struct EthernetHeader
{
	byte DstMac[6];
	byte SrcMac[6];
	word EtherType;
};
struct ArpHeader
{
	EthernetHeader Eth;
	word HwType;
	word ProtoType;
	byte HwLen;
	byte ProtoLen;
	word Operation;
	byte SenderMac[6];
	byte SenderIp[4];
	byte TargetMac[6];
	byte TargetIp[4];
};
struct IpHeader
{
	EthernetHeader Eth;
	byte Ihl:4;
	byte Version:4;
	byte Ecn:2;
	byte Dscp:6;
	word TotalLength;
	word Identification;
	byte FragmentOffsetHi:5;
	byte Flags:3;
	byte FragmentOffsetLo:8;
	byte TimeToLive;
	byte Protocol;
	word HeaderChecksum;
	byte SourceIp[4];
	byte DestinationIp[4];
};
#pragma pack(pop)

// ----------------------------------------------------------------------------
class Network
{
private:
	byte broadcastMac[6];
	byte selfMac[6];
	byte selfIp[4];

public:
	Network()
	{
		KeWaitForSymbol(SmNetwork_Ready);

		KeEnableNotification(NfNetwork_RecvdPacket);

		memset(broadcastMac, 0xFF, 6);

		selfIp[0] = 192;
		selfIp[1] = 168;
		selfIp[2] = 150;
		selfIp[3] = 100;

		KeRequestCall(ClNetwork_GetSelfMACAddress, null, 0, selfMac, 6);

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

	void WriteMac(byte* buf, byte* mac)
	{
		memcpy(buf, mac, 6);
	}

	void WriteIp(byte* buf, byte* ip)
	{
		memcpy(buf, ip, 4);
	}

	word SwapWord(word bigEndianWord)
	{
		return ((bigEndianWord & 0xFF) << 8) | (bigEndianWord >> 8);
	}

	void ProcessArp(ArpHeader* arp)
	{
		if (SwapWord(arp->HwType) != 0x0001)
			return;
		if (SwapWord(arp->ProtoType) != 0x0800)
			return;
		if (arp->HwLen != 0x06)
			return;
		if (arp->ProtoLen != 0x04)
			return;

		if (SwapWord(arp->Operation) == 0x0001)
		{
			if (!IsMacEqual(arp->Eth.SrcMac, arp->SenderMac))
				return;
			if (!IsMacEqual(arp->Eth.DstMac, broadcastMac))
				if (!IsMacEqual(arp->Eth.DstMac, selfMac))
					return;
				
			if (IsIpEqual(arp->TargetIp, selfIp))
			{
				DebugOut("[arp req from:", 14);
				for (int i = 0; i < 4; i++)
				{
					DebugOut(arp->SenderIp[i]);
					DebugOut(".", 1);
				}
				DebugOut("]", 1);

				ArpHeader arpReply;
				WriteMac(arpReply.Eth.DstMac, arp->Eth.SrcMac);
				WriteMac(arpReply.Eth.SrcMac, arp->Eth.DstMac);
				arpReply.Eth.EtherType = arp->Eth.EtherType;
				arpReply.HwType = arp->HwType;
				arpReply.ProtoType = arp->ProtoType;
				arpReply.HwLen = arp->HwLen;
				arpReply.ProtoLen = arp->ProtoLen;
				arpReply.Operation = SwapWord(0x0002);
				WriteMac(arpReply.SenderMac, selfMac);
				WriteIp(arpReply.SenderIp, selfIp);
				WriteMac(arpReply.TargetMac, arp->SenderMac);
				WriteIp(arpReply.TargetIp, arp->SenderIp);

				KeNotify(NfNetwork_SendPacket, (byte*)&arpReply, sizeof(arpReply));
			}
		}
	}

	void ProcessIp(IpHeader* ip)
	{
		if (ip->Version != 4)
			return;
		if (ip->Ihl != 5)
			return;

		if (ip->Protocol == 0x01) // ICMP
		{
			DebugOut("[icmp]", 6);
		}
	}

	void ProcessPacket(byte* data, int len)
	{
		if (len < sizeof(EthernetHeader))
			return;

		DebugOut("p", 1);

		EthernetHeader* eth = (EthernetHeader*)data;
		if (SwapWord(eth->EtherType) == 0x0806) // ARP
		{
			if (len < sizeof(ArpHeader))
				return;
			ArpHeader* arp = (ArpHeader*)data;
			ProcessArp(arp);
		}
		else if (SwapWord(eth->EtherType) == 0x0800) // IP
		{
			if (len < sizeof(IpHeader))
				return;
			IpHeader* ip = (IpHeader*)data;
			ProcessIp(ip);
		}
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	Network network;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=