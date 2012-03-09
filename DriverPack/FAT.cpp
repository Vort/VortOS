// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FAT.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"
#include "Storage.h"

// ----------------------------------------------------------------------------
class CDirectoryEntry
{
public:
	bool IsFile()
	{
		bool B1 = (Attributes & (0x04/*System*/ |
			0x08/*VolLabel*/ | 0x10/*Dir*/)) == 0;
		bool B2 = (NameBytes[0] != 0) && (byte(NameBytes[0]) != 0xE5);
		return B1 && B2;
	}

	bool IsNamedAs(char* ConvName)
	{
		for (dword i = 0; i < 11; i++)
			if (NameBytes[i] != ConvName[i])
				return false;
		return true;
	}

	byte GetRaw(dword Index)
	{
		return (PB(this))[Index];
	}

public:
	char NameBytes[8];
	char ExtBytes[3];
	byte Attributes;
	byte Reserved;
	byte CreateTime_ms;
	word CreateTime;
	word CreateDate;
	word AccessedDate;
	word ClusterNumber_High;
	word ModifiedTime;
	word ModifiedDate;
	word FirstClusterAddress_FAT12;
	dword SizeofFile;
};

// ----------------------------------------------------------------------------
class CVirtualDirectoryEntry
{
public:
	CVirtualDirectoryEntry(dword FileSize, dword ClusterIndex, CStringA& FileName)
		: m_FileName(FileName)
	{
		m_FileSize = FileSize;
		m_ClusterIndex = ClusterIndex;
	}

	dword GetFileSize()
	{
		return m_FileSize;
	}

	dword GetClusterIndex()
	{
		return m_ClusterIndex;
	}

	const CStringA& GetFileName()
	{
		return m_FileName;
	}

private:
	dword m_FileSize;
	dword m_ClusterIndex;
	CStringA m_FileName;
};

// ----------------------------------------------------------------------------
class CFATDevice
{
public:
	CFATDevice(CStorageDeviceInfo& SDI, byte* Boot, byte FATBits)
		: m_SDI(SDI)
	{
		m_FATBits = FATBits;

		m_CacheRequest[0] = 0;
		m_CacheRequest[1] = SDI.m_DriverID;
		m_CacheRequest[2] = SDI.m_PhysicalDeviceID;
		m_CacheRequest[3] = SDI.m_LogicalDeviceID;

		m_ReservedSectorsCount = *PW(Boot + 0xE);
		m_SectorsPerCluster = Boot[0xD];
		m_RootEntryCount = *PW(Boot + 0x11);
		m_FATSectorsCount = *PW(Boot + 0x16);

		m_RootSectorsCount = m_RootEntryCount / 16;

		m_Boot = Boot;
		m_FAT = new byte[m_FATSectorsCount * 512];
		m_Root = new CDirectoryEntry[m_RootEntryCount];

		for (dword i = 0; i < m_FATSectorsCount; i++)
		{
			m_CacheRequest[0] = i + 1; // i + Boot
			KeRequestCall(ClCache_GetSector, PB(m_CacheRequest), 16, m_FAT + 512 * i, 512);
		}
		for (dword i = 0; i < m_RootSectorsCount; i++)
		{
			m_CacheRequest[0] = i + 1 + 2 * m_FATSectorsCount; // i + Boot + 2*FAT
			KeRequestCall(ClCache_GetSector, PB(m_CacheRequest), 16, PB(m_Root) + 512 * i, 512);
		}

		FillVirtualDirectory();
	}

	void FillVirtualDirectory()
	{
		CStringA LFN;
		CStringA Empty;
		for (dword i = 0; i < m_RootEntryCount; i++)
		{
			CStringA FileName;
			if (m_Root[i].IsFile())
			{
				if (LFN == Empty)
				{
					for (dword j = 0; j < 8; j++)
						if (m_Root[i].NameBytes[j] != ' ')
							FileName.Add(m_Root[i].NameBytes[j]);
					for (dword j = 0; j < 3; j++)
						if (m_Root[i].ExtBytes[j] != ' ')
						{
							if (j == 0)
								FileName.Add('.');
							FileName.Add(m_Root[i].ExtBytes[j]);
						}
					FileName.ToLower();
				}
				else
					FileName = LFN;

				m_VirtualDirectory.PushBack(
					CVirtualDirectoryEntry(m_Root[i].SizeofFile,
					m_Root[i].FirstClusterAddress_FAT12, FileName));
				LFN = Empty;
			}
			else if (m_Root[i].Attributes == 0xF)
			{
				if (m_Root[i].NameBytes[0] & 0x40)
					LFN = Empty;

				CStringA T(LFN);
				LFN = MakeLFNChunk(i);
				LFN.Add(T);
			}
		}
	}

