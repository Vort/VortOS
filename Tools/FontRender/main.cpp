// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// main.cpp
#include "Defs.h"
#include "Array2.h"
#include "File.h"
#include <Windows.h>

#pragma comment (linker, "-entry:main")

// ----------------------------------------------------------------------------
using namespace Lib;

// ----------------------------------------------------------------------------
void RLEWriteSize(dword Size, bool IsSolid, CArray<byte>& Dest)
{
	byte B1 = 0;
	byte B2 = 0;

	if (IsSolid)
		B1 |= 0x40;

	B1 |= Size & 0x3F;

	if (Size <= 0x3F)
		Dest.PushBack(B1);
	else if (Size <= 0x3FFF)
	{
		B1 |= 0x80;
		B2 = (Size >> 6) & 0xFF;
		Dest.PushBack(B1);
		Dest.PushBack(B2);
	}
	else
		Err;
}

// ----------------------------------------------------------------------------
void RLECompress(byte* Data, dword DataSize, CArray<byte>& Dest)
{
	bool IsRepeat = false;
	CArray<byte> Chunk;
	for (dword i = 0; i < DataSize; i++)
	{
		Chunk.PushBack(Data[i]);
		dword ChunkSize = Chunk.Size();

//		if (ChunkSize > 3)
		if (ChunkSize > 2)
		{
			byte B1 = Chunk[ChunkSize - 1];
			byte B2 = Chunk[ChunkSize - 2];
			byte B3 = Chunk[ChunkSize - 3];
			//byte B4 = Chunk[ChunkSize - 4];
			bool B = (B1 == B2) && (B1 == B3)/* && (B1 == B4)*/;
			if (!IsRepeat)
			{
				if (B)
				{
					CArray<byte> SolidChunk = Chunk.Extract(0, ChunkSize - /*4*/3);
					dword SolidChunkSize = SolidChunk.Size();
					if (SolidChunkSize != 0)
					{
						RLEWriteSize(SolidChunkSize, true, Dest);
						Dest.PushBack(SolidChunk);
					}
					IsRepeat = true;
				}
			}
			else
			{
				if (!B)
				{
					byte RepeatByte = Chunk[0];
					dword RepeatChunkSize = ChunkSize - 1;
					Chunk.Delete(0, RepeatChunkSize);

					RLEWriteSize(RepeatChunkSize, false, Dest);
					Dest.PushBack(RepeatByte);
					IsRepeat = false;
				}
			}
		}
	}

	if (Chunk.Size() == 0)
		return;

	if (IsRepeat)
	{
		byte RepeatByte = Chunk[0];
		dword RepeatChunkSize = Chunk.Size();

		RLEWriteSize(RepeatChunkSize, false, Dest);
		Dest.PushBack(RepeatByte);
	}
	else
	{
		dword SolidChunkSize = Chunk.Size();
		RLEWriteSize(SolidChunkSize, true, Dest);
		Dest.PushBack(Chunk);
	}
}

// ----------------------------------------------------------------------------
void RLEDeCompress(byte* Data, dword DataSize, CArray<byte>& Dest)
{
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
				Dest.PushBack(Data[DataPtr + i]);
			DataPtr += ChunkSize;
		}
		else
		{
			byte RepeatByte = Data[DataPtr];
			DataPtr++;

			for (dword i = 0; i < ChunkSize; i++)
				Dest.PushBack(RepeatByte);
		}
		if (DataPtr >= DataSize)
			break;
	}
}

