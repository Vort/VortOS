// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Cache.cpp
#include "API.h"
#include "Array2.h"
#include "Storage.h"

// ----------------------------------------------------------------------------
class CStorageCache
{
public:
	CStorageCache(const CStorageDeviceInfo& SDI)
		: m_SDI(SDI)
	{
		m_EntryCount = SDI.m_CachedSectorsCount;
		if (m_EntryCount < 4)
			m_EntryCount = 4;
		else if (m_EntryCount > 128)
			m_EntryCount = 128;

		m_CurrentEntry = 0;
		m_SectorIndexes = new dword[m_EntryCount];
		m_SectorData = new byte[m_EntryCount * SDI.m_SectorSize];

		for (dword i = 0; i < m_EntryCount; i++)
			m_SectorIndexes[i] = -1;
	}

	bool IsCaching(dword StorageDriverID, dword StoragePhysicalDeviceID,
		dword StorageLogicalDeviceID)
	{
		return (m_SDI.m_DriverID == StorageDriverID) &&
			(m_SDI.m_PhysicalDeviceID == StoragePhysicalDeviceID) &&
			(m_SDI.m_LogicalDeviceID == StorageLogicalDeviceID);
	}

	dword GetSectorSize()
	{
		return m_SDI.m_SectorSize;
	}

	byte* GetSector(dword Index)
	{
		for (dword i = 0; i < m_EntryCount; i++)
			if (m_SectorIndexes[i] == Index)
				return m_SectorData + m_SDI.m_SectorSize * i;

		dword ReadFuncParams[2];
		ReadFuncParams[0] = m_SDI.m_PhysicalDeviceID;
		ReadFuncParams[1] = Index + m_SDI.m_StartSector;
		m_SectorIndexes[m_CurrentEntry] = Index;
		KeRequestCall(m_SDI.m_ReadSectorFunc, PB(&ReadFuncParams), 8,
			m_SectorData + m_SDI.m_SectorSize * m_CurrentEntry,
			m_SDI.m_SectorSize);

		dword CachedIndex = m_CurrentEntry;

		m_CurrentEntry++;
		if (m_CurrentEntry >= m_EntryCount)
			m_CurrentEntry = 0;

		return m_SectorData + m_SDI.m_SectorSize * CachedIndex;
	}

	CStorageDeviceInfo& GetStorageDeviceInfo()
	{
		return m_SDI;
	}

private:
	dword m_CurrentEntry;
	dword m_EntryCount;
	byte* m_SectorData;
	dword* m_SectorIndexes;
	CStorageDeviceInfo m_SDI;
};

// ----------------------------------------------------------------------------
class CCache
{
public:
	CCache()
	{
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(Nf_StorageDeviceDetected);
		KeEnableCallRequest(ClCache_GetSector);
		KeEnableCallRequest(ClCache_GetDeviceList);
		KeSetSymbol(SmCache_Ready);

		CNotification<0x100> N;
		CCallRequest<0x100> CR;
		for (;;)
		{
			KeWaitFor(3);
			dword NfCount;
			dword CallCount;
			KeGetNfClCount(NfCount, CallCount);

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == Nf_StorageDeviceDetected)
				{
					CStorageDeviceInfo& SDI = *(CStorageDeviceInfo*)N.GetBuf();
					m_Caches.PushBack(CStorageCache(SDI));
					KeNotify(Nf_StorageDeviceCached, N.GetBuf(), sizeof(CStorageDeviceInfo));
				}
				else if (N.GetID() == NfKe_TerminateProcess)
					return;
			}
			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClCache_GetSector)
				{
					dword SectorLBA = CR.GetDword(0);
					dword StorageDriverID = CR.GetDword(1);
					dword StoragePhysicalDeviceID = CR.GetDword(2);
					dword StorageLogicalDeviceID = CR.GetDword(3);

					bool IsCacheFound = false;
					for (dword i = 0; i < m_Caches.Size(); i++)
						if (m_Caches[i].IsCaching(StorageDriverID,
							StoragePhysicalDeviceID, StorageLogicalDeviceID))
						{
							IsCacheFound = true;
							CR.Respond(
								m_Caches[i].GetSector(SectorLBA), 
								m_Caches[i].GetSectorSize());
							break;
						}

					if (!IsCacheFound)
						CR.Respond();
				}
				else if (CR.GetTypeID() == ClCache_GetDeviceList)
				{
					CArray<CStorageDeviceInfo> SDIs;
					for (dword i = 0; i < m_Caches.Size(); i++)
						SDIs.PushBack(m_Caches[i].GetStorageDeviceInfo());
					if (SDIs.Size() == 0)
						CR.Respond();
					else
					{
						CR.Respond(PB(SDIs._ptr()),
							SDIs.Size() * sizeof(CStorageDeviceInfo));
					}
				}
			}
		}
	}

private:
	CArray<CStorageCache> m_Caches;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Cache))
		return;
	CCache C;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=