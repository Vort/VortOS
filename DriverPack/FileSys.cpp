// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FileSystem.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"
#include "Storage.h"

// ----------------------------------------------------------------------------
class CFileSystem
{
public:
	CFileSystem()
	{
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(NfFileSystem_AddDisk);
		KeEnableCallRequest(ClFileSystem_GetDiskCount);
		KeEnableCallRequest(ClFileSystem_ReadFile);
		KeEnableCallRequest(ClFileSystem_GetFileSize);
		KeEnableCallRequest(ClFileSystem_FindFirstFile);
		KeEnableCallRequest(ClFileSystem_FindNextFile);
		KeSetSymbol(SmFileSystem_Ready);

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
				if (N.GetID() == NfFileSystem_AddDisk)
				{
					CFileSystemDeviceInfo FSDI;
					FSDI.m_DeviceID = N.GetDword(0);
					FSDI.m_ReadFileFunc = N.GetDword(1);
					FSDI.m_GetFileSizeFunc = N.GetDword(2);
					FSDI.m_FindFirstFileFunc = N.GetDword(3);
					FSDI.m_FindNextFileFunc = N.GetDword(4);
					m_FSDIs.PushBack(FSDI);

					KeSetSymbol(SmFileSystem_DiskReady);
				}
				else if (N.GetID() == NfKe_TerminateProcess)
					return;
			}

			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClFileSystem_GetDiskCount)
				{
					CR.Respond(m_FSDIs.Size());
				}
				else if (CR.GetTypeID() == ClFileSystem_ReadFile)
				{
					dword SMID = CR.GetDword(0);

					CStringA FSPath;
					CStringA Path = CStringA(PC(CR.GetBuf() + 4));
					dword FSDevID = ParsePath(Path, FSPath);

					byte Buf[256];
					(PD(Buf))[0] = SMID;
					(PD(Buf))[1] = m_FSDIs[FSDevID].m_DeviceID;
					for (dword i = 0; i < FSPath.Len(); i++)
						Buf[i + 8] = FSPath.GetCh(i);
					Buf[FSPath.Len() + 8] = 0;

					KeRequestCall(m_FSDIs[FSDevID].m_ReadFileFunc,
						Buf, FSPath.Len() + 8 + 1, null, 0);
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClFileSystem_GetFileSize)
				{
					dword FileSize = 0;
					CStringA FSPath;
					CStringA Path = CStringA(PC(CR.GetBuf()));
					dword FSDevID = ParsePath(Path, FSPath);

					byte Buf[256];
					(PD(Buf))[0] = m_FSDIs[FSDevID].m_DeviceID;
					for (dword i = 0; i < FSPath.Len(); i++)
						Buf[i + 4] = FSPath.GetCh(i);
					Buf[FSPath.Len() + 4] = 0;

					KeRequestCall(m_FSDIs[FSDevID].m_GetFileSizeFunc,
						Buf, FSPath.Len() + 4 + 1, PB(&FileSize), 4);
					CR.Respond(FileSize);
				}
				else if (CR.GetTypeID() == ClFileSystem_FindFirstFile)
				{
					byte OutBuf[64];
					dword BufSize = 4;
					dword DeviceIndex = CR.GetDword(0);
					KeRequestCall(m_FSDIs[DeviceIndex].m_FindFirstFileFunc,
						PB(&m_FSDIs[DeviceIndex].m_DeviceID), 4, OutBuf, 64);
					for (; OutBuf[BufSize]; BufSize++);
					BufSize++;
					CR.Respond(PB(&OutBuf), BufSize);
				}
				else if (CR.GetTypeID() == ClFileSystem_FindNextFile)
				{
					byte OutBuf[64];
					dword BufSize = 4;
					dword PrevPosition = CR.GetDword(0);
					dword DeviceIndex = CR.GetDword(1);
					dword FindBuf[2];
					FindBuf[0] = PrevPosition;
					FindBuf[1] = m_FSDIs[DeviceIndex].m_DeviceID;
					KeRequestCall(m_FSDIs[DeviceIndex].m_FindNextFileFunc,
						PB(&FindBuf), 8, OutBuf, 64);
					for (; OutBuf[BufSize]; BufSize++);
					BufSize++;
					CR.Respond(PB(&OutBuf), BufSize);
				}
			}
		}
	}

	dword ParsePath(const CStringA& Path, CStringA& FSPath)
	{
		if ((Path.GetCh(0) == '/') &&
			(Path.GetCh(1) == 'd') &&
			(Path.GetCh(3) == '/'))
		{
			dword FSDevIndex = Path.GetCh(2) - '0';
			FSPath = Path.RightAbs(4);
			return FSDevIndex;
		}
		else
		{
			FSPath = Path;
			return 0;
		}
	}

private:
	CArray<CFileSystemDeviceInfo> m_FSDIs;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_FileSys))
		KeExitProcess();

	CFileSystem FS;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=