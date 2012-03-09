// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// ProcInfo.cpp
#include "API.h"
#include "Array2.h"
#include "String2.h"

// ----------------------------------------------------------------------------
class CProcData
{
public:
	dword m_PID;
	dword m_UsedPageCount;
	dword m_NotificationCount;
	dword m_UserPerfData[128];
	dword m_KernelPerfData[128];
	qword m_UserTotal;
	qword m_KernelTotal;
	dword m_UserPercent;
	dword m_KernelPercent;
	CStringA m_Name;
};

// ----------------------------------------------------------------------------
class CProcInfo
{
public:
	CProcInfo()
	{
		KeWaitForSymbol(Sm_InitStage2);

		m_TickCount = 0;
		m_KernelTime = 0;
		m_VisualProgressIndex = 6;

		m_X = 20;
		m_Y = 20;

		m_Width = 236;
		m_Height = m_Margin * 4 + (m_VisibleProcCount + 4) * m_FontH;

		char Digit = '0';
		KeRequestCall(ClFont_GetTextWidth, PB(&Digit), 1, PB(&m_DigitW), 4);

		m_SurfaceID = CreateSurface(m_X, m_Y, m_Width, m_Height);
		DrawRect(m_SurfaceID, 0, 0, m_Width, m_Margin * 3 + m_FontH * 4, m_BGColor);
		OutText(m_SurfaceID, m_Margin, m_Margin + m_FontH * 0, 0xFFA020A0, "Page Usage:");
		OutText(m_SurfaceID, m_Margin, m_Margin + m_FontH * 1, 0xFFA020A0, "Heap Usage:");
		OutText(m_SurfaceID, m_Margin, m_Margin + m_FontH * 2, 0xFFA020A0, "Kernel Time:");

		OutText(m_SurfaceID, m_Margin+4, m_Margin*2 + m_FontH * 3, 0xFF2020A0, "ID");
		OutText(m_SurfaceID, m_Margin+18, m_Margin*2 + m_FontH * 3, 0xFF20A020, "Mem");
		OutText(m_SurfaceID, m_Margin+52, m_Margin*2 + m_FontH * 3, 0xFFA0A020, "Nf");
		OutText(m_SurfaceID, m_Margin+74, m_Margin*2 + m_FontH * 3, 0xFF20A0A0, "Usr");
		OutText(m_SurfaceID, m_Margin+104, m_Margin*2 + m_FontH * 3, 0xFF20A0A0, "Krn");
		OutText(m_SurfaceID, m_Margin+128, m_Margin*2 + m_FontH * 3, 0xFFA02020, "Name");

		KeEnableNotification(NfKe_IRQ0);
		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<1> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();

			if (N.GetID() == NfKe_IRQ0)
			{
				if (m_TickCount % 144 == 0)
				{
					m_ProcList.Clear();

					CProcData PD;
					char Name[128];
					dword PrevPID = 0;
					for (;;)
					{
						dword PID = KeGetNextProcessInfo(PrevPID,
							PD.m_UsedPageCount, PD.m_NotificationCount,
							PD.m_UserPerfData, PD.m_KernelPerfData, Name);
						if (PID == 0xFFFFFFFF)
							break;
						PD.m_PID = PID;
						PD.m_Name = CStringA(Name);
						m_ProcList.PushBack(PD);
						PrevPID = PID;
					}

					qword UserTotalG = 0;
					qword KernelTotalG = 0;
					for (dword i = 0; i < m_ProcList.Size(); i++)
					{
						qword UserTotalL = 0;
						qword KernelTotalL = 0;
						for (dword j = 0; j < 128; j++)
						{
							UserTotalL += m_ProcList[i].m_UserPerfData[j];
							KernelTotalL += m_ProcList[i].m_KernelPerfData[j];
						}
						m_ProcList[i].m_UserTotal = UserTotalL;
						m_ProcList[i].m_KernelTotal = KernelTotalL;

						UserTotalG += UserTotalL;
						KernelTotalG += KernelTotalL;
					}

					for (dword i = 0; i < m_ProcList.Size(); i++)
					{
						m_ProcList[i].m_UserPercent = (m_ProcList[i].m_UserTotal << 16) / UserTotalG;
						m_ProcList[i].m_KernelPercent = (m_ProcList[i].m_KernelTotal << 16) / KernelTotalG;
					}

					m_KernelTime = (KernelTotalG << 16) / (KernelTotalG + UserTotalG);


					DrawRect(m_SurfaceID, 0, m_Margin * 3 + m_FontH * 4,
						m_Width, m_Height - (m_Margin * 3 + m_FontH * 4), m_BGColor);

					dword VisibleProcCount = m_ProcList.Size();
					if (VisibleProcCount > m_VisibleProcCount)
						VisibleProcCount = m_VisibleProcCount;

					for (dword i = 0; i < VisibleProcCount; i++)
					{
						char CharBuf[5] = {0};
						dword XOffset = m_Margin + 14;
						dword YOffset = m_Margin * 3 + m_FontH * (4 + i);

						OutDecimal(XOffset, YOffset, m_ProcList[i].m_PID, 0xFF2020A0);
						XOffset += 24;

						OutDecimal(XOffset, YOffset, m_ProcList[i].m_UsedPageCount, 0xFF20A020);
						XOffset += 22;

						OutDecimal(XOffset, YOffset, m_ProcList[i].m_NotificationCount, 0xFFA0A020);
						XOffset += 0;

						OutPercent(XOffset, YOffset, m_ProcList[i].m_UserPercent, 0xFF20A0A0);
						XOffset += 32;

						OutPercent(XOffset, YOffset, m_ProcList[i].m_KernelPercent, 0xFF20A0A0);
						XOffset += 36;

						OutText(m_SurfaceID, XOffset, YOffset, 0xFFA02020, m_ProcList[i].m_Name._ptr());
					}
				}

				if (m_TickCount % 24 == 0)
				{
					dword HeapUsed;
					dword HeapTotal;
					dword PageUsed;
					dword PageTotal;
					KeGetMemInfo(HeapUsed, HeapTotal, PageUsed, PageTotal);

					dword PageUsage = dword((qword(PageUsed) * 0x10000) / PageTotal);
					dword HeapUsage = dword((qword(HeapUsed) * 0x10000) / HeapTotal);

					dword KernelTime = 0;
					dword HeapColor = 0xFF20A0A0;

					if (HeapUsage > 0x9999)
						HeapColor = 0xFFF8BF16;
					if (HeapUsage > 0xB333)
						HeapColor = 0xFFFA5914;

					DrawRect(m_SurfaceID, m_Margin + 74, m_Margin, 48, m_FontH * 3, m_BGColor);
					OutPercent(m_Margin + 74, m_Margin + m_FontH * 0, PageUsage, 0xFF20A0A0, true);
					OutPercent(m_Margin + 74, m_Margin + m_FontH * 1, HeapUsage, HeapColor, true);
					OutPercent(m_Margin + 74, m_Margin + m_FontH * 2, m_KernelTime, 0xFF20A0A0, true);

					dword PrevVP = m_VisualProgressIndex;

					m_VisualProgressIndex++;
					if (m_VisualProgressIndex >= 6)
						m_VisualProgressIndex = 0;

					dword C1 = 0xFFB0B8FF;
					dword C2 = 0xFFD0D4FF;
					if (m_VisualProgressIndex == 0)
					{
						C1 = 0xFFFFA393;
						C2 = 0xFFFFCCC4;
					}
					DrawRect(m_SurfaceID, 140 + PrevVP * 12,
						m_FontH + m_Margin + 4, 8, 8, m_BGColor);
					DrawRect(m_SurfaceID, 140 + m_VisualProgressIndex * 12,
						m_FontH + m_Margin + 4, 8, 8, C1);
					DrawRect(m_SurfaceID, 140 + m_VisualProgressIndex * 12 + 1,
						m_FontH + m_Margin + 4 + 1, 6, 6, C2);
				}

				if (m_TickCount == 0)
				{
					ShowSurface(m_SurfaceID);
				}

				m_TickCount++;
			}
			else if (N.GetID() == NfKe_TerminateProcess)
			{
				return;
			}
		}
	}

	void OutDecimal(dword RightX, dword Y, dword Val, dword Color)
	{
		dword TVal = Val;
		char Buf[11] = "         0";
		dword Ofs = 9;

		Buf[9] += TVal % 10;
		TVal /= 10;
		while (TVal != 0)
		{
			Buf[Ofs - 1] = '0' + TVal % 10;
			TVal /= 10;
			Ofs--;
		}
		OutText(m_SurfaceID, RightX - m_DigitW * (10 - Ofs), Y, Color, Buf + Ofs);
	}

	void OutPercent(dword X, dword Y, dword PercentHex, dword Color, bool UsePercentSign = false)
	{
		dword PercentDec = (PercentHex * 10000) / 65536;
		char Buf[8] = "  0.00%";
		if (!UsePercentSign)
			Buf[6] = 0;
		dword Ofs = 2;
		
		Buf[5] += PercentDec % 10;
		PercentDec /= 10;
		Buf[4] += PercentDec % 10;
		PercentDec /= 10;
		Buf[2] += PercentDec % 10;
		PercentDec /= 10;
		if (PercentDec != 0)
		{
			Buf[1] = '0' + PercentDec % 10;
			PercentDec /= 10;
			Ofs = 1;
		}
		if (PercentDec != 0)
		{
			Buf[0] = '0' + PercentDec % 10;
			Ofs = 0;
		}
		OutText(m_SurfaceID, X + m_DigitW * Ofs, Y, Color, Buf + Ofs);
	}

