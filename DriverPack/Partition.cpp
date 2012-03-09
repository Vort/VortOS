// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Partition.cpp
#include "API.h"
#include "Storage.h"

// ----------------------------------------------------------------------------
class CPartition
{
public:
	CPartition()
	{
		m_DiskIndex = 1;

		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(Nf_StorageDeviceCached);
		KeSetSymbol(SmPartition_Ready);

		ProcessCachedDevices();

		CNotification<0x100> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();

			if (N.GetID() == Nf_StorageDeviceCached)
			{
				CStorageDeviceInfo& SDI = *(CStorageDeviceInfo*)N.GetBuf();
				ProcessSDI(SDI);
			}
			else if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}

	void ProcessSDI(CStorageDeviceInfo& SDI)
	{
		if ((SDI.m_StartSector == 0) &&
			(SDI.m_SectorSize == 512) &&
			(SDI.m_LogicalDeviceID == 0))
		{
			ProcessPartitionTable(SDI, 0, SDI.m_SectorCount);
		}
	}

	void ProcessCachedDevices()
	{
		byte Buf[0x1000];
		dword ActualBufSize = 0;
		KeRequestCall(ClCache_GetDeviceList,
			null, 0, Buf, 0x1000, ActualBufSize);

		if (ActualBufSize == 0)
			return;
		if ((ActualBufSize % sizeof(CStorageDeviceInfo)) != 0)
			return;

		CStorageDeviceInfo* SDIs = (CStorageDeviceInfo*)Buf;
		dword SDICount = ActualBufSize / sizeof(CStorageDeviceInfo);
		for (dword i = 0; i < SDICount; i++)
			ProcessSDI(SDIs[i]);
	}

	void ProcessPartitionTable(CStorageDeviceInfo& SDI, dword Sector, dword SectorCount)
	{
		byte* Boot = new byte[512];

		dword CacheRequest[4];
		CacheRequest[0] = Sector;
		CacheRequest[1] = SDI.m_DriverID;
		CacheRequest[2] = SDI.m_PhysicalDeviceID;
		CacheRequest[3] = SDI.m_LogicalDeviceID;

		KeRequestCall(ClCache_GetSector, PB(CacheRequest), 16, Boot, 512);

		if (Boot[0] == 0xEB)
			return;
		if (Boot[0x1FE] != 0x55)
			return;
		if (Boot[0x1FF] != 0xAA)
			return;

		byte* PartRecord = Boot + 0x1AE;
		for (int i = 0; i < 4; i++)
		{
			PartRecord += 0x10;

			dword PartSectorStart = *PD(PartRecord + 8);
			dword PartSectorCount = *PD(PartRecord + 12);
			byte PartType = PartRecord[4];

			if (PartSectorStart == 0)
				continue;
			if (PartSectorStart + PartSectorCount > SectorCount)
				continue;
			if (PartType == 0x00)
				continue;

			if (PartType == 0x05)
			{
				ProcessPartitionTable(SDI, Sector + PartSectorStart, PartSectorCount);
			}
			else
			{
				CStorageDeviceInfo PartitionDevice;
				PartitionDevice.m_DriverID = SDI.m_DriverID;
				PartitionDevice.m_PhysicalDeviceID = SDI.m_PhysicalDeviceID;
				PartitionDevice.m_LogicalDeviceID = m_DiskIndex++;
				PartitionDevice.m_SectorSize = SDI.m_SectorSize;
				PartitionDevice.m_SectorCount = PartSectorCount;
				PartitionDevice.m_StartSector = PartSectorStart + Sector;
				PartitionDevice.m_CachedSectorsCount = 64;
				PartitionDevice.m_ReadSectorFunc = SDI.m_ReadSectorFunc;
				KeNotify(Nf_StorageDeviceDetected, PB(&PartitionDevice), sizeof(PartitionDevice));
			}
		}

		delete Boot;
	}

private:
	dword m_DiskIndex;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Partition))
		return;
	CPartition P;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=