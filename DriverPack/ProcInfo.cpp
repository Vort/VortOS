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
		m_SurfaceActivated = true;

		m_X = 20;
		m_Y = 20;

		m_Width = 236;
		m_Height = m_Margin * 4 + (m_VisibleProcCount + 4) * m_FontH;

		char Digit = '0';
		KeRequestCall(ClFont_GetTextWidth, PB(&Digit), 1, PB(&m_DigitW), 4);

		m_SurfaceID = CreateSurface(m_X, m_Y, m_Width, m_Height);
		DrawActiveBorder();
		DrawRect(m_SurfaceID, 1, 1, m_Width - 2, m_Margin * 3 + m_FontH * 4 - 1, m_BGColor);
		OutText(m_SurfaceID, m_Margin, m_Margin + m_FontH * 0, 0xFFA020A0, "Page Usage:");
		OutText(m_SurfaceID, m_Margin, m_Margin + m_FontH * 1, 0xFFA020A0, "Heap Usage:");
		OutText(m_SurfaceID, m_Margin, m_Margin + m_FontH * 2, 0xFFA020A0, "Kernel Time:");

		OutText(m_SurfaceID, m_Margin+4, m_Margin*2 + m_FontH * 3, 0xFF2020A0, "ID");
		OutText(m_SurfaceID, m_Margin+18, m_Margin*2 + m_FontH * 3, 0xFF20A020, "Mem");
		OutText(m_SurfaceID, m_Margin+52, m_Margin*2 + m_FontH * 3, 0xFFA0A020, "Nf");
		OutText(m_SurfaceID, m_Margin+74, m_Margin*2 + m_FontH * 3, 0xFF20A0A0, "Usr");
		OutText(m_SurfaceID, m_Margin+104, m_Margin*2 + m_FontH * 3, 0xFF20A0A0, "Krn");
		OutText(m_SurfaceID, m_Margin+128, m_Margin*2 + m_FontH * 3, 0xFFA02020, "Name");

		KeEnableNotification(NfKe_TimerTick);
		KeEnableNotification(Nf_SurfaceActivated);
		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<4> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();

			if (N.GetID() == Nf_SurfaceActivated)
			{
				dword activatedSurfaceID = N.GetDword(0);

				if (activatedSurfaceID == m_SurfaceID)
				{
					if (!m_SurfaceActivated)
					{
						m_SurfaceActivated = true;
						DrawActiveBorder();
					}
				}
				else
				{
					if (m_SurfaceActivated)
					{
						m_SurfaceActivated = false;
						DrawActiveBorder();
					}
				}
			}
			else if (N.GetID() == NfKe_TimerTick)
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


					DrawRect(m_SurfaceID, 1, m_Margin * 3 + m_FontH * 4,
						m_Width - 2, m_Height - (m_Margin * 3 + m_FontH * 4) - 1, m_BGColor);

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

	void DrawActiveBorder()
	{
		DrawFrameRect(m_SurfaceID, 0, 0, m_Width, m_Height,
			m_SurfaceActivated ? 0xFF5E8AFF : 0xFFAAAAAA);
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
	bool m_SurfaceActivated;
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