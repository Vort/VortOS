// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PS2Keyb.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CPS2Keyb
{
public:
	CPS2Keyb()
	{
		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(Nfi8043_KeyboardData);

		m_Stage = 0;
		bool IsFinalStage;

		CNotification<1> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();
			if (N.GetID() == Nfi8043_KeyboardData)
			{
				byte Param = N.GetByte(0);

				IsFinalStage = false;
				bool IsPressed = (Param & 0x80) == 0;
				byte LoParam = Param & ~0x80;

				if ((m_Stage == 12) && (!IsFinalStage))
				{
					// E1 1D
					IsFinalStage = true;
					if      (LoParam == 0x45) OnVK(VK_Pause, IsPressed);
					else IsFinalStage = false;
					m_Stage = 0;
				}
				if ((m_Stage == 11) && (!IsFinalStage))
				{
					// E1
					IsFinalStage = true;
					if      (LoParam == 0x1D) m_Stage = 12;
					else
					{
						IsFinalStage = false;
						m_Stage = 0;
					}
				}
				if ((m_Stage == 1) && (!IsFinalStage))
				{
					// E0
					IsFinalStage = true;
					if      (LoParam == 0x1C) OnVK(VK_NumEnter, IsPressed);
					else if (LoParam == 0x1D) OnVK(VK_RightControl, IsPressed);
					else if (LoParam == 0x35) OnVK(VK_NumSlash, IsPressed);
					else if (LoParam == 0x37) OnVK(VK_PrintScreen, IsPressed);
					else if (LoParam == 0x38) OnVK(VK_RightAlt, IsPressed);
					else if (LoParam == 0x47) OnVK(VK_GreyHome, IsPressed);
					else if (LoParam == 0x48) OnVK(VK_GreyUp, IsPressed);
					else if (LoParam == 0x49) OnVK(VK_GreyPageUp, IsPressed);
					else if (LoParam == 0x4B) OnVK(VK_GreyLeft, IsPressed);
					else if (LoParam == 0x4D) OnVK(VK_GreyRight, IsPressed);
					else if (LoParam == 0x4F) OnVK(VK_GreyEnd, IsPressed);
					else if (LoParam == 0x50) OnVK(VK_GreyDown, IsPressed);
					else if (LoParam == 0x51) OnVK(VK_GreyPageDown, IsPressed);
					else if (LoParam == 0x52) OnVK(VK_GreyInsert, IsPressed);
					else if (LoParam == 0x53) OnVK(VK_GreyDelete, IsPressed);
					else if (LoParam == 0x5B) OnVK(VK_LeftWindow, IsPressed);
					else if (LoParam == 0x5C) OnVK(VK_RightWindow, IsPressed);
					else if (LoParam == 0x5D) OnVK(VK_Menu, IsPressed);
					else IsFinalStage = false;
					m_Stage = 0;
				}
				if ((m_Stage == 0) && (!IsFinalStage))
				{
					IsFinalStage = true;
					if (Param == 0xE0) m_Stage = 1;
					if (Param == 0xE1) m_Stage = 11;
					else if ((LoParam >= 0x01) && (LoParam <= 0x53))
					{
						static byte Translate[0x53] =
						{
							VK_Esc, VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8,
							VK_9, VK_0, VK_Minus, VK_Equal, VK_Backspace, VK_Tab,
							VK_Q, VK_W, VK_E, VK_R, VK_T, VK_Y, VK_U, VK_I, VK_O,
							VK_P, VK_LeftBracket, VK_RightBracket, VK_Enter,
							VK_LeftControl, VK_A, VK_S, VK_D, VK_F, VK_G, VK_H, VK_J,
							VK_K, VK_L, VK_Semicolon, VK_Apostrophe, VK_GraveAccent,
							VK_LeftShift, VK_Backslash, VK_Z, VK_X, VK_C, VK_V, VK_B,
							VK_N, VK_M, VK_Comma, VK_Dot, VK_Slash, VK_RightShift,
							VK_NumAsterisk, VK_LeftAlt, VK_Space, VK_CapsLock, VK_F1,
							VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9,
							VK_F10, VK_NumLock, VK_ScrollLock, VK_Num7, VK_Num8,
							VK_Num9, VK_NumMinus, VK_Num4, VK_Num5, VK_Num6,
							VK_NumPlus, VK_Num1, VK_Num2, VK_Num3, VK_Num0, VK_NumDel
						};
						OnVK(EVirtualKey(Translate[LoParam - 1]), IsPressed);
					}
					else if (LoParam == 0x57) OnVK(VK_F11, IsPressed);
					else if (LoParam == 0x58) OnVK(VK_F12, IsPressed);
				}
			}
			else if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}

	void OnVK(EVirtualKey VK, bool IsPressed)
	{
		if (IsPressed)
		{
			byte LEDIndex;
			bool LedSwitch = false;
			if (VK == VK_NumLock)
			{
				LEDIndex = 1;
				LedSwitch = true;
			}
			else if (VK == VK_CapsLock)
			{
				LEDIndex = 2;
				LedSwitch = true;
			}
			else if (VK == VK_ScrollLock)
			{
				LEDIndex = 0;
				LedSwitch = true;
			}

			if (LedSwitch)
			{
				//Console_OutString("[K_LED]");
				KeNotify(NfKeyboard_SwitchLEDStatus, &LEDIndex, 1);
			}
		}

		byte NfData[2];
		NfData[0] = byte(VK);
		NfData[1] = IsPressed?1:0;
		KeNotify(Nf_VirtualKey, NfData, 2);

		/*
		char CS[3] = {0};
		CS[0] = '\'';
		CS[1] = (char)VK;
		Console_OutString("[K_");
		Console_OutByte((byte)VK);
		Console_OutString(CS);
		if (IsPressed)
			Console_OutString("'0]");
		else
			Console_OutString("'1]");
			*/
	}

private:
	dword m_Stage;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Ps2Keyb))
		return;
	CPS2Keyb K;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=