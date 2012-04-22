// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// VGAVideo.cpp
#include "API.h"
#include "Random.h"

// ----------------------------------------------------------------------------
class CVGAVideoParams
{
public:
    byte sequ_regs[4];
    byte miscreg;
    byte crtc_regs[25];
    byte actl_regs[20];
    byte grdc_regs[9];
};

// ----------------------------------------------------------------------------
class CUpdateInfo
{
public:
	dword m_X;
	dword m_Y;
	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
class CVGAVideo
{
public:
	CVGAVideo()
	{
		dword Width = 640;
		dword Height = 480;

		m_ShadowBuf = new byte[640 * 480 / 2];
		m_VGABuf = KeMapSharedMem(
			KeAllocSharedMemAt(640 * 480 / 8, 0xA0000));

		InitVGA();

		KeOutPortWord(0x3C4, 0x0F02);
		for (dword i = 0; i < 640 * 480 / 8; i++)
			m_VGABuf[i] = 0;

		dword FrameBufSMID = KeAllocSharedMem(Width * Height * 4);
		m_FrameBuf = KeMapSharedMem(FrameBufSMID);

		KeEnableCallRequest(ClVideo_GetFrameSurface);
		KeEnableCallRequest(ClVideo_GetCaps);
		KeEnableCallRequest(ClVideo_UpdateFrameSurface);
		KeEnableCallRequest(ClVideo_GetQuantSize);
		KeEnableNotification(NfKe_TerminateProcess);

		KeSetSymbol(SmVideo_OK);

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
				if (CR.GetTypeID() == ClVideo_GetFrameSurface)
				{
					dword RespBuf[3];
					RespBuf[0] = FrameBufSMID;
					RespBuf[1] = Width;
					RespBuf[2] = Height;
					CR.Respond(PB(RespBuf), 12);
				}
				else if (CR.GetTypeID() == ClVideo_GetCaps)
				{
					byte Caps[2];
					Caps[0] = 0; // No Double Buffering
					Caps[1] = 1; // Driver Update
					CR.Respond(Caps, 2);
				}
				else if (CR.GetTypeID() == ClVideo_UpdateFrameSurface)
				{
					dword UpdateInfoArrSMID   = CR.GetDword(0);
					dword UpdateInfoTopIndex  = CR.GetDword(1);
					UpdateFrameSurface(UpdateInfoArrSMID, UpdateInfoTopIndex);
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClVideo_GetQuantSize)
				{
					dword QuantSize = 8;
					CR.Respond(QuantSize);
				}
			}
		}
	}

	void UpdateFrameSurface(dword UpdateInfoArrSMID, dword UpdateInfoTopIndex)
	{
		dword DirtyW = 640 / 8;
		dword DirtyH = 480 / 8;

		CUpdateInfo* UI =
			(CUpdateInfo*)KeMapSharedMem(UpdateInfoArrSMID);

		for (dword i = 0; i < UpdateInfoTopIndex; i++)
			VGAShadowBlit(UI[i].m_X / 8, UI[i].m_Y / 8, DirtyW);

		for (dword k = 0; k < 4; k++)
		{
			dword km = 4 - k - 1;
			KeOutPortWord(0x3C4, 2 | (1 << (km + 8)));
			byte* ShadowPlaneOffset = m_ShadowBuf + 38400 * km;

			for (dword i = 0; i < UpdateInfoTopIndex; i++)
			{
				dword x = UI[i].m_X / 8;
				dword y = UI[i].m_Y / 8;

				dword VGAByteOfss = (x + y * DirtyW) * 8;
				dword VidBufOffset = x + DirtyW * (y * 8);
				for (dword z = 0; z < 8; z++)
				{
					m_VGABuf[VidBufOffset] = ShadowPlaneOffset[VGAByteOfss];
					VGAByteOfss++;
					VidBufOffset += DirtyW;
				}
			}
		}
	}

	void VGAShadowBlit(dword QX, dword QY, dword DirtyW)
	{
		dword VGAByteOfss = (QX + QY * DirtyW) * 8;
		for (dword y = 0; y < 8; y++)
		{
			dword FullColOffs = (QX * 8 + (QY * 8 + y) * 640) * 4;
			for (dword x = 0; x < 8; x++)
			{
				byte B = m_FrameBuf[0 + FullColOffs];
				byte G = m_FrameBuf[1 + FullColOffs];
				byte R = m_FrameBuf[2 + FullColOffs];

				byte I = (B * 11 + G * 59 + R * 30) / 100;

				dword RNum = m_Rand.GetNextWord() % 16;
				if (RNum < (I & 0xF)) I = (I >> 4) + 1;
				else I = I >> 4;
				if (I > 15) I = 15;

				for (dword k = 0; k < 4; k++)
				{
					if (I & 1)
						SetBit(m_ShadowBuf[VGAByteOfss], 7-x);
					else
						ClearBit(m_ShadowBuf[VGAByteOfss], 7-x);
					VGAByteOfss += 38400;
					I >>= 1;
				}
				VGAByteOfss -= 38400 * 4;
				FullColOffs += 4;
			}
			VGAByteOfss++;
		}
	}

