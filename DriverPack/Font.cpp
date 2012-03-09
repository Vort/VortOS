// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Font.cpp
#include "API.h"
#include "Font_Arial.h"
#include "VideoStruct.h"

// ----------------------------------------------------------------------------
void RLEDeCompress(const byte* Data, dword DataSize, byte* DstData)
{
	dword DstDataPtr = 0;
	dword DataPtr = 0;
	for (;;)
	{
		byte B1 = Data[DataPtr];
		byte B2 = 0;
		DataPtr++;

		dword ChunkSize = B1 & 0x3F;

		if (B1 & 0x80)
		{
			B2 = Data[DataPtr];
			ChunkSize = ChunkSize | (dword(B2) << 6);
			DataPtr++;
		}

		if (B1 & 0x40)
		{
			for (dword i = 0; i < ChunkSize; i++)
				DstData[DstDataPtr++] = Data[DataPtr + i];
			DataPtr += ChunkSize;
		}
		else
		{
			byte RepeatByte = Data[DataPtr];
			DataPtr++;

			for (dword i = 0; i < ChunkSize; i++)
				DstData[DstDataPtr++] = RepeatByte;
		}
		if (DataPtr >= DataSize)
			break;
	}
}

// ----------------------------------------------------------------------------
class CFont
{
public:
	CFont()
	{
		dword Count = (PW(ArialFontImage))[0];
		dword TotalW = (PW(ArialFontImage))[1];
		m_FontHeight = (PW(ArialFontImage))[2];

		const byte* Ptr = ArialFontImage + 6;

		for (dword i = 0; i < 256; i++)
		{
			m_Sizes[i] = 0;
			m_Offsets[i] = 0;
		}

		dword CurW = 0;
		for (dword i = 0; i < Count; i++)
		{
			dword C = Ptr[0];
			dword W = Ptr[1];
			m_Offsets[C] = CurW;
			m_Sizes[C] = W;
			CurW += W;
			Ptr += 2;
		}

		dword TexSMID = KeAllocSharedMem(m_FontHeight * TotalW);
		byte* Texture = KeMapSharedMem(TexSMID);
		RLEDeCompress(Ptr, sizeof(ArialFontImage) - 6 - Count * 2, Texture);

		m_BlitTableSMID = KeAllocSharedMem(256 * sizeof(CFontBlitTableEntry));
		CFontBlitTableEntry* BlitTable = 
			(CFontBlitTableEntry*)KeMapSharedMem(m_BlitTableSMID);

		for (dword i = 0; i < 256; i++)
		{
			BlitTable[i].m_X = m_Offsets[i];
			BlitTable[i].m_Y = 0;
			BlitTable[i].m_Width = m_Sizes[i];
			BlitTable[i].m_Height = m_FontHeight;
		}

		dword OutBuf[8];
		KeWaitForSymbol(SmSurfMgr_Ready);

		OutBuf[0] = TotalW;
		OutBuf[1] = m_FontHeight;
		OutBuf[2] = TexSMID;
		OutBuf[3] = m_BlitTableSMID;
		KeRequestCall(ClSurfMgr_SetFont, PB(OutBuf), 16, null, 0);

		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableCallRequest(ClFont_GetTextWidth);
		KeEnableCallRequest(ClFont_FitText);
		KeSetSymbol(SmFont_Ready);

		CNotification<4> N;
		CCallRequest<0x100> CR;
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

				if (CR.GetTypeID() == ClFont_GetTextWidth)
				{
					CR.Respond(GetTextWidth(PC(CR.GetBuf()), CR.GetSize()));
				}
				else if (CR.GetTypeID() == ClFont_FitText)
				{
					dword Width = CR.GetDword(0);
					dword F = FitText(Width, PC(CR.GetBuf() + 4), CR.GetSize() - 4);
					CR.Respond(F);
				}
			}
		}
	}

	dword FitText(dword Width, const char* Text, dword TextSize)
	{
		dword Acc = 0;
		for (dword i = 0; i < TextSize; i++)
		{
			Acc += m_Sizes[Text[i]];
			if (Acc > Width)
				return i;
		}
		return TextSize;
	}

	dword GetTextWidth(const char* Text, dword TextSize)
	{
		dword Acc = 0;
		for (dword i = 0; i < TextSize; i++)
			Acc += m_Sizes[Text[i]];
		return Acc;
	}

private:
	dword m_BlitTableSMID;
	CFontBlitInfo* m_FontBlitInfo;

	dword m_Offsets[256];
	dword m_Sizes[256];
	dword m_TextSurfaceID;
	dword m_FontHeight;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Font))
		return;

	CFont F;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=