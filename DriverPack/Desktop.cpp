// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Desktop.cpp
#include "API.h"
#include "BitOp.h"

// ----------------------------------------------------------------------------
class CDesktop
{
public:
	CDesktop()
	{
		KeWaitForSymbol(SmRenderer_Ready);

		dword Width = 32;
		dword Height = 32;
		GetResolution(Width, Height);

		dword SMID = KeAllocSharedMem(Width*Height*4);
		byte* TexData = KeMapSharedMem(SMID);
		dword Ofs = 0;
		for (dword y = 0; y < Height; y++)
			for (dword x = 0; x < Width; x++)
			{
				byte I = x ^ y;
				TexData[Ofs+0] = I;
				TexData[Ofs+1] = I;
				TexData[Ofs+2] = I;
				TexData[Ofs+3] = 0xFF;
				Ofs += 4;
			}

		KeWaitForSymbol(SmSurfMgr_Ready);
		KeWaitForSymbol(SmCursor_Ready);
		dword SurfaceID = CreateSurface(0, 0, Width, Height, 0);

		SetSurfaceData(SurfaceID, SMID, Width*Height*4);
		KeReleaseSharedMem(SMID);

		ShowSurface(SurfaceID);
		WaitRedraw();
		KeSetSymbol(SmDesktop_Ready);

		m_IsRCtrl = false;
		m_IsLCtrl = false;
		m_IsRAlt = false;
		m_IsLAlt = false;
		m_IsNumDel = false;
		m_IsGreyDel = false;

		m_IsRebootStarted = false;

		KeEnableNotification(Nf_VirtualKey);
		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<8> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();

			if (N.GetID() == Nf_VirtualKey)
			{
				EVirtualKey VK = EVirtualKey(N.GetByte(0));
				byte IsPressed = N.GetByte(1);

				if (VK == VK_RightControl)
					m_IsRCtrl = IsPressed == 1;
				else if (VK == VK_LeftControl)
					m_IsLCtrl = IsPressed == 1;
				else if (VK == VK_RightAlt)
					m_IsRAlt = IsPressed == 1;
				else if (VK == VK_LeftAlt)
					m_IsLAlt = IsPressed == 1;
				else if (VK == VK_NumDel)
					m_IsNumDel = IsPressed == 1;
				else if (VK == VK_GreyDelete)
					m_IsGreyDel = IsPressed == 1;

				if (m_IsRCtrl || m_IsLCtrl)
					if (m_IsRAlt || m_IsLAlt)
						if (m_IsNumDel || m_IsGreyDel)
							Reboot();
			}
			else if (N.GetID() == NfKe_TerminateProcess)
			{
				KeWaitForSymbol(SmCursor_Terminated);
				FillSurface(SurfaceID, 0xFF000000);
				WaitRedraw();
				KeSetSymbol(SmDesktop_Terminated);
				return;
			}
		}
	}

	void Reboot()
	{
		if (!m_IsRebootStarted)
		{
			m_IsRebootStarted = true;
			KeRequestReboot();
		}
	}

private:
	bool m_IsRCtrl;
	bool m_IsLCtrl;
	bool m_IsRAlt;
	bool m_IsLAlt;
	bool m_IsNumDel;
	bool m_IsGreyDel;

	bool m_IsRebootStarted;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Desktop))
		return;
	CDesktop D;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=