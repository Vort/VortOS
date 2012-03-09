// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Benchmark.cpp
#include "API.h"
#include "Random.h"
#include "NumericConverter.h"

extern "C" double sin(double x);
extern "C" double cos(double x);
#pragma intrinsic(sin, cos)

// ----------------------------------------------------------------------------
extern "C" int _fltused = 0;

// ----------------------------------------------------------------------------
class CBenchmark
{
public:
	CBenchmark()
	{
		for (dword i = 0; i < 4; i++)
			m_PerfData[i] = 0;

		m_SurfID = CreateSurface(300, 200, 256, 256 + 16);
		FillSurface(m_SurfID, 0xFFFFFFFF);
		DrawRect(m_SurfID, 0, 0, 256, 1, 0xFF808080);
		DrawRect(m_SurfID, 0, 0, 1, 256+16, 0xFF808080);
		DrawRect(m_SurfID, 0, 256+16-1, 256, 1, 0xFF808080);
		DrawRect(m_SurfID, 256-1, 0, 1, 256+16, 0xFF808080);
		ShowSurface(m_SurfID);

		KeEnableNotification(Nf_VirtualKey);
		KeEnableNotification(NfKe_TerminateProcess);

		double T = 0.0;
		dword FPS = 0;
		dword T1 = KeGetTime();
		for (;;)
		{
			for (dword i = 0; i < 6; i++)
				DrawIter(T - 4.0 * (5 - i), 0xFFFFFFFF);

			T += 1.0;
			for (dword i = 0; i < 6; i++)
				DrawIter(T - 4.0 * (5 - i), 0x004070FF | (((i + 1) * 0x10) << 24));

			WaitRedraw();
			FPS++;

			dword T2 = KeGetTime();
			if (T2 - T1 >= 1000)
			{
				dword AvgFPS = AddPerfData(FPS);
				T1 = T2;
				FPS = 0;

				ShowStat(AvgFPS);
			}


			CNotification<4> Nf;
			dword NfCount = KeGetNotificationCount();
			for (dword i = 0; i < NfCount; i++)
			{
				Nf.Recv();
				if (Nf.GetID() == Nf_VirtualKey)
				{
					if (Nf.GetByte(0) == VK_Esc)
						return;
				}
				else if (Nf.GetID() == NfKe_TerminateProcess)
					return;
			}
		}
	}

	void ShowStat(dword AvgFPS)
	{
		char Buf[64];
		dword L = CNumericConverter().DwordToString(AvgFPS, Buf);
		DrawRect(m_SurfID, 1, 1, 256-2, 16-1, 0xFFFFFFFF);
		OutText(m_SurfID, 2, 1, 0xFF000000, Buf, L);
	}

	void DrawIter(double T, dword Color)
	{
		double S = 64.0;
		double A = T * 3.1415 / 180.0;
		double X1 = 128.0 + 92.0 * cos(A);
		double Y1 = 128.0 + 16.0 + 92.0 * sin(A);
		double X2 = 128.0 - 92.0 * cos(A);
		double Y2 = 128.0 + 16.0 - 92.0 * sin(A);
		DrawRect(m_SurfID, X1 - S * 0.5, Y1 - S * 0.5, S, S, Color);
		DrawRect(m_SurfID, X2 - S * 0.5, Y2 - S * 0.5, S, S, Color);
	}

	dword AddPerfData(dword FPS)
	{
		m_PerfData[3] = m_PerfData[2];
		m_PerfData[2] = m_PerfData[1];
		m_PerfData[1] = m_PerfData[0];
		m_PerfData[0] = FPS;
		return (m_PerfData[0] + m_PerfData[1] + m_PerfData[2] + m_PerfData[3]) / 4;
	}

private:
	dword m_SurfID;
	dword m_PerfData[4];
};

// ----------------------------------------------------------------------------
void Entry()
{
	CBenchmark B;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=