// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PCI.cpp
#include "API.h"
#include "Array2.h"

// ----------------------------------------------------------------------------
class CPCIDevice
{
public:
	CPCIDevice(dword Bus, dword Device, dword Function)
	{
		m_Bus = Bus;
		m_Device = Device;
		m_Function = Function;

		dword VendorDevice = ReadConfDword(0x00);

		m_VendorID = VendorDevice & 0xFFFF;
		m_DeviceID = VendorDevice >> 16;
	}

	CPCIDevice()
	{
		m_Bus = 0;
		m_Device = 0;
		m_Function = 0;

		m_VendorID = 0xFFFF;
		m_DeviceID = 0xFFFF;
	}

	bool IsPresent()
	{
		return !HasID(0xFFFF, 0xFFFF);
	}

	bool HasID(word VendorID, word DeviceID)
	{
		return
			(m_VendorID == VendorID) &&
			(m_DeviceID == DeviceID);
	}

	bool HasBusDevFunc(byte BusNumber, byte DevNumber, byte FuncNumber)
	{
		return
			(m_Bus == BusNumber) &&
			(m_Device == DevNumber) &&
			(m_Function == FuncNumber);
	}

	byte GetBusNumber()
	{
		return m_Bus;
	}

	byte GetDevNumber()
	{
		return m_Device;
	}

	byte GetFuncNumber()
	{
		return m_Function;
	}

	word GetVendorID()
	{
		return m_VendorID;
	}

	word GetDeviceID()
	{
		return m_DeviceID;
	}

	byte ReadConfByte(byte regOffset)
	{
		KeOutPortDword(0xCF8, MakeConfAddr(m_Bus, m_Device, m_Function, regOffset));
		return KeInPortByte(0xCFC);
	}

	word ReadConfWord(byte regOffset)
	{
		KeOutPortDword(0xCF8, MakeConfAddr(m_Bus, m_Device, m_Function, regOffset));
		return KeInPortWord(0xCFC);
	}

	dword ReadConfDword(byte regOffset)
	{
		KeOutPortDword(0xCF8, MakeConfAddr(m_Bus, m_Device, m_Function, regOffset));
		return KeInPortDword(0xCFC);
	}

	void WriteConfByte(byte regOffset, byte val)
	{
		KeOutPortDword(0xCF8, MakeConfAddr(m_Bus, m_Device, m_Function, regOffset));
		KeOutPortByte(0xCFC, val);
	}

	void WriteConfWord(byte regOffset, word val)
	{
		KeOutPortDword(0xCF8, MakeConfAddr(m_Bus, m_Device, m_Function, regOffset));
		KeOutPortWord(0xCFC, val);
	}

	void WriteConfDword(byte regOffset, dword val)
	{
		KeOutPortDword(0xCF8, MakeConfAddr(m_Bus, m_Device, m_Function, regOffset));
		KeOutPortDword(0xCFC, val);
	}

private:
	dword MakeConfAddr(byte busNumber, byte devNumber, byte funcNumber, byte regOffset)
	{
		dword R = 0x80000000;
		R |= busNumber << 16;
		R |= (devNumber & 0x1F) << 11;
		R |= (funcNumber & 0x7) << 8;
		R |= regOffset;
		return R;
	}

private:
	dword m_Bus;
	dword m_Device;
	dword m_Function;

	word m_VendorID;
	word m_DeviceID;
};

