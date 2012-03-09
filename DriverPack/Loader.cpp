// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Loader.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"

// ----------------------------------------------------------------------------
class CLoader
{
public:
	CLoader()
	{
		dword VidSMID = KeAllocSharedMemAt(1, 0xB8000);
		byte* VideoBuf = KeMapSharedMem(VidSMID);

		KeWaitForSymbol(SmFileSystem_DiskReady);

		char DrvFileName[] = "Drivers.a";
		dword FileSize = GetFileSize(DrvFileName);
		dword DrvSMID = KeAllocSharedMem(FileSize);
		byte* DriversList = KeMapSharedMem(DrvSMID);
		ReadFile(DrvSMID, DrvFileName);

		CStringA DriverName;
		for (dword i = 0; i < FileSize; i++)
		{
			char C = DriversList[i];
			if (C == 0x0D)
			{
				m_FileList.PushBack(DriverName);
				DriverName = "";
				continue;
			}
			else if (C == 0x0A)
				continue;

			DriverName.Add(C);
		}
		m_FileList.PushBack(DriverName);

		dword j = KeGetPreloadedDriversCount();
		for (dword i = 0; i < m_FileList.Size(); i++)
		{
			if (m_FileList[i].Len())
				if (m_FileList[i].GetCh(0) == ';')
					continue;
			j++;

			dword NameLen = m_FileList[i].Len() + 1;
			RawOutString(VideoBuf, "Loading '", 0 + (j / 24) * 40, j % 24, 0xA);
			RawOutString(VideoBuf, m_FileList[i]._ptr(), 9 + (j / 24) * 40, j % 24, 0xA);
			RawOutString(VideoBuf, "'...", 9 + NameLen - 1 + (j / 24) * 40, j % 24, 0xA);

			dword SMID = 0;
			bool Loaded = true;
			FileSize = GetFileSize(m_FileList[i]._ptr());
			if (FileSize != 0)
			{
				SMID = KeAllocSharedMem(FileSize);
				ReadFile(SMID, m_FileList[i]._ptr());
			}
			else
				Loaded = false;

			if (Loaded)
			{
				byte* DrvImage = KeMapSharedMem(SMID);
				if (!KeCreateProcess(DrvImage, FileSize, m_FileList[i]._ptr()))
					Loaded = false;
				KeReleaseSharedMem(SMID);
			}

			if (Loaded)
				RawOutString(VideoBuf, "OK", 13 + NameLen - 1 + (j / 24) * 40, j % 24, 0xA);
			else
				RawOutString(VideoBuf, "Fail", 13 + NameLen - 1 + (j / 24) * 40, j % 24, 0xC);
		}
		KeReleaseSharedMem(DrvSMID);
		KeReleaseSharedMem(VidSMID);
		KeSetSymbol(Sm_InitStage0);
	}

	void RawOutString(byte* VidBuf, const char* Msg, dword X, dword Y, byte Color)
	{
		if (X > 79) return;
		if (Y > 24) return;
		char* Buf = PC(VidBuf);
		Buf += (Y*80+X)*2;
		for (dword i = 0; Msg[i] != 0; i++)
		{
			*Buf++ = Msg[i];
			*Buf++ = Color;
		}
	}

private:
	CArray<CStringA> m_FileList;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Loader))
		return;
	CLoader L;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=