	CStringA MakeLFNChunk(dword Index)
	{
		CStringA R;
		byte Indexes[] = {1,3,5,7,9,14,16,18,20,22,24,28,30};
		for (dword i = 0; i < 13; i++)
		{
			char c = m_Root[Index].GetRaw(Indexes[i]);
			if ((c == 0xFF) || (c == 0))
				break;
			R.Add(c);
		}
		return R;
	}

	dword FindFirstFile()
	{
		return FindNextFile(-1);
	}

	dword MakeFindResponse(dword Position, byte* RespBuf)
	{
		(PD(RespBuf))[0] = Position;
		if (Position != -1)
		{
			const CStringA& Name = m_VirtualDirectory[Position].GetFileName();
			dword FileNameLen = Name.Len();
			for (dword i = 0; i < FileNameLen; i++)
				RespBuf[4 + i] = Name.GetCh(i);
			RespBuf[4 + FileNameLen] = 0;
			return FileNameLen + 4 + 1;
		}
		else
			return 4;
	}

	dword FindNextFile(dword PrevPosition)
	{
		dword FindPos = PrevPosition + 1;
		if (FindPos >= m_VirtualDirectory.Size())
			return -1;
		return FindPos;
	}

	bool GetFileIndexByName2(char* Name, dword& FileIndex)
	{
		CStringA SrcName = Name;
		SrcName.ToLower();
		for (dword i = 0; i < m_VirtualDirectory.Size(); i++)
		{
			CStringA DirName = m_VirtualDirectory[i].GetFileName();
			DirName.ToLower();
			if (DirName == SrcName)
			{
				FileIndex = i;
				return true;
			}
		}
		return false;
	}

	bool GetFileSize(char* Name, dword& FileSize)
	{
		dword FileIndex = 0;
		if (!GetFileIndexByName2(Name, FileIndex))
			return false;
		FileSize = m_VirtualDirectory[FileIndex].GetFileSize();
		return true;
	}

	bool ReadFile(char* Name, byte* FileBuf)
	{
		dword FileIndex;
		if (!GetFileIndexByName2(Name, FileIndex))
			return false;

		dword Cluster = m_VirtualDirectory[FileIndex].GetClusterIndex();
		dword FileSize = m_VirtualDirectory[FileIndex].GetFileSize();
		dword FileSizeLeft = FileSize;

		dword Cnt = 0;
		for (;;)
		{
			if (FileSizeLeft == 0)
				break;
			if (IsEOC(Cluster))
				break;

			ReadCluster(Cluster, FileBuf + Cnt * m_SectorsPerCluster * 512, FileSizeLeft);
			Cluster = GetFATEntry(Cluster);
			Cnt++;
		}
		return true;
	}

	void ReadCluster(dword Cluster, byte* FileBuf, dword& FileSizeLeft)
	{
		dword StartSectNum = m_ReservedSectorsCount + m_FATSectorsCount * 2 +
			m_RootSectorsCount + (Cluster - 2) * m_SectorsPerCluster; 

		byte SectorBuf[512];
		for (dword i = 0; i < m_SectorsPerCluster; i++)
		{
			if (FileSizeLeft == 0)
				return;

			m_CacheRequest[0] = StartSectNum + i;
			KeRequestCall(ClCache_GetSector, PB(m_CacheRequest), 16, SectorBuf, 512);

			dword FileOffset = i * 512;
			dword CopySize = FileSizeLeft;
			if (CopySize > 512)
				CopySize = 512;
			for (dword j = 0; j < CopySize; j++)
				FileBuf[FileOffset + j] = SectorBuf[j];
			FileSizeLeft -= CopySize;
		}
	}

	bool IsEOC(dword FATEntry)
	{
		if (m_FATBits == 12)
		{
			if (FATEntry >= 0xFF8)
				if (FATEntry <= 0xFFF)
					return true;
		}
		else if (m_FATBits == 16)
		{
			if (FATEntry >= 0xFFF8)
				if (FATEntry <= 0xFFFF)
					return true;
		}
		return false;
	}

	dword GetFATEntry(dword Cluster)
	{
		dword Entry = 0;
		if (m_FATBits == 12)
		{
			dword Ofs = (3 * Cluster) / 2;
			if (Cluster & 1) // нечет
			{
				Entry = m_FAT[Ofs] >> 4;
				Entry |= (m_FAT[Ofs+1]) << 4;
			}
			else // чет
			{
				Entry = m_FAT[Ofs];
				Entry |= (m_FAT[Ofs+1] & 0xF) << 8;
			}
		}
		else if (m_FATBits == 16)
		{
			return (PW(m_FAT))[Cluster];
		}
		return Entry;
	}

private:
	dword m_CacheRequest[4];
	CStorageDeviceInfo m_SDI;

	byte m_FATBits;

