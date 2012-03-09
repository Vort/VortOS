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

		dword VendorDevice = ReadReg(0);

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

	dword ReadReg(dword RegNumber)
	{
		dword R = MakePCIReg(m_Bus, m_Device, m_Function, RegNumber);
		KeOutPortDword(0xCF8, R);
		return KeInPortDword(0xCFC);
	}

private:
	dword MakePCIReg(byte BusNumber, byte DevNumber, byte FuncNumber, byte RegNumber)
	{
		dword R = 0x80000000;
		R |= BusNumber << 16;
		R |= (DevNumber & 0x1F) << 11;
		R |= (FuncNumber & 0x7) << 8;
		R |= RegNumber & 0xFC;
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
		KeEnableCallRequest(ClPCI_GetDeviceByID);
		KeEnableCallRequest(ClPCI_GetDeviceReg);
		KeEnableCallRequest(ClPCI_GetDeviceCount);
		KeEnableCallRequest(ClPCI_GetDeviceInfoByIndex);
		KeSetSymbol(SmPCI_Ready);
		
		CCallRequest<0x10> CR;
		CNotification<4> N;
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
					return;
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
				else if (CR.GetTypeID() == ClPCI_GetDeviceReg)
				{
					CPCIDevice D = GetDevice(CR.GetByte(0), CR.GetByte(1), CR.GetByte(2));
					dword R = 0;
					if (D.IsPresent())
						R = D.ReadReg(CR.GetByte(3));
					CR.Respond(R);
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