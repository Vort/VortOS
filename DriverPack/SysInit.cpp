// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SysInit.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"

// ----------------------------------------------------------------------------
class SysInit
{
private:
	dword bootType;

	byte* videoBuf;
	dword cursorIndex;

public:
	SysInit()
	{
		dword vidSmid = KeAllocSharedMemAt(1, 0xB8000);
		videoBuf = KeMapSharedMem(vidSmid);
		cursorIndex = KeGetPreloadedDriversCount();

		KeWaitForSymbol(SmFileSystem_DiskReady);

		bootType = KeGetBootType();
		LoadProcesses();

		KeReleaseSharedMem(vidSmid);
		KeSetSymbol(Sm_InitStage0);
	}

	void LoadKeybAndMouse()
	{
		CreateProcess("i8042.bin");
		CreateProcess("Serial.bin");
		CreateProcess("PS2Keyb.bin");
		CreateProcess("VMwareMouse.bin");
		if (KeWaitForSymbol(SmVMWareMouse_DetectOK,
			SmVMWareMouse_DetectFail) == SmVMWareMouse_DetectFail)
		{
			CreateProcess("PS2Mouse.bin");
		}
		CreateProcess("SerialMouse.bin");
	}

	void LoadProcesses()
	{
		if (bootType == '  dc')
		{
			LoadKeybAndMouse();
			//CreateProcess("Partition.bin");
			//CreateProcess("FAT.bin");
		}
		else if (bootType == ' ddf')
		{
			CreateProcess("Partition.bin");
			//CreateProcess("ATA.bin");
			LoadKeybAndMouse();
		}
		else
		{
			return;
		}
		CreateProcess("PCI.bin");
		CreateProcess("Network.bin");
		//CreateProcess("NE2000.bin");
		//CreateProcess("Am79C970.bin");
		//CreateProcess("Rtl8139.bin");

		char* videoDrvList[] =
		{
			"BochsVideo.bin",
			"CLGD5446Video.bin",
			"S3Trio64Video.bin",
			"GeForceVideo.bin",
			"VMwareVideo.bin",
			"VGAVideo.bin",
			null
		};
		LoadVideo(videoDrvList);

		CreateProcess("Renderer.bin");
		CreateProcess("SurfMgr.bin");
		CreateProcess("Cursor.bin");
		CreateProcess("Desktop.bin");
		CreateProcess("Font.bin");

		KeWaitForSymbol(SmCursor_Ready);
		KeWaitForSymbol(SmDesktop_Ready);
		KeWaitForSymbol(SmFont_Ready);
		KeSetSymbol(Sm_InitStage2);

		CreateProcess("Viewer.bin");
	}

	void LoadVideo(char* videoDrvList[])
	{
		for (int i = 0; ; i++)
		{
			if (videoDrvList[i] == null)
				return;
			CreateProcess(videoDrvList[i]);
			if (KeWaitForSymbol(SmVideo_OK, SmVideo_Fail) == SmVideo_OK)
				return;
			KeResetSymbol(SmVideo_Fail);
		}
	}

	bool CreateProcess(char* fileName)
	{
		cursorIndex++;

		CStringA message = "Loading '";
		message.Add(fileName);
		message.Add("'...");

		WriteText(message._ptr(), (cursorIndex / 24) * 40, cursorIndex % 24, 0xA);

		dword smid = 0;
		bool loaded = true;
		dword fileSize = GetFileSize(fileName);
		if (fileSize != 0)
		{
			smid = KeAllocSharedMem(fileSize);
			ReadFile(smid, fileName);
		}
		else
		{
			loaded = false;
		}

		if (loaded)
		{
			byte* drvImage = KeMapSharedMem(smid);
			if (!KeCreateProcess(drvImage, fileSize, fileName))
				loaded = false;
			KeReleaseSharedMem(smid);
		}

		if (loaded)
			WriteText("OK", message.Len() + (cursorIndex / 24) * 40, cursorIndex % 24, 0xA);
		else
			WriteText("Fail", message.Len() + (cursorIndex / 24) * 40, cursorIndex % 24, 0xC);

		return loaded;
	}

	void WriteText(const char* message, dword x, dword y, byte color)
	{
		if (x > 79) return;
		if (y > 24) return;
		char* ptr = (char*)(videoBuf);
		ptr += (y * 80 + x) * 2;
		for (dword i = 0; message[i] != 0; i++)
		{
			*ptr++ = message[i];
			*ptr++ = color;
		}
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_SysInit))
		return;
	SysInit si;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=