// ----------------------------------------------------------------------------
void main()
{
	CArray<dword> m_LetterW;
	CArray<dword> m_LetterH;

	dword TexDataH = 14;

	HFONT hFont = CreateFontA(TexDataH, 0, 0, 0, FW_BOLD, 0, 0,
		0, RUSSIAN_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, "Arial");

	HDC tempDC = GetDC(GetDesktopWindow());
	HDC DC = CreateCompatibleDC(tempDC);
	HBITMAP BMP = CreateCompatibleBitmap(tempDC, 4096, TexDataH);
	SelectObject(DC, BMP);
	SelectObject(DC, hFont);

	//SetTextColor(DC, 0xFFFFFF);
	//SetBkColor(DC, 0);

	SetTextColor(DC, 0);
	SetBkColor(DC, 0xFFFFFF);
	RECT RC = {0};
	RC.top = TexDataH;
	RC.right = 4096;
	FillRect(DC, &RC, CreateSolidBrush(0xFFFFFF));

	for (dword i = 0; i < 256; i++)
	{
		char c[2] = {0};
		c[0] = char(i);
		SIZE S = {0};
		GetTextExtentPoint32A(DC, c, 1, &S);
		m_LetterW.PushBack(S.cx/* + 1*/);
		m_LetterH.PushBack(S.cy);

		TextOutA(DC, i * 16, 0, c, 1);
	}

	bool EnChars[256] = {0};
	for (char c = '0'; c <= '9'; c++)
		EnChars[c] = true;
	for (char c = 'a'; c <= 'z'; c++)
		EnChars[c] = true;
	for (char c = 'A'; c <= 'Z'; c++)
		EnChars[c] = true;

	EnChars[' '] = true;
	EnChars['!'] = true;
	EnChars['"'] = true;
	EnChars['#'] = true;
	EnChars['$'] = true;
	EnChars['%'] = true;
	EnChars['&'] = true;
	EnChars['\''] = true;
	EnChars['('] = true;
	EnChars[')'] = true;
	EnChars['*'] = true;
	EnChars['+'] = true;
	EnChars[','] = true;
	EnChars['-'] = true;
	EnChars['.'] = true;
	EnChars['/'] = true;
	EnChars[':'] = true;
	EnChars[';'] = true;
	EnChars['<'] = true;
	EnChars['='] = true;
	EnChars['>'] = true;
	EnChars['?'] = true;
	EnChars['@'] = true;
	EnChars['['] = true;
	EnChars['\\'] = true;
	EnChars[']'] = true;
	EnChars['^'] = true;
	EnChars['_'] = true;
	EnChars['`'] = true;
	EnChars['{'] = true;
	EnChars['|'] = true;
	EnChars['}'] = true;
	EnChars['~'] = true;

	dword ECCount = 0;
	dword TexDataW = 0;
	for (dword i = 0; i < 256; i++)
	{
		if (EnChars[i])
		{
			TexDataW += m_LetterW[i];
			ECCount++;
		}
	}
	//cout << TexDataW;
	byte* m_TexData = new byte[TexDataW * TexDataH];

	dword WI = 0;
	for (dword i = 0; i < 256; i++)
	{
		if (EnChars[i])
		{
			dword LWI = m_LetterW[i];
			for (dword y = 0; y < TexDataH; y++)
				for (dword x = 0; x < LWI; x++)
				{
//					byte V = GetPixel(DC, i * 16 + x - 1, y) & 0xFF;
					byte V = 0xFF - (GetPixel(DC, i * 16 + x - 1, y) & 0xFF);
					m_TexData[TexDataW * y + WI + x] = V;
				}
			WI += LWI;
		}
	}

	DeleteDC(DC);
	DeleteObject(hFont);
	DeleteObject(BMP);

	CFile F("Font.raw", Write);
	F.Clear();
	F.Write(PB(&ECCount), 2);
	F.Write(PB(&TexDataW), 2);
	F.Write(PB(&TexDataH), 2);

	for (dword i = 0; i < 256; i++)
	{
		if (EnChars[i])
		{
			dword LW = m_LetterW[i];
			F.Write(PB(&i), 1);
			F.Write(PB(&LW), 1);
		}
	}

	CArray<byte> Compressed;
	RLECompress(m_TexData, TexDataW * 14, Compressed);

	CArray<byte> DeCompressed;
	RLEDeCompress(Compressed._ptr(), Compressed.Size(), DeCompressed);

	if (DeCompressed.Size() != TexDataW * 14)
		Err;

	for (dword i = 0; i < TexDataW * 14; i++)
		if (DeCompressed[i] != m_TexData[i])
			Err;

	//F.Write(m_TexData, TexDataW * 14);
	F.Write(Compressed._ptr(), Compressed.Size());
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=