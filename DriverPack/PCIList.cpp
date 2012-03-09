// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// PCIList.cpp
#include "API.h"
#include "Array2.h"

// ----------------------------------------------------------------------------
class CDevInfo
{
public:
	CDevInfo(word VendorID, word DeviceID, byte Bus, byte Device, byte Function)
	{
		m_VendorID = VendorID;
		m_DeviceID = DeviceID;

		m_Bus = Bus;
		m_Device = Device;
		m_Function = Function;
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

private:
	byte m_Bus;
	byte m_Device;
	byte m_Function;

	word m_VendorID;
	word m_DeviceID;
};

// ----------------------------------------------------------------------------
class CPCIList
{
public:
	CPCIList()
	{
		KeWaitForSymbol(SmPCI_Ready);

		dword DevCount = 0;
		KeRequestCall(ClPCI_GetDeviceCount, null, 0, PB(&DevCount), 4);

		byte InfBuf[7] = {0};
		for (dword i = 0; i < DevCount; i++)
		{
			KeRequestCall(ClPCI_GetDeviceInfoByIndex, PB(&i), 4, InfBuf, 7);
			m_Devices.PushBack(CDevInfo((PW(InfBuf))[0], (PW(InfBuf))[1],
				InfBuf[4], InfBuf[5], InfBuf[6]));
		}

		m_SurfaceID = CreateSurface(520, 50, 112, 400);
		FillSurface(m_SurfaceID, 0xFFEEFFEE);

		char CB1[3] = {0};
		char CB2[5] = {0};
		for (dword i = 0; i < DevCount; i++)
		{
			ByteToString(m_Devices[i].GetBusNumber(), CB1);
			OutText(m_SurfaceID, 2, 2 + i*14, 0xFF000000, CB1);
			ByteToString(m_Devices[i].GetDevNumber(), CB1);
			OutText(m_SurfaceID, 2+16, 2 + i*14, 0xFF000000, CB1);
			ByteToString(m_Devices[i].GetFuncNumber(), CB1);
			OutText(m_SurfaceID, 2+32, 2 + i*14, 0xFF000000, CB1);
			WordToString(m_Devices[i].GetVendorID(), CB2);
			OutText(m_SurfaceID, 2+50, 2 + i*14, 0xFF000000, CB2);
			WordToString(m_Devices[i].GetDeviceID(), CB2);
			OutText(m_SurfaceID, 2+80, 2 + i*14, 0xFF000000, CB2);
		}

		ShowSurface(m_SurfaceID);

		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<4> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();

			if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}

private:
	CArray<CDevInfo> m_Devices;
	dword m_SurfaceID;
};

// ----------------------------------------------------------------------------
void Entry()
{
	CPCIList P;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=