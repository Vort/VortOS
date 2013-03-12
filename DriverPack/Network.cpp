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
struct IcmpHeader
{
	IpHeader Ip;
	byte Type;
	byte Code;
};
struct IcmpEchoHeader
{
	IcmpHeader Icmp;
	word Checksum;
	word Identifier;
	word SequenceNumber;
};
struct UdpPseudoHeader
{
	byte SourceIp[4];
	byte DestinationIp[4];
	byte Zero;
	byte Protocol;
	word UdpLength;
};
struct UdpHeader
{
	IpHeader Ip;
	word SourcePort;
	word DestinationPort;
	word Length;
	word Checksum;
};
struct DhcpHeader
{
	UdpHeader Udp;
	byte Op;
	byte HType;
	byte HLen;
	byte Hops;
	dword XId;
	word Secs;
	word Flags;
	byte CIAddr[4];
	byte YIAddr[4];
	byte SIAddr[4];
	byte GIAddr[4];
	byte CHAddr[6];
	byte CHAPadding[10];
	byte SName[64];
	byte File[128];
	dword Magic;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------
class Network
{
private:
	dword dhcpXId;
	byte dhcpSrvIp[4];
	dword discoverTime;
	dword secsElapsed;

	byte broadcastIp[4];
	byte broadcastMac[6];
	byte zeroMac[6];
	byte zeroIp[4];
	byte selfMac[6];
	byte selfIp[4];

public:
	Network()
	{
		secsElapsed = 0;
		memset(broadcastIp, 0xFF, 4);
		memset(broadcastMac, 0xFF, 6);
		memset(zeroMac, 0x00, 6);
		memset(zeroIp, 0x00, 4);
		memset(selfIp, 0x00, 4);
		memset(dhcpSrvIp, 0x00, 4);

		KeEnableNotification(NfNetwork_RecvdPacket);
		KeEnableNotification(NfKe_TerminateProcess);

		KeSetSymbol(SmNetwork_Waiting);
		KeWaitForSymbol(SmNetwork_Ready);
		KeRequestCall(ClNetwork_GetSelfMACAddress, null, 0, selfMac, 6);

		KeEnableNotification(NfKe_TimerTick);
		dhcpXId = GetRandomDword();
		SendDhcpDiscover();

		CNotification<2048> N;
		for (;;)
		{
			KeWaitFor(1);
			dword NfCount = KeGetNotificationCount();

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfKe_TimerTick)
				{
					if (IsIpEqual(dhcpSrvIp, zeroIp))
					{
						if (KeGetTime() - discoverTime >= 5000)
						{
							secsElapsed += 5;
							SendDhcpDiscover();
						}
					}
					else
					{
						KeDisableNotification(NfKe_TimerTick);
					}
				}
				else if (N.GetID() == NfNetwork_RecvdPacket)
				{
					ProcessPacket(N.GetBuf(), N.GetSize());
				}
				else if (N.GetID() == NfKe_TerminateProcess)
				{
					return;
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
				DebugOut("[arp req from:");
				DebugOutIp(arp->SenderIp);
				DebugOut("]");

				ArpHeader arpReply;
				WriteMac(arpReply.Eth.DstMac, arp->Eth.SrcMac);
				WriteMac(arpReply.Eth.SrcMac, selfMac);
				arpReply.Operation = SwapWord(0x0002);
				FillArpHeader(&arpReply);
				WriteMac(arpReply.SenderMac, selfMac);
				WriteIp(arpReply.SenderIp, selfIp);
				WriteMac(arpReply.TargetMac, arp->SenderMac);
				WriteIp(arpReply.TargetIp, arp->SenderIp);

				KeNotify(NfNetwork_SendPacket, (byte*)&arpReply, sizeof(arpReply));
			}
		}
	}

	void ProcessIp(IpHeader* ip, int packetLen)
	{
		if (ip->Version != 4)
			return;
		if (ip->Ihl != 5)
			return;

		if (SwapWord(ip->TotalLength) + sizeof(EthernetHeader) > packetLen)
			return;

		// Fragmentation not supported
		if (ip->Flags == 0x01)
			return;
		if (ip->FragmentOffsetLo != 0)
			return;
		if (ip->FragmentOffsetHi != 0)
			return;

		if (CalcIpChecksum(ip) != 0x0000)
			return;

		if (ip->Protocol == 0x01) // ICMP
		{
			if (packetLen < sizeof(IcmpHeader))
				return;
			IcmpHeader* icmp = (IcmpHeader*)ip;
			ProcessIcmp(icmp);
		}
		else if (ip->Protocol == 0x11) // UDP
		{
			if (packetLen < sizeof(UdpHeader))
				return;
			UdpHeader* udp = (UdpHeader*)ip;
			ProcessUdp(udp);
		}
	}

