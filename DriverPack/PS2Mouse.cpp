// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PS2Mouse.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CPS2Mouse
{
public:
	CPS2Mouse()
	{
		buttonPressed[0] = false;
		buttonPressed[1] = false;

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

					bool buttonPressedNew[2] =
					{
						TestBit(m_Flags, 0),
						TestBit(m_Flags, 1)
					};

					if ((DeltaX != 0) || (DeltaY != 0))
					{
						int moveData[2] = {DeltaX, DeltaY};
						KeNotify(Nf_MouseDeltaMove, (byte*)moveData, 8);
					}

					for (int i = 0; i < 2; i++)
					{
						if (buttonPressed[i] != buttonPressedNew[i])
						{
							if (buttonPressedNew[i])
								KeNotify(Nf_MouseButtonDown, (byte*)(&i), 1);
							else
								KeNotify(Nf_MouseButtonUp, (byte*)(&i), 1);
							buttonPressed[i] = buttonPressedNew[i];
						}
					}
				}
			}
			else if (N.GetID() == NfKe_TerminateProcess)
			{
				return;
			}
		}
	}

private:
	dword m_DataIndex;
	byte m_Flags;
	byte m_DeltaX;
	byte m_DeltaY;

	bool buttonPressed[2];
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Ps2Mouse))
		return;
	CPS2Mouse M;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=