// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// i8042.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class Ci8042
{
public:
	Ci8042()
	{
		m_IsScrollActive = false;
		m_IsNumActive = false;
		m_IsCapsActive = false;

		KeUnmaskIRQ(1);
		KeUnmaskIRQ(12);
		KeEnableNotification(NfKe_IRQ0);
		KeEnableNotification(NfKe_IRQ1);
		KeEnableNotification(NfKe_IRQ12);
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(NfKeyboard_SwitchLEDStatus);

		InitDevices();

		CNotification<16> N;
		for (;;)
		{
			KeWaitFor(1);
			dword NfCount = KeGetNotificationCount();

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfKe_IRQ0)
				{
					m_TimeoutTimer++;
					if (m_TimeoutTimer >= 20)
					{
						KeDisableNotification(NfKe_IRQ0);
						//DebugOut("[MouseInitTimeout]", 18);
						KeInPortByte(0x60);
						m_IsInitFinished = true;
					}
				}
				else if (N.GetID() == NfKe_IRQ1)
				{
					byte D = KeInPortByte(0x60);

					//DebugOut("[fK:", 4);
					//DebugOut(D);
					//DebugOut("]", 1);

					OnKeybByte(D);
					KeEndOfInterrupt(1);
				}
				else if (N.GetID() == NfKe_IRQ12)
				{
					bool M = false;
					if (KeInPortByte(0x64) & 0x20)
						M = true;
					byte D = KeInPortByte(0x60);

					//if (M)
					//	DebugOut("[fM:", 4);
					//else
					//	DebugOut("[fm:", 4);
					//DebugOut(D);
					//DebugOut("]", 1);

					if (M)
						OnMouseByte(D);
					KeEndOfInterrupt(12);
				}
				else if (N.GetID() == NfKeyboard_SwitchLEDStatus)
				{
					SwitchLEDStatus(N.GetByte(0));
				}
				else if (N.GetID() == NfKe_TerminateProcess)
					return;
			}

			if (KeGetNotificationCount() == 0)
			{
				if (m_LEDSwitchState == 1)
				{
					SendKeyboardData(0xED);
					m_LEDSwitchState = 2;
				}
				else if (m_LEDSwitchState == 3)
				{
					SendKeyboardData(m_LEDStatus);
					m_LEDSwitchState = 4;
				}
			}
		}
	}

	void InitDevices()
	{
		m_TimeoutTimer = 0;
		m_LEDSwitchState = 0;
		m_IsInitFinished = false;

		// Write Command Byte
		SendControllerCommand(0x60);
		// For Mouse And Keyboard
		// Enable Interface, Enable Interrupts, Enable XLAT
		SendKeyboardData(0x43);

		// Enable Mouse Data Reporting
		SendMouseCommand(0xF4);
	}

	void OnKeybByte(byte B)
	{
		if (B == 0xFA)
		{
			if (m_LEDSwitchState == 2)
			{
				m_LEDSwitchState = 3;
				return;
			}
			else if (m_LEDSwitchState == 4)
			{
				m_LEDSwitchState = 0;
				return;
			}
		}
		KeNotify(Nfi8043_KeyboardData, &B, 1);
	}

	void OnMouseByte(byte B)
	{
		if (!m_IsInitFinished)
		{
			KeDisableNotification(NfKe_IRQ0);
			m_IsInitFinished = true;
			return;
		}

		KeNotify(Nfi8043_MouseData, &B, 1);
	}

	void SwitchLEDStatus(byte LEDIndex)
	{
		if (LEDIndex == 0)
			m_IsScrollActive = !m_IsScrollActive;
		else if (LEDIndex == 1)
			m_IsNumActive = !m_IsNumActive;
		else if (LEDIndex == 2)
			m_IsCapsActive = !m_IsCapsActive;
		else
			return;

		m_LEDStatus = 
			(m_IsScrollActive << 0) |
			(m_IsNumActive << 1) |
			(m_IsCapsActive << 2);
		if (m_LEDSwitchState == 0)
			m_LEDSwitchState = 1;
	}

	void SendMouseCommand(byte Cmd)
	{
		SendControllerCommand(0xD4);
		SendKeyboardData(Cmd);
	}

	void SendControllerCommand(byte Cmd)
	{
		//DebugOut("[tC:", 4);
		//DebugOut(Cmd);
		WaitForDataWrite();
		KeOutPortByte(0x64, Cmd);
		//DebugOut("]", 1);
	}

	void SendKeyboardData(byte Cmd)
	{
		//DebugOut("[tK:", 4);
		//DebugOut(Cmd);
		WaitForDataWrite();
		KeOutPortByte(0x60, Cmd);
		//DebugOut("]", 1);
	}

	void WaitForDataWrite()
	{
		while (KeInPortByte(0x64) & 2);
	}

private:
	dword m_LEDSwitchState;

	bool m_IsInitFinished;
	dword m_TimeoutTimer;

	bool m_IsScrollActive;
	bool m_IsNumActive;
	bool m_IsCapsActive;

	byte m_LEDStatus;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_i8042))
		return;

	Ci8042 i8042;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=