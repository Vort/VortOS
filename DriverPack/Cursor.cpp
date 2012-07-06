// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Cursor.cpp
#include "API.h"
#include "CursorTexture.h"

// ----------------------------------------------------------------------------
extern "C" int _fltused = 0;

// ----------------------------------------------------------------------------
class CCursor
{
public:
	CCursor()
	{
		KeWaitForSymbol(SmRenderer_Ready);

		dword Width = 32;
		dword Height = 32;
		GetResolution(Width, Height);

		m_Click1_X = -2;
		m_Click1_Y = -2;
		m_Click1_T = -2;
		m_Click2_X = -1;
		m_Click2_Y = -1;
		m_Click2_T = -1;

		m_MouseDownX = 0;
		m_MouseDownY = 0;
		m_MaxCursorX = Width - 1;
		m_MaxCursorY = Height - 1;
		m_CursorX = m_MaxCursorX / 2;
		m_CursorY = m_MaxCursorY / 2;

		KeWaitForSymbol(SmSurfMgr_Ready);
		m_SurfaceID = CreateSurface(m_CursorX, m_CursorY, 11, 18, 2);

		dword SMID = KeAllocSharedMem(11*18*4);
		byte* SMTexData = KeMapSharedMem(SMID);
		for (dword i = 0; i < 11*18*4; i++)
			SMTexData[i] = CursorTexture[i];
		SetSurfaceData(m_SurfaceID, SMID, 11*18*4);
		KeReleaseSharedMem(SMID);

		ShowSurface(m_SurfaceID);
		WaitRedraw();
		KeSetSymbol(SmCursor_Ready);
		KeWaitForSymbol(SmDesktop_Ready);

		KeEnableNotification(Nf_MouseDeltaMove);
		KeEnableNotification(Nf_MouseAbsMove);
		KeEnableNotification(Nf_MouseButtonDown);
		KeEnableNotification(Nf_MouseButtonUp);
		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<8> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();

			if (N.GetID() == Nf_MouseDeltaMove)
			{
				int DeltaX = N.GetDword(0);
				int DeltaY = N.GetDword(1);

				m_CursorX += DeltaX;
				m_CursorY += DeltaY;

				ProcessMovement();
			}
			if (N.GetID() == Nf_MouseAbsMove)
			{
				float x = *(float*)(N.GetBuf() + 0);
				float y = *(float*)(N.GetBuf() + 4);

				m_CursorX = (dword)(x * m_MaxCursorX);
				m_CursorY = (dword)(y * m_MaxCursorY);

				ProcessMovement();
			}
			else if (N.GetID() == Nf_MouseButtonDown)
			{
				if (N.GetByte(0) == 0)
				{
					m_MouseDownX = m_CursorX;
					m_MouseDownY = m_CursorY;
				}
			}
			else if (N.GetID() == Nf_MouseButtonUp)
			{
				if (N.GetByte(0) == 0)
					if (m_MouseDownX == m_CursorX)
						if (m_MouseDownY == m_CursorY)
							OnClick();
			}
			else if (N.GetID() == NfKe_TerminateProcess)
			{
				WaitRedraw();
				KeSetSymbol(SmCursor_Terminated);
				return;
			}
		}
	}

	void ProcessMovement()
	{
		if (m_CursorX < 0)
			m_CursorX = 0;
		if (m_CursorY < 0)
			m_CursorY = 0;
		if (m_CursorX > m_MaxCursorX)
			m_CursorX = m_MaxCursorX;
		if (m_CursorY > m_MaxCursorY)
			m_CursorY = m_MaxCursorY;

		byte NfBuf[8];
		*PD(&NfBuf[0]) = m_CursorX;
		*PD(&NfBuf[4]) = m_CursorY;
		KeNotify(Nf_CursorMoveTo, NfBuf, 8);
		MoveSurface(m_SurfaceID, m_CursorX, m_CursorY);
	}

	void OnClick()
	{
		m_Click2_X = m_Click1_X;
		m_Click2_Y = m_Click1_Y;
		m_Click2_T = m_Click1_T;
		m_Click1_X = m_CursorX;
		m_Click1_Y = m_CursorY;
		m_Click1_T = KeGetTime();

		KeNotify(Nf_MouseClick, null, null);
		if ((m_Click1_T - m_Click2_T) <= 400)
			if (m_Click1_X == m_Click2_X)
				if (m_Click1_Y == m_Click2_Y)
				{
					KeNotify(Nf_MouseDoubleClick, null, null);
					m_Click1_X = -2;
					m_Click1_Y = -2;
					m_Click1_T = -2;
					m_Click2_X = -1;
					m_Click2_Y = -1;
					m_Click2_T = -1;
				}
	}

	int m_Click1_X;
	int m_Click1_Y;
	dword m_Click1_T;
	int m_Click2_X;
	int m_Click2_Y;
	dword m_Click2_T;


	dword m_MaxCursorX;
	dword m_MaxCursorY;
	int m_CursorX;
	int m_CursorY;

	int m_MouseDownX;
	int m_MouseDownY;

	dword m_SurfaceID;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Cursor))
		return;
	CCursor C;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
