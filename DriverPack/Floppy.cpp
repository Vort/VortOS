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
		Found = false;
		HaveDisk = false;
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

public:
	bool Found;
	bool HaveDisk;

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
		// 82077AA CHMOS SINGLE-CHIP FLOPPY DISK CONTROLLER
		// 82077AA_FloppyControllerDatasheet.pdf

		// Floppy access available only if system boots from it
		if (KeGetBootType() == ' 3df')
			floppySize = 1440;
		else if (KeGetBootType() == ' 5df')
			floppySize = 360;
		else
			floppySize = 0;

		if (floppySize == 0)
			return;

		if (floppySize == 1440)
			sectorsPerTrack = 18;
		else if (floppySize == 360)
			sectorsPerTrack = 9;

		m_DMABuffer = 0;

		KeWaitForSymbol(SmDMA_Ready);
		KeWaitForSymbol(SmCache_Ready);
		KeWaitForSymbol(SmFileSystem_Ready);
		KeEnableNotification(NfKe_TimerTick);
		KeEnableNotification(NfKe_TerminateProcess);
		KeLinkIrq(6);

		// Figure 8-3
		m_SelectedDrive = 0;
		m_CachedDORValue = 0xFF;
		m_IsInResetState = true;  // Reset Controller
		UpdateDOR();
		m_IsInResetState = false;  // Reset Controller
		UpdateDOR();

		// Program data rate
		if (floppySize == 1440)
			KeOutPortByte(m_CCR, 0x00); // 500kbps
		else if (floppySize == 360)
			KeOutPortByte(m_CCR, 0x01); // 300kbps

		WaitForInterrupt();
		for (dword i = 0; i < 4; i++)
			CmdSenseInterrupt();

		CmdSpecify();

		for (dword i = 0; i < 4; i++)
			m_Drives[i].EnableMotor();

		bool noDriveFound = true;
		for (dword i = 0; i < 4; i++)
		{
			m_SelectedDrive = i;
			UpdateDOR();
			if (CmdRecalibrate(i))
			{
				m_Drives[i].Found = true;
				noDriveFound = false;
			}
		}

		if (noDriveFound)
			return;

		bool noDiskFound = true;
		for (dword i = 0; i < 4; i++)
		{
			if (!m_Drives[i].Found)
				continue;

			m_SelectedDrive = i;
			UpdateDOR();

			m_Drives[i].HaveDisk = true;
			if (KeInPortByte(m_DIR) & 0x80) // Disk change bit set
			{
				CmdSeek(1, 0);
				CmdSeek(0, 0);
				if (KeInPortByte(m_DIR) & 0x80)
					m_Drives[i].HaveDisk = false;
			}
			if (m_Drives[i].HaveDisk)
			{
				// Only one disk is supported
				noDiskFound = false;
				break;
			}
		}

		for (dword i = 0; i < 4; i++)
			if (!m_Drives[i].HaveDisk)
				m_Drives[i].DisableMotor();
		UpdateDOR();

		if (noDiskFound)
			return;


		KeEnableCallRequest(ClFloppy_ReadSector);

		CStorageDeviceInfo SDI;
		SDI.m_DriverID = 0xF15A52BC;
		SDI.m_PhysicalDeviceID = 0;
		SDI.m_LogicalDeviceID = 0;
		SDI.m_SectorSize = 512;
		SDI.m_StartSector = 0;
		if (floppySize == 1440)
			SDI.m_SectorCount = 2880;
		else if (floppySize == 360)
			SDI.m_SectorCount = 720;
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
					m_Drives[m_SelectedDrive].IncrementMotorIdleTime();
					if (m_Drives[m_SelectedDrive].GetMotorIdleTime() > 18 * 4)
						StopMotor(m_SelectedDrive);
				}
				else if (N.GetID() == NfKe_TerminateProcess)
				{
					StopMotor(m_SelectedDrive);
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

	void LBAtoCHS(dword LBA, byte& Cyl, byte& Head, byte& Sect)
	{
		dword CYL = LBA / (2 * sectorsPerTrack);
		dword TEMP = LBA % (2 * sectorsPerTrack);
		dword HEAD = TEMP / sectorsPerTrack;
		dword SECT = TEMP % sectorsPerTrack + 1;

		Cyl = CYL & 0xFF;
		Head = HEAD & 0xFF;
		Sect = SECT & 0xFF;
	}

	bool ReadSector(byte Cyl, byte Head, byte Sect)
	{
		bool IsOK = true;
		SpinMotor(m_SelectedDrive);
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
				CmdRecalibrate(m_SelectedDrive);
		}
		return IsOK;
	}

	bool CmdRecalibrate(int driveIndex)
	{
		WriteDR(0x07); // Recalibrate
		WriteDR(driveIndex);

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
		WriteDR((Head << 2) | m_SelectedDrive);
		if (floppySize == 1440)
			WriteDR(Cyl);
		else if (floppySize == 360) // In 720k or 1200k drive
			WriteDR(Cyl * 2);

		WaitForInterrupt();
		return CmdSenseInterrupt();
	}

	bool CmdRead(byte Cyl, byte Head, byte Sect)
	{
		WriteDR(0x46); // Read
		WriteDR((Head << 2) | m_SelectedDrive);
		WriteDR(Cyl);  // Cyl
		WriteDR(Head); // Head
		WriteDR(Sect); // Sect Num R
		WriteDR(0x02); // Sect Sz N
		WriteDR(sectorsPerTrack);   // TrLen
		if (floppySize == 1440)
			WriteDR(0x1B); // Gap3
		else if (floppySize == 360)
			WriteDR(0x2A); // Gap3
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
		for (;;)
		{
			byte MSR = ReadMSR();
			if (TestBit(MSR, 7))
				if (!TestBit(MSR, 6))
					break;
		}
		KeOutPortByte(m_DR, Byte);
	}

	byte ReadDR()
	{
		for (;;)
		{
			byte MSR = ReadMSR();
			if (TestBit(MSR, 7))
				if (TestBit(MSR, 6))
					break;
		}
		byte D = KeInPortByte(m_DR);
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

	void CmdSpecify()
	{
		WriteDR(0x03); // Specify
		WriteDR(0xDF); /* steprate = 3ms, unload time = 240ms */
		WriteDR(0x02); /* load time = 16ms, no-DMA = 0 */
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

		m_Drives[DriveIndex].EnableMotor();
		UpdateDOR();
	}

	void StopMotor(byte DriveIndex)
	{
		if (!m_Drives[DriveIndex].IsMotorEnabled())
			return;

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

	dword floppySize;
	dword sectorsPerTrack;

	static const word m_DOR = 0x3F2;
	static const word m_MSR = 0x3F4;
	static const word m_DR  = 0x3F5;
	static const word m_DIR = 0x3F7;
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