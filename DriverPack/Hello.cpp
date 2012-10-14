// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Hello.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CHello
{
public:
	CHello()
	{
		m_SurfX = 150;
		m_SurfY = 100;
		m_SurfW = 240;
		m_SurfH = 180;

		m_IsLocked = false;

		char* HelloText1 = "Hello, world!";
		wchar_t* HelloText2 = L"Привет, мир!";
		dword SurfaceID = CreateSurface(m_SurfX, m_SurfY, m_SurfW, m_SurfH);
		FillSurface(SurfaceID, 0xB0E0E0FF);
		OutText(SurfaceID, 0, 0, 0xFF0000FF, HelloText1);
		OutText(SurfaceID, 0, 14, 0xFF0000FF, HelloText2);

		ShowSurface(SurfaceID);

		KeEnableNotification(Nf_CursorMoveTo);
		KeEnableNotification(Nf_MouseButtonDown);
		KeEnableNotification(Nf_MouseButtonUp);
		KeEnableNotification(Nf_VirtualKey);
		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<8> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();

			if (N.GetID() == Nf_CursorMoveTo)
			{
				m_CursorX = N.GetDword(0);
				m_CursorY = N.GetDword(1);
				if (m_IsLocked)
				{
					m_SurfX = m_CursorX - m_LockDX;
					m_SurfY = m_CursorY - m_LockDY;
					MoveSurface(SurfaceID, m_SurfX, m_SurfY);
				}
			}
			else if (N.GetID() == Nf_VirtualKey)
			{
				if (N.GetByte(0) == VK_F)
				{
					FillSurface(SurfaceID, 0xFFEEEEFF);
					OutText(SurfaceID, 0, 0, 0xFF0000FF, HelloText1);
					OutText(SurfaceID, 0, 14, 0xFF0000FF, HelloText2);
				}
			}
			else if (N.GetID() == Nf_MouseButtonDown)
			{
				if (IsCursorInSurf())
				{
					if (N.GetByte(0) == 0)
					{
						m_LockDX = m_CursorX - m_SurfX;
						m_LockDY = m_CursorY - m_SurfY;
						m_IsLocked = true;
					}
					else if (N.GetByte(0) == 1)
					{
						return;
					}
				}
			}
			else if (N.GetID() == Nf_MouseButtonUp)
				m_IsLocked = false;
			else if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}

	bool IsCursorInSurf()
	{
		if (m_CursorX < m_SurfX)
			return false;
		if (m_CursorY < m_SurfY)
			return false;
		if (m_CursorX > m_SurfX + m_SurfW)
			return false;
		if (m_CursorY > m_SurfY + m_SurfH)
			return false;
		return true;
	}

private:
	int m_CursorX;
	int m_CursorY;

	int m_SurfX;
	int m_SurfY;
	dword m_SurfW;
	dword m_SurfH;

	bool m_IsLocked;
	int m_LockDX;
	int m_LockDY;
};

// ----------------------------------------------------------------------------
void Entry()
{
	KeWaitForSymbol(Sm_InitStage2);
	CHello H;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=