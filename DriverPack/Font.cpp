// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Font.cpp
#include "API.h"
#include "VideoStruct.h"

// ----------------------------------------------------------------------------
extern "C" void* memset(void* ptr, int value, size_t num);

// ----------------------------------------------------------------------------
#pragma pack(push, 1)
struct GlyphHeader
{
	wchar_t code;
	unsigned char advanceX;
	signed char offsetX;
	signed char offsetY;
	unsigned char width;
	unsigned char height;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------
class CFont
{
public:
	CFont()
	{
		char* fileName = "OpenSans.vfnt";
		dword fileSize = GetFileSize(fileName);
		dword fontImageSmid = KeAllocSharedMem(fileSize);
		ReadFile(fontImageSmid, fileName);
		byte* fontImage = KeMapSharedMem(fontImageSmid);

		dword signature = ((dword*)fontImage)[0];
		if (signature != 'tnfv')
			return;
		byte version = fontImage[4];
		if (version != 1)
			return;
		word charCount = *((word*)&fontImage[5]);

		for (dword i = 0; i < 2048; i++)
			sizes[i] = 0;

		dword blitTableSMID = KeAllocSharedMem(2048 * sizeof(CFontBlitTableEntry));
		CFontBlitTableEntry* blitTable = 
			(CFontBlitTableEntry*)KeMapSharedMem(blitTableSMID);
		memset(blitTable, 0x00, 2048 * sizeof(CFontBlitTableEntry));

		dword shiftX = 0;
		dword textureHeight = 0;
		const byte* ptr = fontImage + 7;
		for (int i = 0; i < charCount; i++)
		{
			GlyphHeader* gh = (GlyphHeader*)ptr;
			sizes[gh->code] = gh->advanceX;
			blitTable[gh->code].texSrcX = shiftX;
			blitTable[gh->code].advanceX = gh->advanceX;
			blitTable[gh->code].offsetX = gh->offsetX;
			blitTable[gh->code].offsetY = gh->offsetY;
			blitTable[gh->code].bitmapWidth = gh->width;
			blitTable[gh->code].bitmapHeight = gh->height;
			if (gh->height > textureHeight)
				textureHeight = gh->height;
			shiftX += gh->width;
			ptr += sizeof(GlyphHeader) + gh->width * gh->height;
		}

		dword textureWidth = shiftX;
		dword texSMID = KeAllocSharedMem(textureHeight * textureWidth);
		byte* texture = KeMapSharedMem(texSMID);

		shiftX = 0;
		ptr = fontImage + 7;
		for (int i = 0; i < charCount; i++)
		{
			GlyphHeader* gh = (GlyphHeader*)ptr;
			ptr += sizeof(GlyphHeader);

			for (int y = 0; y < gh->height; y++)
				for (int x = 0; x < gh->width; x++)
					texture[shiftX + x + y * textureWidth] = ptr[x + y * gh->width];

			ptr += gh->width * gh->height;
			shiftX += gh->width;
		}

		KeReleaseSharedMem(fontImageSmid);
		KeUnmapSharedMem(texSMID);

		dword OutBuf[8];
		KeWaitForSymbol(SmSurfMgr_Ready);

		OutBuf[0] = textureWidth;
		OutBuf[1] = textureHeight;
		OutBuf[2] = texSMID;
		OutBuf[3] = blitTableSMID;
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
			Acc += sizes[Text[i]];
			if (Acc > Width)
				return i;
		}
		return TextSize;
	}

	dword GetTextWidth(const char* Text, dword TextSize)
	{
		dword Acc = 0;
		for (dword i = 0; i < TextSize; i++)
			Acc += sizes[Text[i]];
		return Acc;
	}

private:
	short sizes[2048];
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Font))
		return;

	CFont F;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=