	word CalcIcmpEchoChecksum(IcmpEchoHeader* icmpEcho, int dataLen)
	{
		return CalcInternetChecksum(0xFFFF,
			(byte*)icmpEcho + sizeof(IpHeader),
			sizeof(IcmpEchoHeader) - sizeof(IpHeader) + dataLen);
	}

	word CalcIpChecksum(IpHeader* ip)
	{
		return CalcInternetChecksum(0xFFFF,
			(byte*)ip + sizeof(EthernetHeader),
			sizeof(IpHeader) - sizeof(EthernetHeader));
	}

	word CalcUdpChecksum(UdpHeader* udp)
	{
		UdpPseudoHeader ph;
		WriteIp(ph.SourceIp, udp->Ip.SourceIp);
		WriteIp(ph.DestinationIp, udp->Ip.DestinationIp);
		ph.Zero = 0x00;
		ph.Protocol = 0x11;
		ph.UdpLength = udp->Length;
		return CalcInternetChecksum(CalcInternetChecksum(0xFFFF,
			(byte*)&ph, sizeof(UdpPseudoHeader)),
			(byte*)udp + sizeof(IpHeader), SwapWord(udp->Length));
	}

	void ProcessUdp(UdpHeader* udp)
	{
		word udpLength = SwapWord(udp->Ip.TotalLength) +
			sizeof(EthernetHeader) - sizeof(IpHeader);

		if (udpLength != SwapWord(udp->Length))
			return;

		if (udp->Checksum != 0x0000)
		{
			if (CalcUdpChecksum(udp) != 0x0000)
				return;
		}

		word dataLength = SwapWord(udp->Ip.TotalLength) +
			sizeof(EthernetHeader) - sizeof(UdpHeader);

		if ((SwapWord(udp->SourcePort) == 67) &&
			(SwapWord(udp->DestinationPort) == 68))
		{
			if (dataLength < (sizeof(DhcpHeader) - sizeof(UdpHeader)))
				return;
			DhcpHeader* dhcp = (DhcpHeader*)udp;
			ProcessDhcp(dhcp);
		}

		if (SwapWord(udp->DestinationPort) == 12321)
		{
			DebugOut("[udp:");
			DebugOut((char*)udp + sizeof(UdpHeader), dataLength);
			DebugOut("]");
		}
	}

	void ProcessDhcp(DhcpHeader* dhcp)
	{
		if (dhcp->Op != 0x02)
			return;
		if (dhcp->XId != dhcpXId)
			return;
		if (!IsMacEqual(dhcp->CHAddr, selfMac))
			return;
		if (dhcp->Magic != 0x63538263)
			return;

		byte msgType = 0;
		byte serverIp[4] = {0};
		byte* optionsPtr = (byte*)dhcp + sizeof(DhcpHeader);
		for (;;)
		{
			byte code = *optionsPtr++;
			if (code == 0x00)
			{
				continue;
			}
			else if (code == 0xFF)
			{
				break;
			}
			else
			{
				byte len = *optionsPtr++;
				if (code == 53)
				{
					msgType = *optionsPtr;
				}
				else if (code == 54)
				{
					WriteIp(serverIp, optionsPtr);
				}
				optionsPtr += len;
			}
		}

		if (msgType == 0x02)
		{
			if (!IsIpEqual(dhcpSrvIp, zeroIp))
				return;

			WriteIp(dhcpSrvIp, serverIp);
			SendDhcpRequest(dhcp->YIAddr, serverIp);
		}
		else if (msgType == 0x05)
		{
			if (!IsIpEqual(serverIp, dhcpSrvIp))
				return;

			WriteIp(selfIp, dhcp->YIAddr);

			ArpHeader arp;
			WriteMac(arp.Eth.DstMac, broadcastMac);
			WriteMac(arp.Eth.SrcMac, selfMac);
			arp.Operation = SwapWord(0x0001);
			FillArpHeader(&arp);
			WriteMac(arp.SenderMac, selfMac);
			WriteIp(arp.SenderIp, selfIp);
			WriteMac(arp.TargetMac, zeroMac);
			WriteIp(arp.TargetIp, selfIp);
			KeNotify(NfNetwork_SendPacket, (byte*)&arp, sizeof(ArpHeader));

			DebugOut("[dhcp_ip:");
			DebugOutIp(selfIp);
			DebugOut("]");
		}
	}

