// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PS2Mouse.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CPS2Mouse
{
public:
	CPS2Mouse()
	{
		m_IsLMBPressed = false;
		m_IsRMBPressed = false;

		m_Flags = 0;
		m_DeltaX = 0;
		m_DeltaY = 0;
		m_DataIndex = 0;

		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(Nfi8043_MouseData);

		CNotification<1> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();
			if (N.GetID() == Nfi8043_MouseData)
			{
				byte Param = N.GetByte(0);

				if (m_DataIndex == 0)
				{
					byte Flags = Param;
					if (Flags & 0x8)
					{
						m_Flags = Flags;
						m_DataIndex++;
					}
				}
				else if (m_DataIndex == 1)
				{
					m_DeltaX = Param;
					m_DataIndex++;
				}
				else
				{
					m_DeltaY = Param;
					m_DataIndex = 0;

					int DeltaX = m_DeltaX;
					int DeltaY = m_DeltaY;

					DeltaX |= TestBit(m_Flags, 4) * 0xFFFFFF00;
					DeltaY |= TestBit(m_Flags, 5) * 0xFFFFFF00;
					DeltaY = -DeltaY;
					bool LMB = TestBit(m_Flags, 0);
					bool RMB = TestBit(m_Flags, 1);

					byte NfData[8];
					*PD(&NfData[0]) = DeltaX;
					*PD(&NfData[4]) = DeltaY;

					KeNotify(Nf_MouseDeltaMove, NfData, 8);
					if (LMB != m_IsLMBPressed)
					{
						NfData[0] = 0;
						if (m_IsLMBPressed == false)
							KeNotify(Nf_MouseButtonDown, NfData, 1);
						else
							KeNotify(Nf_MouseButtonUp, NfData, 1);
						m_IsLMBPressed = LMB;
					}
					if (RMB != m_IsRMBPressed)
					{
						NfData[0] = 1;
						if (m_IsRMBPressed == false)
							KeNotify(Nf_MouseButtonDown, NfData, 1);
						else
							KeNotify(Nf_MouseButtonUp, NfData, 1);
						m_IsRMBPressed = RMB;
					}
				}
			}
			else if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}

private:
	dword m_DataIndex;
	byte m_Flags;
	byte m_DeltaX;
	byte m_DeltaY;
	bool m_IsLMBPressed;
	bool m_IsRMBPressed;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Ps2Mouse))
		return;
	CPS2Mouse M;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=