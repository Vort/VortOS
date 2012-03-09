// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ATA.cpp
#include "API.h"
#include "BitOp.h"
#include "Storage.h"

// ----------------------------------------------------------------------------
class CATA
{
public:
	bool IsPortsAvail(dword Base)
	{
		KeOutPortByte(Base + m_RegDrive, 0xA0);
		ATADelay(Base);
		KeOutPortByte(Base + m_RegSectorCount, 0x88);
		if (KeInPortByte(Base + m_RegSectorCount) == 0x88)
			return true;

		KeOutPortByte(Base + m_RegDrive, 0xB0);
		ATADelay(Base);
		KeOutPortByte(Base + m_RegSectorCount, 0x88);
		if (KeInPortByte(Base + m_RegSectorCount) == 0x88)
			return true;

		return false;
	}

	void ATADelay(dword BasePort)
	{
		for (dword i = 0; i < 5; i++)
			KeInPortByte(BasePort + m_RegStatus);
	}

	bool Identify(dword Drive)
	{
		dword BasePort = m_BasePrimary;
		if (Drive >= 2)
			BasePort = m_BaseSecondary;

		dword ChDrive = Drive & 1;
		KeOutPortByte(BasePort + m_RegDrive, 0xA0 | (ChDrive << 4));
		ATADelay(BasePort);
		KeOutPortByte(BasePort + m_RegCommand, 0xEC); // Identify

		if (KeInPortByte(BasePort + m_RegStatus) == 0)
			return false;

		bool IsErr = false;
		for (;;)
		{
			byte B = KeInPortByte(BasePort + m_RegStatus);
			if (!(B & 0x80)) // BSY cleared
			{
				if (B & 0x1) // ERR Set
				{
					IsErr = true;
					break;
				}
				if (B & 0x8) // DRQ set
				{
					break;
				}
			}
		}

		if (IsErr)
		{
			byte B1 = KeInPortByte(BasePort + m_RegLBAMid);
			byte B2 = KeInPortByte(BasePort + m_RegLBAHigh);

			if ((B1 == 0x14) && (B2 == 0xEB))
			{
				dword SectCount = 0;
				if (!GetCDCapacity(Drive, SectCount))
					return false;

				m_IsDriveCD[Drive] = true;

				CStorageDeviceInfo SDI;
				SDI.m_DriverID = 0x312AF2BF;
				SDI.m_PhysicalDeviceID = Drive;
				SDI.m_LogicalDeviceID = 0;
				SDI.m_SectorSize = 2048;
				SDI.m_StartSector = 0;
				SDI.m_SectorCount = SectCount;
				SDI.m_CachedSectorsCount = 16;
				SDI.m_ReadSectorFunc = ClATA_ReadSector;
				KeNotify(Nf_StorageDeviceDetected, PB(&SDI), sizeof(SDI));

				return true;
			}
		}

		if (IsErr)
			return false;

		for (dword i = 0; i < 512; i++)
			m_SectorBuf[i] = 0;

		KeInPortWordArray(BasePort + m_RegData, 256, PW(m_SectorBuf));

		dword SectorsCount = (PD(m_SectorBuf))[30];
		if (SectorsCount == 0)
			return false;

		CStorageDeviceInfo SDI;
		SDI.m_DriverID = 0x312AF2BF;
		SDI.m_PhysicalDeviceID = Drive;
		SDI.m_LogicalDeviceID = 0;
		SDI.m_SectorSize = 512;
		SDI.m_StartSector = 0;
		SDI.m_SectorCount = SectorsCount;
		SDI.m_CachedSectorsCount = 16;
		SDI.m_ReadSectorFunc = ClATA_ReadSector;
		KeNotify(Nf_StorageDeviceDetected, PB(&SDI), sizeof(SDI));

		return true;
	}

