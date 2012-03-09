// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CDFS.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"
#include "Storage.h"

// ----------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)

// ----------------------------------------------------------------------------
class CDirectoryRecord
{
public:
	byte m_RecordLen;
	byte m_ExtAttribLen;
	dword m_ExtentLoc;
	dword m_ExtentLocR;
	dword m_DataLen;
	dword m_DataLenR;
	byte m_DateTime[7];
	byte m_FileFlags;
	byte m_FileUnitSize;
	byte m_IntGapSize;
	dword m_VolSeqNumber;
	byte m_FileNameLen;
};

// ----------------------------------------------------------------------------
#pragma pack(pop)

// ----------------------------------------------------------------------------
class CVirtualDirectoryRecord
{
public:
	CVirtualDirectoryRecord(dword FileSize,
		dword SectorIndex, CStringA& FileName)
		: m_FileName(FileName)
	{
		m_FileSize = FileSize;
		m_SectorIndex = SectorIndex;
	}

	dword GetFileSize()
	{
		return m_FileSize;
	}

	dword GetSectorIndex()
	{
		return m_SectorIndex;
	}

	const CStringA& GetFileName()
	{
		return m_FileName;
	}

private:
	dword m_FileSize;
	dword m_SectorIndex;
	CStringA m_FileName;
};

// ----------------------------------------------------------------------------
class CCDFSDevice
{
public:
	CCDFSDevice(CStorageDeviceInfo& SDI, byte* PrimaryVD)
		: m_SDI(SDI)
	{
		m_CacheRequest[0] = 0;
		m_CacheRequest[1] = SDI.m_DriverID;
		m_CacheRequest[2] = SDI.m_PhysicalDeviceID;
		m_CacheRequest[3] = SDI.m_LogicalDeviceID;

		CDirectoryRecord* DR = (CDirectoryRecord*)(PrimaryVD + 0x9C);

		CVirtualDirectoryRecord VDR(DR->m_DataLen, DR->m_ExtentLoc,
			CStringA(PC(DR) + sizeof(CDirectoryRecord), DR->m_FileNameLen - 1));

		ReadDirectory(VDR.GetSectorIndex(), VDR.GetFileSize(), m_RootDir);
	}

	void ReadDirectory(dword SectorIndex, dword DataLen,
		CArray<CVirtualDirectoryRecord>& Directory)
	{
		Directory.Clear();

		dword SecCount = DataLen / 2048;
		byte* DirBuf = new byte[SecCount * 2048];

		for (dword i = 0; i < SecCount; i++)
			ReadSector(SectorIndex + i, DirBuf + i * 2048);

		dword RecIndex = 0;
		for (;;)
		{
			if (RecIndex >= DataLen)
				break;
			CDirectoryRecord* DR = (CDirectoryRecord*)(DirBuf + RecIndex);
			if (DR->m_RecordLen == 0)
			{
				RecIndex++;
				continue;
			}
			if (DR->m_FileNameLen != 1)
				if ((DR->m_FileFlags & 0x02) == 0)
				{
					CVirtualDirectoryRecord VDR(
						DR->m_DataLen, DR->m_ExtentLoc,
						CStringA(PC(DR) + sizeof(CDirectoryRecord),
						DR->m_FileNameLen));
					if (VDR.GetFileName().Len() != 0)
						Directory.PushBack(VDR);
				}

			RecIndex += DR->m_RecordLen;
		}

		delete DirBuf;
	}

	void ReadSector(dword Index, byte* DataBuf)
	{
		m_CacheRequest[0] = Index;
		KeRequestCall(ClCache_GetSector,
			PB(m_CacheRequest), 16, DataBuf, 2048);
	}

	dword FindFirstFile()
	{
		return FindNextFile(-1);
	}

	dword FindNextFile(dword PrevPosition)
	{
		dword FindPos = PrevPosition + 1;
		if (FindPos >= m_RootDir.Size())
			return -1;
		return FindPos;
	}