private:
	const byte* m_FrameBuf;
	byte* m_ShadowBuf;
	volatile byte* m_VGABuf;
	CRandom m_Rand;

private:
	void InitVGA()
	{
		static CVGAVideoParams VP =
		{
			 0x01, 0x0f, 0x00, 0x06, /* sequ_regs */
			 0xe3, /* miscreg */
			 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
			 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3,
			 0xff, /* crtc_regs */
			 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			 0x01, 0x00, 0x0f, 0x00, /* actl_regs */
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */ 
		};

		#define VGAREG_ACTL_ADDRESS            0x3c0
		#define VGAREG_ACTL_WRITE_DATA         0x3c0
		#define VGAREG_ACTL_READ_DATA          0x3c1

		#define VGAREG_SEQU_ADDRESS            0x3c4
		#define VGAREG_SEQU_DATA               0x3c5

		#define VGAREG_GRDC_ADDRESS            0x3ce
		#define VGAREG_GRDC_DATA               0x3cf 

		#define VGAREG_VGA_CRTC_ADDRESS        0x3d4
		#define VGAREG_VGA_CRTC_DATA           0x3d5 

		#define VGAREG_WRITE_MISC_OUTPUT       0x3c2
		#define VGAREG_ACTL_RESET              0x3da 

		#define VGAREG_DAC_WRITE_ADDRESS       0x3c8
		#define VGAREG_DAC_DATA                0x3c9
		#define VGAREG_PEL_MASK                0x3c6

		KeOutPortByte(VGAREG_PEL_MASK, 0xFF); 
		KeOutPortByte(VGAREG_DAC_WRITE_ADDRESS, 0x00);
		for (dword i = 0; i < 16; i++)
		{
			KeOutPortByte(VGAREG_DAC_DATA, (i << 2) | (i >> 2));
			KeOutPortByte(VGAREG_DAC_DATA, (i << 2) | (i >> 2));
			KeOutPortByte(VGAREG_DAC_DATA, (i << 2) | (i >> 2));
		}

		// Reset Attribute Ctl flip-flop
		KeInPortByte(VGAREG_ACTL_RESET);

		// Set Attribute Ctl
		for (dword i = 0; i <= 0x13; i++)
		{
			KeOutPortByte(VGAREG_ACTL_ADDRESS,i);
			KeOutPortByte(VGAREG_ACTL_WRITE_DATA, VP.actl_regs[i]);
		}
		KeOutPortByte(VGAREG_ACTL_ADDRESS,0x14);
		KeOutPortByte(VGAREG_ACTL_WRITE_DATA,0x00);

		// Set Sequencer Ctl
		KeOutPortByte(VGAREG_SEQU_ADDRESS,0);
		KeOutPortByte(VGAREG_SEQU_DATA,0x03);
		for (dword i = 1; i <= 4; i++)
		{
			KeOutPortByte(VGAREG_SEQU_ADDRESS,i);
			KeOutPortByte(VGAREG_SEQU_DATA,VP.sequ_regs[i - 1]);
		}

		// Set Grafx Ctl
		for (dword i = 0; i <= 8; i++)
		{
			KeOutPortByte(VGAREG_GRDC_ADDRESS,i);
			KeOutPortByte(VGAREG_GRDC_DATA,VP.grdc_regs[i]);
		}

		// Disable CRTC write protection
		KeOutPortWord(VGAREG_VGA_CRTC_ADDRESS, 0x0011);
		// Set CRTC regs
		for (dword i = 0; i <= 0x18; i++)
		{
			KeOutPortByte(VGAREG_VGA_CRTC_ADDRESS,i);
			KeOutPortByte(VGAREG_VGA_CRTC_ADDRESS + 1,VP.crtc_regs[i]);
		}

		// Set the misc register
		KeOutPortByte(VGAREG_WRITE_MISC_OUTPUT,VP.miscreg);

		// Enable video
		KeOutPortByte(VGAREG_ACTL_ADDRESS,0x20);
		KeInPortByte(VGAREG_ACTL_RESET); 
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	CVGAVideo VV;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=