private:
	dword m_X;
	dword m_Y;
	dword m_Width;
	dword m_Height;

	dword m_DigitW;
	dword m_TickCount;
	dword m_SurfaceID;
	dword m_VisualProgressIndex;

	dword m_KernelTime;

	static const dword m_BGColor = 0xFFFFFFFA;
	static const dword m_Margin = 2;
	static const dword m_FontH = 14;
	static const dword m_VisibleProcCount = 30;

	CArray<CProcData> m_ProcList;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_ProcInfo))
		return;

	CProcInfo PI;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



		/*
		dword VisualProgress = 6;

		KeWaitForSymbol(Sm_InitStage2);
		KeEnableNotification(NfKe_TerminateProcess);
		bool IsShowed = false;
		
		dword Width = 200;
		dword Height = 30 * 14;

		dword SurfaceID = CreateSurface(20, 20, Width, Height, 1);
		FillSurfaceRect(SurfaceID, 0, 0, Width, 42, 0xFFFFFFFA);

		OutText(SurfaceID, 1, 0, 0xFFA020A0, "Page Usage:");
		OutText(SurfaceID, 1, 14, 0xFFA020A0, "Heap Usage:");
		OutText(SurfaceID, 1, 28, 0xFFA020A0, "Kernel Time:");

		dword ProcPIDs[32+1];
		char IDText[5];
		IDText[4] = 0;
		for (;;)
		{
			KeGetProcessList(ProcPIDs, 32+1);
			dword ProcessCount = ProcPIDs[0];

			bool IsSorted = false;
			while (!IsSorted)
			{
				IsSorted = true;
				for (dword i = 0; i < ProcessCount-1; i++)
					if (ProcPIDs[i+1] > ProcPIDs[i+2])
					{
						dword T = ProcPIDs[i+1];
						ProcPIDs[i+1] = ProcPIDs[i+2];
						ProcPIDs[i+2] = T;
						IsSorted = false;
					}
			}

			Processes.Clear();
			for (dword i = 0; i < ProcessCount; i++)
			{
				CProcInfo PI;
				PI.m_PID = ProcPIDs[i + 1];

				char Name[32];
				KeGetProcessInfo(PI.m_PID, PI.m_PageCount, PI.m_User,
					PI.m_Kernel, PI.m_NFCount, Name);
				PI.m_Name = CStringA(Name);

				Processes.PushBack(PI);
			}

			FillSurfaceRect(SurfaceID, 0, 42, Width, Height - 42, 0xFFFFFFFA);

			for (dword i = 0; i < ProcessCount; i++)
			{
				IDText[2] = 0;
				ByteToString(Processes[i].m_PID & 0xFF, IDText);
				OutText(SurfaceID, 1, 42+i*14, 0xFF2020A0, IDText);
				WordToString(Processes[i].m_PageCount, IDText);
				OutText(SurfaceID, 1+16, 42+i*14, 0xFF20A020, IDText);
				WordToString(Processes[i].m_NFCount, IDText);
				OutText(SurfaceID, 1+16+28, 42+i*14, 0xFFA0A020, IDText+1);
				IDText[2] = 0;
				ByteToString(Processes[i].m_User, IDText);
				OutText(SurfaceID, 1+32+12+22, 42+i*14, 0xFF20A020, IDText);
				IDText[2] = 0;
				ByteToString(Processes[i].m_Kernel, IDText);
				OutText(SurfaceID, 1+32+16+12+22, 42+i*14, 0xFF20A020, IDText);
				OutText(SurfaceID, 1+32+10+32+22, 42+i*14, 0xFFA02020, Processes[i].m_Name._ptr());
			}

			for (int i = 0; i < 6; i++)
			{
				dword PageUsage = 0;
				dword HeapUsage = 0;
				dword KernelTime = 0;
				KeGetMemInfo(PageUsage, HeapUsage, KernelTime);

				FillSurfaceRect(SurfaceID, 64+16, 0, 32, 42, 0xFFFFFFFA);

				WordToString(PageUsage, IDText);
				OutText(SurfaceID, 64+16, 0, 0xFF20A0A0, IDText);
				WordToString(HeapUsage, IDText);
				OutText(SurfaceID, 64+16, 14, 0xFF20A0A0, IDText);
				WordToString(KernelTime, IDText);
				OutText(SurfaceID, 64+16, 28, 0xFF20A0A0, IDText);

				dword PrevVP = VisualProgress;

				VisualProgress++;
				if (VisualProgress >= 6)
					VisualProgress = 0;

				FillSurfaceRect(SurfaceID, 128 + PrevVP * 8, 18, 8, 8, 0xFFFFFFFA);
				FillSurfaceRect(SurfaceID, 128 + VisualProgress * 8, 18, 8, 8, 0xFFB0B8FF);
				FillSurfaceRect(SurfaceID, 128 + VisualProgress * 8 + 1, 18+1, 6, 6, 0xFFD0D4FF);

				if (!IsShowed)
				{
					ShowSurface(SurfaceID);
					IsShowed = true;
				}

				for (int j = 0; j < 6; j++)
				{
					KeWaitTicks(4);
					if (KeGetNotificationCount())
						return;
				}
			}
		}
		*/