	bool ExecATAPICommand(dword Drive, byte* Packet,
		dword ReqDataSize, byte* OutDataBuf)
	{
		if (ReqDataSize > 65535)
			return false;
		if (ReqDataSize % 2 != 0)
			return false;

		dword BasePort = m_BasePrimary;
		if (Drive >= 2)
			BasePort = m_BaseSecondary;

		dword ChDrive = Drive & 1;
		KeOutPortByte(BasePort + m_RegDrive, 0xA0 | (ChDrive << 4));

		WaitForNotBusy(BasePort);
		WaitForNotDataTransferRequest(BasePort);

		KeOutPortByte(BasePort + m_RegLBAMid, ReqDataSize & 0xFF);
		KeOutPortByte(BasePort + m_RegLBAHigh, ReqDataSize >> 8);

		KeOutPortByte(BasePort + m_RegCommand, 0xA0); // Packet

		WaitForNotBusy(BasePort);
		WaitForDataTransferRequest(BasePort);

		for (dword i = 0; i < 6; i++)
			KeOutPortWord(BasePort + m_RegData, (PW(Packet))[i]);

		WaitForNotBusy(BasePort);

		byte SB = KeInPortByte(BasePort + m_RegStatus);
		if (TestBit(SB, 0))
			return false;

		if (TestBit(SB, 3))
		{
			dword ActDataSize = 
				KeInPortByte(BasePort + m_RegLBAMid) |
				(KeInPortByte(BasePort + m_RegLBAHigh) << 8);
			if (ActDataSize != ReqDataSize)
				return false;

			KeInPortWordArray(BasePort + m_RegData,
				ReqDataSize / 2, PW(OutDataBuf));

			return true;
		}
		return false;
	}

	bool GetCDCapacity(dword Drive, dword& SectCount)
	{
		byte Packet[12] = {0};
		Packet[0] = 0x25; // READ CAPACITY Command

		byte CmdBuf[8];
		bool R = ExecATAPICommand(Drive, Packet, 8, PB(CmdBuf));
		if (R == false)
			return false;

		dword SectSize = 
			(CmdBuf[4] << 24) |
			(CmdBuf[5] << 16) |
			(CmdBuf[6] << 8) |
			(CmdBuf[7] << 0);

		if (SectSize != 2048)
			return false;

		SectCount = 
			(CmdBuf[0] << 24) |
			(CmdBuf[1] << 16) |
			(CmdBuf[2] << 8) |
			(CmdBuf[3] << 0);

		SectCount++;
		return true;
	}

	bool ReadCDSector(dword Drive, dword LBA)
	{
		byte Packet[12];
		Packet[0] = 0xBE; // READ CD Command
		Packet[1] = 0x00; // Any format
		Packet[2] = (LBA >> 24) & 0xFF;
		Packet[3] = (LBA >> 16) & 0xFF;
		Packet[4] = (LBA >> 8) & 0xFF;
		Packet[5] = (LBA >> 0) & 0xFF;
		Packet[6] = 0; //
		Packet[7] = 0; //
		Packet[8] = 1; // Sect Count
		Packet[9] = 0x10; // Flags
		Packet[10] = 0;
		Packet[11] = 0;
		return ExecATAPICommand(Drive, Packet, 2048, m_SectorBuf);
	}

	bool ReadSector(dword Drive, dword LBA)
	{
		dword BasePort = m_BasePrimary;
		if (Drive >= 2)
			BasePort = m_BaseSecondary;

		dword ChDrive = Drive & 1;
		KeOutPortByte(BasePort + m_RegDrive,
			0xE0 | (ChDrive << 4) | ((LBA >> 24) & 0xF));

		KeOutPortByte(BasePort + m_RegFeatures, 0);     // Delay
		KeOutPortByte(BasePort + m_RegSectorCount, 1);  // Sector Count

		KeOutPortByte(BasePort + m_RegLBALow, LBA & 0xFF);
		KeOutPortByte(BasePort + m_RegLBAMid, (LBA >> 8) & 0xFF);
		KeOutPortByte(BasePort + m_RegLBAHigh, (LBA >> 16) & 0xFF);
		WaitForNotBusy(BasePort);
		KeOutPortByte(BasePort + m_RegCommand, 0x20); // Sect Read W Retry
		WaitForDataTransferRequest(BasePort);
		KeInPortWordArray(BasePort + m_RegData, 256, PW(m_SectorBuf));
		return true;
	}