	byte* m_Boot;
	byte* m_FAT;
	dword m_FATSectorsCount;
	dword m_ReservedSectorsCount;
	dword m_RootSectorsCount;
	dword m_RootEntryCount;
	dword m_SectorsPerCluster;
	CDirectoryEntry* m_Root;
	CArray<CVirtualDirectoryEntry> m_VirtualDirectory;
};

// ----------------------------------------------------------------------------
class CFAT
{
public:
	CFAT()
	{
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(Nf_StorageDeviceCached);
		KeEnableCallRequest(ClFAT_ReadFile);
		KeEnableCallRequest(ClFAT_GetFileSize);
		KeEnableCallRequest(ClFAT_FindFirstFile);
		KeEnableCallRequest(ClFAT_FindNextFile);

		KeWaitForSymbol(SmCache_Ready);
		ProcessCachedDevices();

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
				if (N.GetID() == Nf_StorageDeviceCached)
				{
					CStorageDeviceInfo& SDI = *(CStorageDeviceInfo*)N.GetBuf();
					ProcessSDI(SDI);
				}
				else if (N.GetID() == NfKe_TerminateProcess)
					return;
			}

			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClFAT_ReadFile)
				{
					dword SMID = CR.GetDword(0);
					dword FSDevID = CR.GetDword(1);
					dword FileSize = 0;
					m_FATDevices[FSDevID].GetFileSize(PC(CR.GetBuf() + 8), FileSize);
					if (FileSize == KeGetSharedMemSize(SMID))
					{
						byte* FileBuf = KeMapSharedMem(SMID);
						m_FATDevices[FSDevID].ReadFile(PC(CR.GetBuf() + 8), FileBuf);
					}
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClFAT_GetFileSize)
				{
					dword FileSize = 0;
					dword FSDevID = CR.GetDword(0);
					m_FATDevices[FSDevID].GetFileSize(PC(CR.GetBuf() + 4), FileSize);
					CR.Respond(FileSize);
				}
				else if (CR.GetTypeID() == ClFAT_FindFirstFile)
				{
					byte OutBuf[64];
					dword FATDeviceIndex = CR.GetDword(0);
					dword OutBufSize =
						m_FATDevices[FATDeviceIndex].MakeFindResponse(
						m_FATDevices[FATDeviceIndex].FindFirstFile(), OutBuf);
					CR.Respond(OutBuf, OutBufSize);
				}
				else if (CR.GetTypeID() == ClFAT_FindNextFile)
				{
					byte OutBuf[64];
					dword PrevPosition = CR.GetDword(0);
					dword FATDeviceIndex = CR.GetDword(1);
					dword OutBufSize =
						m_FATDevices[FATDeviceIndex].MakeFindResponse(
						m_FATDevices[FATDeviceIndex].FindNextFile(PrevPosition), OutBuf);
					CR.Respond(OutBuf, OutBufSize);
				}
			}
		}

	}

	void ProcessSDI(CStorageDeviceInfo& SDI)
	{
		if (SDI.m_SectorSize == 512)
		{
			dword CacheRequest[4];
			CacheRequest[0] = 0;
			CacheRequest[1] = SDI.m_DriverID;
			CacheRequest[2] = SDI.m_PhysicalDeviceID;
			CacheRequest[3] = SDI.m_LogicalDeviceID;

			byte* Boot = new byte[512];
			KeRequestCall(ClCache_GetSector, PB(CacheRequest), 16, Boot, 512);

			if ((Boot[0] == 0xEB) &&
				(Boot[0x36] == 'F') &&
				(Boot[0x37] == 'A') &&
				(Boot[0x38] == 'T') &&
				(Boot[0x3B] == ' ') &&
				(Boot[0x3C] == ' ') &&
				(Boot[0x3D] == ' ') &&
				(Boot[0x1FE] == 0x55) &&
				(Boot[0x1FF] == 0xAA))
			{
				byte FATBits = 0;
				if ((Boot[0x39] == '1') && (Boot[0x3A] == '2'))
					FATBits = 12;
				else if ((Boot[0x39] == '1') && (Boot[0x3A] == '6'))
					FATBits = 16;

				if (FATBits != 0)
				{
					CFileSystemDeviceInfo FSDI;
					FSDI.m_DeviceID = m_FATDevices.Size();
					FSDI.m_ReadFileFunc = ClFAT_ReadFile;
					FSDI.m_GetFileSizeFunc = ClFAT_GetFileSize;
					FSDI.m_FindFirstFileFunc = ClFAT_FindFirstFile;
					FSDI.m_FindNextFileFunc = ClFAT_FindNextFile;

					m_FATDevices.PushBack(CFATDevice(SDI, Boot, FATBits));
					KeNotify(NfFileSystem_AddDisk, PB(&FSDI), sizeof(FSDI));
				}
			}
			else
				delete Boot;
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

private:
	CArray<CFATDevice> m_FATDevices;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_FAT))
		return;
	CFAT FAT;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=