	void DebugOutIp(byte* ip)
	{
		for (int i = 0; i < 4; i++)
		{
			if (i != 0)
				DebugOut(".");
			DebugOutDec(ip[i]);
		}
	}

	void FillArpHeader(ArpHeader* arp)
	{
		arp->Eth.EtherType = SwapWord(0x0806);
		arp->HwType = SwapWord(0x0001);
		arp->ProtoType = SwapWord(0x0800);
		arp->HwLen = 6;
		arp->ProtoLen = 4;
	}

	void ProcessIcmp(IcmpHeader* icmp)
	{
		if (icmp->Type != 8)
			return;
		if (icmp->Code != 0)
			return;
		if (!IsIpEqual(icmp->Ip.DestinationIp, selfIp))
			return;

		IcmpEchoHeader* icmpEcho = (IcmpEchoHeader*)icmp;
		int dataLen = SwapWord(icmp->Ip.TotalLength) + sizeof(EthernetHeader) - sizeof(IcmpEchoHeader);

		if (CalcIcmpEchoChecksum(icmpEcho, dataLen) != 0x0000)
			return;

		DebugOut("[ping]");

		const int replyLen = sizeof(IcmpEchoHeader) + dataLen;
		byte* reply = new byte[replyLen];
		memcpy(reply, icmp, replyLen);
		IcmpEchoHeader* icmpEchoReply = (IcmpEchoHeader*)reply;
		WriteMac(icmpEchoReply->Icmp.Ip.Eth.DstMac, icmp->Ip.Eth.SrcMac);
		WriteMac(icmpEchoReply->Icmp.Ip.Eth.SrcMac, icmp->Ip.Eth.DstMac);
		WriteIp(icmpEchoReply->Icmp.Ip.SourceIp, icmp->Ip.DestinationIp);
		WriteIp(icmpEchoReply->Icmp.Ip.DestinationIp, icmp->Ip.SourceIp);
		icmpEchoReply->Icmp.Ip.Identification = 0x0000;
		icmpEchoReply->Icmp.Ip.HeaderChecksum = 0x0000;
		icmpEchoReply->Icmp.Ip.HeaderChecksum = CalcIpChecksum((IpHeader*)icmpEchoReply);
		icmpEchoReply->Icmp.Type = 0x00;
		icmpEchoReply->Checksum = 0x0000;
		icmpEchoReply->Checksum = CalcIcmpEchoChecksum(icmpEchoReply, dataLen);
		KeNotify(NfNetwork_SendPacket, (byte*)icmpEchoReply, replyLen);
		delete reply;
	}

	word CalcInternetChecksum(word startChecksum, byte* data, int byteCount)
	{
		word wordCount = byteCount / 2;
		dword checksum = ~startChecksum & 0xFFFF;
		for (int i = 0; i < wordCount; i++)
			checksum += ((word*)data)[i];
		if (byteCount % 2 == 1)
			checksum += data[byteCount - 1];
		return ~((checksum & 0xFFFF) + (checksum >> 16));
	}

	void ProcessPacket(byte* data, int packetLen)
	{
		if (packetLen < sizeof(EthernetHeader))
			return;

		DebugOut("p");

		EthernetHeader* eth = (EthernetHeader*)data;
		if (SwapWord(eth->EtherType) == 0x0806) // ARP
		{
			if (packetLen < sizeof(ArpHeader))
				return;
			ArpHeader* arp = (ArpHeader*)data;
			ProcessArp(arp);
		}
		else if (SwapWord(eth->EtherType) == 0x0800) // IP
		{
			if (packetLen < sizeof(IpHeader))
				return;
			IpHeader* ip = (IpHeader*)data;
			ProcessIp(ip, packetLen);
		}
	}

