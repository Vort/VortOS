// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Viewer.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"

// ----------------------------------------------------------------------------
class CViewer
{
public:
	CViewer()
	{
		KeWaitForSymbol(Sm_InitStage2);

		m_CursorPos = -1;

		m_X = 280;
		m_Y = 110;

		m_ActiveDisk = 0;
		m_DiskCount = 0;
		m_VCount = 14;
		m_HCount = 3;
		m_VElSize = 14;
		m_VDiskElSize = 14;
		m_HElSize = 110;
		m_HDiskElSize = 22;
		m_Margin = 2;

		m_SurfW = m_HCount * m_HElSize + m_Margin * (2 + m_HCount - 1);
		m_SurfH = m_VCount * m_VElSize + m_VDiskElSize + m_Margin * 3;

		m_SurfaceID = CreateSurface(m_X, m_Y, m_SurfW, m_SurfH);
		KeRequestCall(ClFileSystem_GetDiskCount, null, 0, PB(&m_DiskCount), 4);

		DrawFrame();
		DrawDisks();
		GetFileList();
		DrawFileList();

		ShowSurface(m_SurfaceID);

		KeEnableNotification(Nf_VirtualKey);
		KeEnableNotification(Nf_CursorMoveTo);
		KeEnableNotification(Nf_MouseButtonDown);
		KeEnableNotification(Nf_MouseDoubleClick);
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(NfFileSystem_AddDisk);

		CNotification<0x40> N;
		for (;;)
		{
			KeWaitFor(1);
			dword NfCount = KeGetNotificationCount();
			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == Nf_VirtualKey)
				{
					if (N.GetByte(1) == 1)
					{
						EVirtualKey VK = EVirtualKey(N.GetByte(0));
						if ((VK == VK_GreyDown) || (VK == VK_Num2))
						{
							if (m_CursorPos < (m_FilesCount-1))
							{
								m_CursorPos++;
								UpdateElement(m_CursorPos);
								UpdateElement(m_CursorPos-1);
							}
						}
						else if ((VK == VK_GreyUp) || (VK == VK_Num8))
						{
							if (m_CursorPos != 0)
							{
								m_CursorPos--;
								UpdateElement(m_CursorPos);
								UpdateElement(m_CursorPos+1);
							}
						}
						else if ((VK == VK_GreyLeft) || (VK == VK_Num4))
						{
							if (m_CursorPos != 0)
							{
								dword OldCurPos = m_CursorPos;
								if (m_CursorPos >= m_VCount)
									m_CursorPos -= m_VCount;
								else
									m_CursorPos = 0;
								UpdateElement(OldCurPos);
								UpdateElement(m_CursorPos);
							}
						}
						else if ((VK == VK_GreyRight) || (VK == VK_Num6))
						{
							dword Delta = m_FilesCount - m_CursorPos - 1;
							if (Delta != 0)
							{
								dword OldCurPos = m_CursorPos;
								if (Delta >= m_VCount)
									m_CursorPos += m_VCount;
								else
									m_CursorPos = m_FilesCount - 1;
								UpdateElement(OldCurPos);
								UpdateElement(m_CursorPos);
							}
						}
						else if ((VK == VK_Enter) || (VK == VK_NumEnter))
						{
							if (m_CursorPos != -1)
								Exec();
						}
					}
				}
				else if (N.GetID() == Nf_CursorMoveTo)
				{
					m_CursorX = N.GetDword(0);
					m_CursorY = N.GetDword(1);
				}
				else if (N.GetID() == Nf_MouseButtonDown)
				{
					if (N.GetByte(0) == 0)
					{
						dword mX = m_CursorX - m_X;
						dword mY = m_CursorY - m_Y;
						int X, Y;
						dword W, H;
						dword NewIndex = -1;
						for (dword i = 0; i < m_FilesCount; i++)
						{
							GetElementDimensions(i, X, Y, W, H);
							if ((mX >= X) && (mX < (X+W)))
								if ((mY >= Y) && (mY < (Y+H)))
									NewIndex = i;
						}
						if (NewIndex != -1)
							if (NewIndex != m_CursorPos)
							{
								dword OldIndex = m_CursorPos;
								m_CursorPos = NewIndex;
								UpdateElement(NewIndex);
								UpdateElement(OldIndex);
							}

						NewIndex = -1;
						for (dword i = 0; i < m_DiskCount; i++)
						{
							GetDiskElementDimensions(i, X, Y, W, H);
							if ((mX >= X) && (mX < (X+W)))
								if ((mY >= Y) && (mY < (Y+H)))
									NewIndex = i;
						}
						if (NewIndex != -1)
							if (NewIndex != m_ActiveDisk)
							{
								dword OldIndex = m_ActiveDisk;
								m_ActiveDisk = NewIndex;

								DrawFrame();
								DrawDisks();
								GetFileList();
								DrawFileList();
							}
					}
				}
				else if (N.GetID() == Nf_MouseDoubleClick)
				{
					dword mX = m_CursorX - m_X;
					dword mY = m_CursorY - m_Y;
					int X, Y;
					dword W, H;
					dword NewIndex = -1;
					for (dword i = 0; i < m_FilesCount; i++)
					{
						GetElementDimensions(i, X, Y, W, H);
						if ((mX >= X) && (mX < (X+W)))
							if ((mY >= Y) && (mY < (Y+H)))
								NewIndex = i;
					}
					if (NewIndex != -1)
						Exec();
				}
				else if (N.GetID() == NfFileSystem_AddDisk)
				{
					m_DiskCount++;
					RedrawDisks();
				}
				else if (N.GetID() == NfKe_TerminateProcess)
					return;
			}
		}
	}

	void Exec()
	{
		CStringA FileName = "/d";
		FileName.Add('0' + m_ActiveDisk);
		FileName.Add('/');
		FileName.Add(m_FileNames[m_CursorPos]);
		dword FileSize = GetFileSize(FileName._ptr());
		dword SMID = KeAllocSharedMem(FileSize);
		byte* ImageBuf = KeMapSharedMem(SMID);
		ReadFile(SMID, FileName._ptr());
		KeCreateProcess(ImageBuf, FileSize, m_FileNames[m_CursorPos]._ptr());
		KeReleaseSharedMem(SMID);
	}

	void GetDiskElementDimensions(dword Index, int& X, int& Y, dword& W, dword& H)
	{
		X = m_Margin * (2 + Index) + m_HDiskElSize * Index;
		Y = m_Margin;
		W = m_HDiskElSize;
		H = m_VDiskElSize;
	}

	void GetElementDimensions(dword Index, int& X, int& Y, dword& W, dword& H)
	{
		X = m_Margin + m_HElSize * (Index / m_VCount) + m_Margin / 2;
		Y = m_Margin * 2 + m_VDiskElSize + (Index % m_VCount) * m_VElSize;
		W = m_HElSize - m_Margin;
		H = m_VElSize;
	}

	void UpdateDiskElement(dword Index)
	{
		char DStr[] = "d0";
		DStr[1] = Index + '0';
		dword Color1 = 0xFF400000;
		dword Color2 = 0xFFFFFFFF;
		if (m_ActiveDisk == Index)
		{
			Color1 = 0xFFFFF4F8;
			Color2 = 0xFFFF8870;
		}
		int X, Y;
		dword W, H;
		GetDiskElementDimensions(Index, X, Y, W, H);
		DrawRect(m_SurfaceID, X, Y, W, H, Color2);
		OutText(m_SurfaceID, X + 4, Y, Color1, DStr);
	}

	void UpdateElement(dword Index)
	{
		dword Color1 = 0xFF000040;
		dword Color2 = 0xFFFFFFFF;
		if (m_CursorPos == Index)
		{
			Color1 = 0xFFF4F8FF;
			Color2 = 0xFF7088FF;
		}
		int X, Y;
		dword W, H;
		GetElementDimensions(Index, X, Y, W, H);
		DrawRect(m_SurfaceID, X, Y, W, H, Color2);
		OutText(m_SurfaceID, X + 1, Y, Color1, m_FileNames[Index]._ptr());
	}

	void RedrawDisks()
	{
		DrawRect(m_SurfaceID, 0, m_Margin,
			m_SurfW, m_VDiskElSize, 0xFFFFFFFF);
		DrawDisks();
	}

	void DrawDisks()
	{
		for (dword i = 0; i < m_DiskCount; i++)
			UpdateDiskElement(i);
	}

	void DrawFileList()
	{
		for (dword i = 0; i < m_FilesCount; i++)
			UpdateElement(i);
	}

	void GetFileList()
	{
		m_FilesCount = 0;
		m_FileNames.Clear();

		char RespBuf[64];
		KeRequestCall(ClFileSystem_FindFirstFile, PB(&m_ActiveDisk), 4, PB(RespBuf), 64);
		dword FirstPos = *PD(&RespBuf[0]);
		if (FirstPos == -1)
			return;
		m_FilesCount++;
		m_CursorPos = 0;
		m_FileNames.PushBack(CStringA(RespBuf + 4));

		dword FilePos = FirstPos;
		for (;;)
		{
			if (m_FilesCount == m_VCount * m_HCount)
				return;

			dword FindBuf[2];
			FindBuf[0] = FilePos;
			FindBuf[1] = m_ActiveDisk;
			KeRequestCall(ClFileSystem_FindNextFile,
				PB(FindBuf), 8, PB(RespBuf), 64);
			FilePos = *PD(&RespBuf[0]);
			if (FilePos == -1)
				return;

			m_FileNames.PushBack(CStringA(RespBuf + 4));
			m_FilesCount++;
		}
	}

	void DrawFrame()
	{
		FillSurface(m_SurfaceID, 0xFFFFFFFF);
	}

private:
	dword m_X;
	dword m_Y;
	dword m_SurfW;
	dword m_SurfH;

	dword m_VCount;
	dword m_HCount;
	dword m_VDiskElSize;
	dword m_VElSize;
	dword m_HElSize;
	dword m_HDiskElSize;
	dword m_Margin;

	dword m_CursorX;
	dword m_CursorY;
	dword m_SurfaceID;

	dword m_FilesCount;
	dword m_DiskCount;
	dword m_CursorPos;
	dword m_ActiveDisk;
	CArray<CStringA> m_FileNames;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Viewer))
		return;
	CViewer V;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=