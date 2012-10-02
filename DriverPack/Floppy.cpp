// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Floppy.cpp
#include "API.h"
#include "Storage.h"

// ----------------------------------------------------------------------------
class CDrive
{
public:
	CDrive()
	{
		m_IsMotorEnabled = false;
		m_MotorIdleTime = 0;
	}

	void ResetMotorIdleTime()
	{
		m_MotorIdleTime = 0;
	}

	void IncrementMotorIdleTime()
	{
		m_MotorIdleTime++;
	}

	dword GetMotorIdleTime()
	{
		return m_MotorIdleTime;
	}

	bool IsMotorEnabled()
	{
		return m_IsMotorEnabled;
	}

	void EnableMotor()
	{
		m_IsMotorEnabled = true;
	}

	void DisableMotor()
	{
		m_IsMotorEnabled = false;
	}

private:
	bool m_IsMotorEnabled;
	dword m_MotorIdleTime;
};

// ----------------------------------------------------------------------------
class CFloppy
{
public:
	CFloppy()
	{
		m_DMABuffer = 0;
		m_SelectedDrive = 0;
		m_CachedDORValue = 0xFF;
		m_IsInResetState = true;

		UpdateDOR(); // Reset Controller

		KeWaitForSymbol(SmDMA_Ready);
		KeWaitForSymbol(SmCache_Ready);
		KeWaitForSymbol(SmFileSystem_Ready);
		KeEnableNotification(NfKe_TimerTick);
		KeEnableNotification(NfKe_TerminateProcess);
		KeLinkIrq(6);

		m_IsInResetState = false;
		m_Drives[0].EnableMotor();
		SelectDrive(0);

		KeOutPortByte(m_CCR, 0x00);

		WaitForInterrupt();
		for (dword i = 0; i < 4; i++)
			CmdSenseInterrupt();

		WriteDR(0x03); // Specify
		WriteDR(0xDF); /* steprate = 3ms, unload time = 240ms */
		WriteDR(0x02); /* load time = 16ms, no-DMA = 0 */

		if (!CmdRecalibrate())
		{
			// Drive #0 detection failed
			return;
		}

		KeEnableCallRequest(ClFloppy_ReadSector);

		CStorageDeviceInfo SDI;
		SDI.m_DriverID = 0xF15A52BC;
		SDI.m_PhysicalDeviceID = 0;
		SDI.m_LogicalDeviceID = 0;
		SDI.m_SectorSize = 512;
		SDI.m_StartSector = 0;
		SDI.m_SectorCount = 2880;
		SDI.m_CachedSectorsCount = 64;
		SDI.m_ReadSectorFunc = ClFloppy_ReadSector;
		KeNotify(Nf_StorageDeviceDetected, PB(&SDI), sizeof(SDI));

		CNotification<0x10> N;
		CCallRequest<0x10> CR;
		for (;;)
		{
			KeWaitFor(3);
			dword NfCount;
			dword CallCount;
			KeGetNfClCount(NfCount, CallCount);

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfKe_TimerTick)
				{
					m_Drives[0].IncrementMotorIdleTime();
					if (m_Drives[0].GetMotorIdleTime() > 18 * 4)
						StopMotor(0);
				}
				else if (N.GetID() == NfKe_TerminateProcess)
				{
					StopMotor(0);
					return;
				}
			}
			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClFloppy_ReadSector)
				{
					byte Cyl;
					byte Head;
					byte Sect;
					
					CR.Recv();
					dword DevID = CR.GetDword(0);
					dword LBA = CR.GetDword(1);

					LBAtoCHS(LBA, Cyl, Head, Sect);
					ReadSector(Cyl, Head, Sect);

					CR.Respond(PB(m_DMABuffer), 512);
				}
			}
		}
	}

	bool ReadSector(byte Cyl, byte Head, byte Sect)
	{
		bool IsOK = true;
		SelectDrive(0);
		SpinMotor(0);
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				ResetDMA();
				IsOK = CmdSeek(Cyl, Head);
				if (IsOK)
				{
					IsOK = CmdRead(Cyl, Head, Sect);
					if (IsOK)
						break;
				}
			}
			if (IsOK)
				break;
			else
				CmdRecalibrate();
		}
		return IsOK;
	}

	bool CmdRecalibrate()
	{
		WriteDR(0x07); // Recalibrate
		WriteDR(0x00); // Drive 0

		WaitForInterrupt();

		byte st0;
		byte pcn;
		CmdSenseInterrupt(st0, pcn);

		// Abnormal termination = 1, Seek End = 1 -> false
		return (st0 & 0x60) != 0x60;
	}

	bool CmdSeek(byte Cyl, byte Head)
	{
		WriteDR(0x0F); // Seek
		WriteDR(Head << 2);
		WriteDR(Cyl);

		WaitForInterrupt();
		return CmdSenseInterrupt();
	}

	bool CmdRead(byte Cyl, byte Head, byte Sect)
	{
		WriteDR(0x46); // Read
		WriteDR(Head << 2);
		WriteDR(Cyl);  // Cyl
		WriteDR(Head); // Head
		WriteDR(Sect); // Sect Num R
		WriteDR(0x02); // Sect Sz N
		WriteDR(18);   // TrLen
		WriteDR(0x1B); // Gap3
		WriteDR(0xFF); // Data Len

		WaitForInterrupt();

		byte ST0 = ReadDR(); // ST0
		ReadDR(); // ST1
		ReadDR(); // ST2
		ReadDR(); // Cyl
		ReadDR(); // Head
		ReadDR(); // Sect Num R
		ReadDR(); // Sect Sz N

		if (ST0 & 0xC0)
			return false;
		else
			return true;
	}

	void ResetDMA()
	{
		dword SMID = 0;
		KeRequestCall(ClDMA_Reset, null, 0, PB(&SMID), 4);
		if (m_DMABuffer == 0)
			m_DMABuffer = dword(KeMapSharedMem(SMID));
	}

	void WriteDR(byte Byte)
	{
		//DebugOut("[>", 2);
		for (;;)
		{
			byte MSR = ReadMSR();
			if (TestBit(MSR, 7))
				if (!TestBit(MSR, 6))
					break;
		}
		//DebugOut(Byte);
		KeOutPortByte(m_DR, Byte);
		//DebugOut("]", 1);
	}

	byte ReadDR()
	{
		//DebugOut("[<", 2);
		for (;;)
		{
			byte MSR = ReadMSR();
			if (TestBit(MSR, 7))
				if (TestBit(MSR, 6))
					break;
		}
		byte D = KeInPortByte(m_DR);
		//DebugOut(D);
		//DebugOut("]", 1);
		return D;
	}

	void WaitForInterrupt()
	{
		CNotification<4> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();
			if (N.GetID() == NfKe_Irq)
			{
				KeEndOfInterrupt(6);
				break;
			}
		}
	}

	void CmdSenseInterrupt(byte& ST0, byte& PCN)
	{
		WriteDR(0x08);
		ST0 = ReadDR();
		PCN = ReadDR();
	}

	bool CmdSenseInterrupt()
	{
		byte ST0 = 0;
		byte PCN = 0;
		CmdSenseInterrupt(ST0, PCN);

		if (ST0 & 0xC0)
			return false;
		else
			return true;
	}

	void SelectDrive(byte DriveIndex)
	{
		m_SelectedDrive = DriveIndex;
		UpdateDOR();
	}

	void SpinMotor(byte DriveIndex)
	{
		m_Drives[DriveIndex].ResetMotorIdleTime();
		if (m_Drives[DriveIndex].IsMotorEnabled())
			return;

		DebugOut("[FM+]", 6);
		m_Drives[DriveIndex].EnableMotor();
		UpdateDOR();
	}

	void StopMotor(byte DriveIndex)
	{
		if (!m_Drives[DriveIndex].IsMotorEnabled())
			return;

		DebugOut("[FM-]", 6);
		m_Drives[DriveIndex].DisableMotor();
		UpdateDOR();
	}

	byte ReadMSR()
	{
		return KeInPortByte(m_MSR);
	}

	void UpdateDOR()
	{
		byte DORValue = 0;
		if (m_Drives[3].IsMotorEnabled())
			DORValue |= 0x80;
		if (m_Drives[2].IsMotorEnabled())
			DORValue |= 0x40;
		if (m_Drives[1].IsMotorEnabled())
			DORValue |= 0x20;
		if (m_Drives[0].IsMotorEnabled())
			DORValue |= 0x10;
		if (!m_IsInResetState)
			DORValue |= 0x04;

		DORValue |= 0x08; // Enable DMA
		DORValue |= m_SelectedDrive & 0x3;

		if (DORValue != m_CachedDORValue)
		{
			KeOutPortByte(m_DOR, DORValue);
			m_CachedDORValue = DORValue;
		}
	}

private:
	bool m_IsInResetState;
	byte m_CachedDORValue;
	dword m_SelectedDrive;

	dword m_DMABuffer;
	CDrive m_Drives[4];

	static const word m_DOR = 0x3F2;
	static const word m_MSR = 0x3F4;
	static const word m_DR  = 0x3F5;
	static const word m_CCR = 0x3F7;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Floppy))
		return;
	CFloppy F;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=