	CATA()
	{
		m_IsPrimaryMasterDetected = false;
		m_IsPrimarySlaveDetected = false;
		m_IsSecondaryMasterDetected = false;
		m_IsSecondarySlaveDetected = false;

		m_IsDriveCD[0] = false;
		m_IsDriveCD[1] = false;
		m_IsDriveCD[2] = false;
		m_IsDriveCD[3] = false;

		bool IsPrimaryFloating = KeInPortByte(m_BasePrimary + m_RegStatus) == 0xFF;
		bool IsSecondaryFloating = KeInPortByte(m_BaseSecondary + m_RegStatus) == 0xFF;

		bool IsPrimaryPortsAvail = false;
		bool IsSecondaryPortsAvail = false;
		if (!IsPrimaryFloating)
			IsPrimaryPortsAvail = IsPortsAvail(m_BasePrimary);

		if (!IsSecondaryFloating)
			IsSecondaryPortsAvail = IsPortsAvail(m_BaseSecondary);

		KeEnableCallRequest(ClATA_ReadSector);
		KeEnableNotification(NfKe_TerminateProcess);
		KeWaitForSymbol(SmCache_Ready);
		KeWaitForSymbol(SmFileSystem_Ready);

		if (IsPrimaryPortsAvail)
		{
			m_IsPrimaryMasterDetected = Identify(0);
			m_IsPrimarySlaveDetected = Identify(1);
		}
		if (IsSecondaryPortsAvail)
		{
			m_IsSecondaryMasterDetected = Identify(2);
			m_IsSecondarySlaveDetected = Identify(3);
		}

		if (!m_IsPrimaryMasterDetected &&
			!m_IsPrimarySlaveDetected &&
			!m_IsSecondaryMasterDetected &&
			!m_IsSecondarySlaveDetected)
		{
			return;
		}

		CCallRequest<0x100> CR;
		CNotification<0x100> N;
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

				if (CR.GetTypeID() == ClATA_ReadSector)
				{
					dword DevID = CR.GetDword(0);
					dword LBA = CR.GetDword(1);

					if (DevID <= 3)
					{
						if (m_IsDriveCD[DevID])
						{
							ReadCDSector(DevID, LBA);
							CR.Respond(m_SectorBuf, 2048);
						}
						else
						{
							ReadSector(DevID, LBA);
							CR.Respond(m_SectorBuf, 512);
						}
					}
					else
						CR.Respond();
				}
			}
		}
	}

	void WaitForDataTransferRequest(dword BasePort)
	{
		while (!TestBit(KeInPortByte(BasePort + m_RegStatus), 3));
	}

	void WaitForNotDataTransferRequest(dword BasePort)
	{
		while (TestBit(KeInPortByte(BasePort + m_RegStatus), 3));
	}

	void WaitForNotBusy(dword BasePort)
	{
		while (TestBit(KeInPortByte(BasePort + m_RegStatus), 7));
	}

private:
	byte m_SectorBuf[2048];

	bool m_IsPrimaryMasterDetected;
	bool m_IsPrimarySlaveDetected;
	bool m_IsSecondaryMasterDetected;
	bool m_IsSecondarySlaveDetected;

	bool m_IsDriveCD[4];

	static const dword m_BasePrimary    = 0x1F0;
	static const dword m_BaseSecondary  = 0x170;

	static const dword m_RegData        = +0;
	static const dword m_RegFeatures    = +1;
	static const dword m_RegSectorCount = +2;
	static const dword m_RegLBALow      = +3;
	static const dword m_RegLBAMid      = +4;
	static const dword m_RegLBAHigh     = +5;
	static const dword m_RegDrive       = +6;
	static const dword m_RegCommand     = +7;
	static const dword m_RegStatus      = +7;
	/*
	static const dword m_RegData = 0x1F0;
	static const dword m_RegError = 0x1F1;
	static const dword m_RegFeatures = 0x1F1;
	static const dword m_RegSectorCount = 0x1F2;
	static const dword m_RegLBALow = 0x1F3;
	static const dword m_RegLBAMid = 0x1F4;
	static const dword m_RegLBAHigh = 0x1F5;
	static const dword m_RegDrive = 0x1F6;
	static const dword m_RegStatus = 0x1F7;
	static const dword m_RegCommand = 0x1F7;
	static const dword m_RegAltStatus = 0x3F6;
	static const dword m_DeviceControl = 0x3F6;
	*/
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_ATA))
		return;
	CATA A;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=