	dword GetRandomDword()
	{
		dword val;
		__asm
		{
			rdtsc
			xor eax, edx
			mov val, eax
		}
		return val;
	}

	void FillDhcpHeader(DhcpHeader* dhcp, int packetLen)
	{
		// Ethernet
		WriteMac(dhcp->Udp.Ip.Eth.DstMac, broadcastMac);
		WriteMac(dhcp->Udp.Ip.Eth.SrcMac, selfMac);
		dhcp->Udp.Ip.Eth.EtherType = SwapWord(0x0800);
		// IP
		dhcp->Udp.Ip.Version = 4;
		dhcp->Udp.Ip.Ihl = 5;
		dhcp->Udp.Ip.Dscp = 0;
		dhcp->Udp.Ip.Ecn = 0;
		dhcp->Udp.Ip.TotalLength = SwapWord(packetLen - sizeof(EthernetHeader));
		dhcp->Udp.Ip.Identification = 0;
		dhcp->Udp.Ip.FragmentOffsetHi = 0;
		dhcp->Udp.Ip.FragmentOffsetLo = 0;
		dhcp->Udp.Ip.Flags = 0;
		dhcp->Udp.Ip.TimeToLive = 0x80;
		dhcp->Udp.Ip.Protocol = 0x11;
		dhcp->Udp.Ip.HeaderChecksum = 0x0000;
		WriteIp(dhcp->Udp.Ip.SourceIp, zeroIp);
		WriteIp(dhcp->Udp.Ip.DestinationIp, broadcastIp);
		dhcp->Udp.Ip.HeaderChecksum = CalcIpChecksum((IpHeader*)dhcp);
		// UDP
		dhcp->Udp.SourcePort = SwapWord(68);
		dhcp->Udp.DestinationPort = SwapWord(67);
		dhcp->Udp.Length = SwapWord(packetLen - sizeof(IpHeader));
		dhcp->Udp.Checksum = 0x0000;
		// DHCP
		dhcp->Op = 0x01;
		dhcp->HType = 0x01;
		dhcp->HLen = 0x06;
		dhcp->Hops = 0x00;
		dhcp->XId = dhcpXId;
		dhcp->Secs = SwapWord(secsElapsed);
		dhcp->Flags = 0x0000;
		WriteMac(dhcp->CHAddr, selfMac);
		dhcp->Magic = 0x63538263;
	}

	void SendDhcpDiscover()
	{
		discoverTime = KeGetTime();

		const int packetLen = sizeof(DhcpHeader) + 4;
		byte packet[packetLen] = {0};
		DhcpHeader* dhcp = (DhcpHeader*)packet;
		FillDhcpHeader(dhcp, packetLen);
		packet[sizeof(DhcpHeader) + 0] = 53;
		packet[sizeof(DhcpHeader) + 1] = 1;
		packet[sizeof(DhcpHeader) + 2] = 1; // Discover
		packet[sizeof(DhcpHeader) + 3] = 255;
		dhcp->Udp.Checksum = CalcUdpChecksum((UdpHeader*)dhcp);
		KeNotify(NfNetwork_SendPacket, packet, packetLen);
	}

	void SendDhcpRequest(byte* clientIp, byte* serverIp)
	{
		const int packetLen = sizeof(DhcpHeader) + 16;
		byte packet[packetLen] = {0};
		DhcpHeader* dhcp = (DhcpHeader*)packet;
		FillDhcpHeader(dhcp, packetLen);
		packet[sizeof(DhcpHeader) + 0] = 53;
		packet[sizeof(DhcpHeader) + 1] = 1;
		packet[sizeof(DhcpHeader) + 2] = 3; // Discover
		packet[sizeof(DhcpHeader) + 3] = 50;
		packet[sizeof(DhcpHeader) + 4] = 4;
		WriteIp(packet + sizeof(DhcpHeader) + 5, clientIp);
		packet[sizeof(DhcpHeader) + 9] = 54;
		packet[sizeof(DhcpHeader) + 10] = 4;
		WriteIp(packet + sizeof(DhcpHeader) + 11, serverIp);
		packet[sizeof(DhcpHeader) + 15] = 255;
		dhcp->Udp.Checksum = CalcUdpChecksum((UdpHeader*)dhcp);
		KeNotify(NfNetwork_SendPacket, packet, packetLen);
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Network))
		return;
	Network network;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=