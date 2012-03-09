// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// DebugConsole.cpp
#include "API.h"
#include "String2.h"

// ----------------------------------------------------------------------------
class CDebugConsole
{
public:
	CDebugConsole()
	{
		KeWaitForSymbol(Sm_InitStage2);

		m_CursorX = 0;
		m_CursorY = 0;
		m_Width = 400;
		m_Height = 140;

		m_SurfaceID = CreateSurface(230, 330, m_Width, m_Height);
		FillSurface(m_SurfaceID, 0xFF000000);
		ShowSurface(m_SurfaceID);

		KeEnableNotification(Nf_DebugOut);
		KeEnableNotification(Nf_VirtualKey);
		KeEnableNotification(NfKe_ExceptionInfo);
		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<0x100> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();
			if (N.GetID() == Nf_DebugOut)
			{
				CheckOverflow();
				FitOut(PC(N.GetBuf()), N.GetSize());
			}
			else if (N.GetID() == Nf_VirtualKey)
			{
				if (N.GetByte(1) == 1)
				{
					EVirtualKey VK = EVirtualKey(N.GetByte(0));
					if (VK == VK_C)
						Clear();
				}
			}
			else if (N.GetID() == NfKe_ExceptionInfo)
			{
				dword PID = N.GetDword(0);
				dword Exc = N.GetDword(1);
				dword EIP = N.GetDword(2);

				char B1[3] = {0};
				char B2[9] = {0};
				ByteToString(byte(Exc), B1);
				WordToString((EIP >> 16) & 0xFFFF, B2);
				WordToString((EIP >> 0) & 0xFFFF, B2 + 4);
				CStringA S = "[Exception 0x";
				S.Add(B1);
				S.Add(" in '");
				S.Add(CStringA(PC(N.GetBuf()) + 12, N.GetSize() - 12));
				S.Add("' at 0x");
				S.Add(B2);
				S.Add(']');

				FitOut(S._ptr(), S.Len(), 0xFFFA7060);
			}
			else if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}

	void Clear()
	{
		m_CursorX = 0;
		m_CursorY = 0;
		FillSurface(m_SurfaceID, 0xFF000000);
	}

	void CheckOverflow()
	{
		if (m_CursorY >= m_Height)
			Clear();
	}

	void FitOut(char* Str, dword StrSize, dword Color = 0xFFFFFFFF)
	{
		char* StrC = Str;
		dword StrCS = StrSize;
		dword TextW = 0;
		for (;;)
		{
			dword FitW = m_Width - m_CursorX;
			dword FitC = FitText(FitW, StrC, StrCS);

			OutText(m_SurfaceID, m_CursorX, m_CursorY,
				Color, StrC, FitC);
			KeRequestCall(ClFont_GetTextWidth,
				PB(StrC), StrCS, PB(&TextW), 4);
			m_CursorX += TextW;

			if (StrCS == FitC)
				break;
			else
			{
				StrC += FitC;
				StrCS -= FitC;
				m_CursorX = 0;
				m_CursorY += 14;
			}
		}
	}

private:
	dword m_CursorX;
	dword m_CursorY;

	dword m_SurfaceID;

	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_DbgCons))
		return;

	CDebugConsole DC;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=