// ----------------------------------------------------------------------------
class CPCI
{
public:
	CPCI()
	{
		for (dword b = 0; b < 256; b++)
			for (dword d = 0; d < 32; d++)
			{
				CPCIDevice D(b, d, 0);
				if (D.IsPresent())
				{
					m_Devices.PushBack(D);
					for (dword f = 1; f < 8; f++)
					{
						CPCIDevice DD(b, d, f);
						if (DD.IsPresent())
							m_Devices.PushBack(DD);
					}
				}
			}

		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(NfPCI_WriteConfByte);
		KeEnableNotification(NfPCI_WriteConfWord);
		KeEnableNotification(NfPCI_WriteConfDword);
		KeEnableCallRequest(ClPCI_GetDeviceByID);
		KeEnableCallRequest(ClPCI_ReadConfByte);
		KeEnableCallRequest(ClPCI_ReadConfWord);
		KeEnableCallRequest(ClPCI_ReadConfDword);
		KeEnableCallRequest(ClPCI_GetDeviceCount);
		KeEnableCallRequest(ClPCI_GetDeviceInfoByIndex);
		KeSetSymbol(SmPCI_Ready);
		
		CCallRequest<0x10> CR;
		CNotification<8> N;
		for (;;)
		{
			KeWaitFor(3);
			dword NfCount;
			dword CallCount;
			KeGetNfClCount(NfCount, CallCount);

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfKe_TerminateProcess)
				{
					return;
				}
				else if (
					(N.GetID() == NfPCI_WriteConfByte) ||
					(N.GetID() == NfPCI_WriteConfWord) ||
					(N.GetID() == NfPCI_WriteConfDword))
				{
					CPCIDevice D = GetDevice(N.GetByte(0), N.GetByte(1), N.GetByte(2));
					if (D.IsPresent())
					{
						if (N.GetID() == NfPCI_WriteConfByte)
							D.WriteConfByte(N.GetByte(3), *(byte*)(N.GetBuf() + 4));
						else if (N.GetID() == NfPCI_WriteConfWord)
							D.WriteConfWord(N.GetByte(3), *(word*)(N.GetBuf() + 4));
						else if (N.GetID() == NfPCI_WriteConfDword)
							D.WriteConfDword(N.GetByte(3), *(dword*)(N.GetBuf() + 4));
					}
				}
			}
			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClPCI_GetDeviceByID)
				{
					byte LocBuf[3];
					CPCIDevice D = GetDevice(CR.GetDword(0), CR.GetDword(1));
					if (D.IsPresent())
					{
						LocBuf[0] = D.GetBusNumber();  
						LocBuf[1] = D.GetDevNumber();  
						LocBuf[2] = D.GetFuncNumber();  
						CR.Respond(LocBuf, 3);
					}
					else
						CR.Respond();
				}
				else if (
					(CR.GetTypeID() == ClPCI_ReadConfByte) ||
					(CR.GetTypeID() == ClPCI_ReadConfWord) ||
					(CR.GetTypeID() == ClPCI_ReadConfDword))
				{
					CPCIDevice D = GetDevice(CR.GetByte(0), CR.GetByte(1), CR.GetByte(2));
					if (D.IsPresent())
					{
						if (CR.GetTypeID() == ClPCI_ReadConfByte)
						{
							byte r = D.ReadConfByte(CR.GetByte(3));
							CR.Respond((byte*)&r, 1);
						}
						else if (CR.GetTypeID() == ClPCI_ReadConfWord)
						{
							word r = D.ReadConfWord(CR.GetByte(3));
							CR.Respond((byte*)&r, 2);
						}
						else if (CR.GetTypeID() == ClPCI_ReadConfDword)
						{
							dword r = D.ReadConfDword(CR.GetByte(3));
							CR.Respond((byte*)&r, 4);
						}
					}
					else
					{
						CR.Respond();
					}
				}
				else if (CR.GetTypeID() == ClPCI_GetDeviceCount)
				{
					dword R = m_Devices.Size();
					CR.Respond(R);
				}
				else if (CR.GetTypeID() == ClPCI_GetDeviceInfoByIndex)
				{
					byte RetBuf[7] = {0};
					dword Index = CR.GetDword(0);
					if (Index < m_Devices.Size())
					{
						(PW(RetBuf))[0] = m_Devices[Index].GetVendorID();
						(PW(RetBuf))[1] = m_Devices[Index].GetDeviceID();
						RetBuf[4] = m_Devices[Index].GetBusNumber();
						RetBuf[5] = m_Devices[Index].GetDevNumber();
						RetBuf[6] = m_Devices[Index].GetFuncNumber();
					}
					CR.Respond(RetBuf, 7);
				}
			}
		}
	}

	CPCIDevice GetDevice(byte BusNumber, byte DevNumber, byte FuncNumber)
	{
		for (dword i = 0; i < m_Devices.Size(); i++)
			if (m_Devices[i].HasBusDevFunc(BusNumber, DevNumber, FuncNumber))
				return m_Devices[i];
		return CPCIDevice();
	}

	CPCIDevice GetDevice(word VendorID, word DeviceID)
	{
		for (dword i = 0; i < m_Devices.Size(); i++)
			if (m_Devices[i].HasID(VendorID, DeviceID))
				return m_Devices[i];
		return CPCIDevice();
	}

private:
	CArray<CPCIDevice> m_Devices;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_PCI))
		return;

	CPCI P;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=