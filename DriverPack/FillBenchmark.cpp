// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// FillBenchmark.cpp
#include "API.h"
#include "Random.h"

// ----------------------------------------------------------------------------
void OutDecimal(dword SurfaceID, dword RightX, dword Y, dword Val, dword Color)
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
	OutText(SurfaceID, RightX - 5 * (10 - Ofs), Y, Color, Buf + Ofs);
}

// ----------------------------------------------------------------------------
void Entry()
{
	dword RPS[3] = {0};
	dword Ticks = 0;
	dword W = 320;
	dword H = 240;

	dword SurfID = CreateSurface(300, 200, W, H);
	FillSurface(SurfID, 0xFFFFFFFF);
	ShowSurface(SurfID);

	KeEnableNotification(NfKe_TimerTick);
	KeEnableNotification(NfKe_TerminateProcess);
	KeEnableNotification(Nf_VirtualKey);

	dword T;
	CRandom R;
	for (;;)
	{
		dword X1 = R.GetNextWord() % (W - 8) + 4;
		dword X2 = R.GetNextWord() % (W - 8) + 4;
		dword Y1 = R.GetNextWord() % (H - 8) + 4;
		dword Y2 = R.GetNextWord() % (H - 8) + 4;
		dword Color = 
			(R.GetNextWord() & 0xFF) |
			(R.GetNextWord() & 0xFF) << 8 |
			(R.GetNextWord() & 0xFF) << 16 |
			0xFF000000;

		if (X1 > X2)
		{
			T = X1;
			X1 = X2;
			X2 = T;
		}

		if (Y1 > Y2)
		{
			T = Y1;
			Y1 = Y2;
			Y2 = T;
		}

		if ((X1 < 38) && (Y1 < 17))
		{
			X1 = 38;
			Y1 = 17;

			if (X1 > X2)
				X2 = R.GetNextWord() % 100;
			if (Y1 > Y2)
				Y2 = R.GetNextWord() % 100;
		}

		dword RW = X2 - X1;
		dword RH = Y2 - Y1;

		DrawRect(SurfID, X1, Y1, RW, RH, Color);
		RPS[0]++;

		WaitRedraw();

		CNotification<4> Nf;
		dword NfCount = KeGetNotificationCount();
		for (dword i = 0; i < NfCount; i++)
		{
			Nf.Recv();
			if (Nf.GetID() == NfKe_TimerTick)
			{
				Ticks++;

				if (Ticks % 20 == 0)
				{
					dword AvgRPS = (RPS[0] + RPS[1] + RPS[2]) / 3;
					DrawRect(SurfID, 4, 2, 34, 13, 0xFFFFFFFF);
					OutDecimal(SurfID, 32, 3, AvgRPS, 0xFFFFFFFF);
					OutDecimal(SurfID, 31, 2, AvgRPS, 0xFF000000);
					RPS[2] = RPS[1];
					RPS[1] = RPS[0];
					RPS[0] = 0;
				}
			}
			else if (Nf.GetID() == Nf_VirtualKey)
			{
				if (Nf.GetByte(0) == VK_Esc)
					return;
			}
			else if (Nf.GetID() == NfKe_TerminateProcess)
				return;
		}
	}
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=