	bool GetFileIndexByName(char* Name, dword& FileIndex)
	{
		CStringA SrcName = Name;
		SrcName.ToLower();
		for (dword i = 0; i < m_RootDir.Size(); i++)
		{
			CStringA DirName = m_RootDir[i].GetFileName();
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
		if (!GetFileIndexByName(Name, FileIndex))
			return false;
		FileSize = m_RootDir[FileIndex].GetFileSize();
		return true;
	}

	bool ReadFile(char* Name, byte* FileBuf)
	{
		dword FileIndex;
		if (!GetFileIndexByName(Name, FileIndex))
			return false;

		dword Sector = m_RootDir[FileIndex].GetSectorIndex();
		dword FileSize = m_RootDir[FileIndex].GetFileSize();
		dword FileSizeLeft = FileSize;

		dword Cnt = 0;
		byte SectBuf[2048];
		dword FileOffset = 0;
		for (;;)
		{
			if (FileSizeLeft == 0)
				break;

			ReadSector(Sector, SectBuf);

			dword CopySize = FileSizeLeft;
			if (CopySize > 2048)
				CopySize = 2048;

			for (dword j = 0; j < CopySize; j++)
				FileBuf[FileOffset + j] = SectBuf[j];

			FileSizeLeft -= CopySize;
			FileOffset += 2048;
			Sector++;
		}
		return true;
	}

	dword MakeFindResponse(dword Position, byte* RespBuf)
	{
		(PD(RespBuf))[0] = Position;
		if (Position != -1)
		{
			const CStringA& Name = m_RootDir[Position].GetFileName();
			dword FileNameLen = Name.Len();
			for (dword i = 0; i < FileNameLen; i++)
				RespBuf[4 + i] = Name.GetCh(i);
			RespBuf[4 + FileNameLen] = 0;
			return FileNameLen + 4 + 1;
		}
		else
			return 4;
	}

private:
	dword m_CacheRequest[4];
	CStorageDeviceInfo m_SDI;

	CArray<CVirtualDirectoryRecord> m_RootDir;
};

// ----------------------------------------------------------------------------
class CCDFS
{
public:
	CCDFS()
	{
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(Nf_StorageDeviceCached);
		KeEnableCallRequest(ClCDFS_ReadFile);
		KeEnableCallRequest(ClCDFS_GetFileSize);
		KeEnableCallRequest(ClCDFS_FindFirstFile);
		KeEnableCallRequest(ClCDFS_FindNextFile);

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

					if (SDI.m_SectorSize == 2048)
					{
						dword CacheRequest[4];
						CacheRequest[0] = 16;
						CacheRequest[1] = SDI.m_DriverID;
						CacheRequest[2] = SDI.m_PhysicalDeviceID;
						CacheRequest[3] = SDI.m_LogicalDeviceID;

						byte* PrimaryVD = new byte[2048];
						KeRequestCall(ClCache_GetSector, PB(CacheRequest),
							16, PrimaryVD, 2048);

						if ((PrimaryVD[0x000] == 0x01) &&
							(PrimaryVD[0x001] == 'C') &&
							(PrimaryVD[0x002] == 'D') &&
							(PrimaryVD[0x003] == '0') &&
							(PrimaryVD[0x004] == '0') &&
							(PrimaryVD[0x005] == '1'))
						{
							CFileSystemDeviceInfo FSDI;
							FSDI.m_DeviceID = m_CDFSDevices.Size();
							FSDI.m_ReadFileFunc = ClCDFS_ReadFile;
							FSDI.m_GetFileSizeFunc = ClCDFS_GetFileSize;
							FSDI.m_FindFirstFileFunc = ClCDFS_FindFirstFile;
							FSDI.m_FindNextFileFunc = ClCDFS_FindNextFile;

							m_CDFSDevices.PushBack(CCDFSDevice(SDI, PrimaryVD));
							KeNotify(NfFileSystem_AddDisk, PB(&FSDI), sizeof(FSDI));
						}
						else
							delete PrimaryVD;
					}
				}
				else if (N.GetID() == NfKe_TerminateProcess)
					return;
			}

			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClCDFS_ReadFile)
				{
					dword SMID = CR.GetDword(0);
					dword FSDevID = CR.GetDword(1);
					dword FileSize = 0;
					m_CDFSDevices[FSDevID].GetFileSize(PC(CR.GetBuf() + 8), FileSize);
					if (FileSize == KeGetSharedMemSize(SMID))
					{
						byte* FileBuf = KeMapSharedMem(SMID);
						m_CDFSDevices[FSDevID].ReadFile(PC(CR.GetBuf() + 8), FileBuf);
					}
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClCDFS_GetFileSize)
				{
					dword FileSize = 0;
					dword FSDevID = CR.GetDword(0);
					m_CDFSDevices[FSDevID].GetFileSize(
						PC(CR.GetBuf() + 4), FileSize);
					CR.Respond(FileSize);
				}
				else if (CR.GetTypeID() == ClCDFS_FindFirstFile)
				{
					byte OutBuf[64];
					dword CDFSDeviceIndex = CR.GetDword(0);
					dword OutBufSize =
						m_CDFSDevices[CDFSDeviceIndex].MakeFindResponse(
						m_CDFSDevices[CDFSDeviceIndex].FindFirstFile(), OutBuf);
					CR.Respond(OutBuf, OutBufSize);
				}
				else if (CR.GetTypeID() == ClCDFS_FindNextFile)
				{
					byte OutBuf[64];
					dword PrevPosition = CR.GetDword(0);
					dword CDFSDeviceIndex = CR.GetDword(1);
					dword OutBufSize =
						m_CDFSDevices[CDFSDeviceIndex].MakeFindResponse(
						m_CDFSDevices[CDFSDeviceIndex].FindNextFile(PrevPosition), OutBuf);
					CR.Respond(OutBuf, OutBufSize);
				}
			}
		}
	}

private:
	CArray<CCDFSDevice> m_CDFSDevices;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_CDFS))
		return;
	CCDFS CDFS;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=