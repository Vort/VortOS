#pragma comment(linker, "-entry:main")
#include "Defs.h"
#include "File.h"
#include "CRC32.h"
#include "String2.h"
#include <windows.h>
#include "UnicodeConvert2.h"
using namespace Lib;

void main()
{
	int nArgs;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if (szArglist == NULL || nArgs < 2)
	{
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "Error", 5, null, null);
		return;
	}

	CStringA SourceFilename(CUnicodeConvert2().StringWToA(szArglist[1]));
	CStringA DestFilename = SourceFilename;
	DestFilename.Replace("bi_", "bin");

	CFile::Delete(DestFilename._ptr());
	CopyFileA(SourceFilename._ptr(), DestFilename._ptr(), true);
	CFile F(DestFilename._ptr(), ReadWrite);
	if (!F.IsOpened())
		return;
	if (F.GetSize() == 0)
		return;

	dword PEOffset = 0;
	F.SetPos(0x3C);
	F.Read(PB(&PEOffset), 4);

	dword ImageBase = 0;
	F.SetPos(PEOffset + 0x34);
	F.Read(PB(&ImageBase), 4);

	dword EP = 0;
	F.SetPos(PEOffset + 0x28);
	F.Read(PB(&EP), 4);

	dword SectionOffset = PEOffset + 0xF8;

	dword CodeRawSize = 0;
	dword RDataRawSize = 0;
	dword DataRawSize = 0;

	F.SetPos(SectionOffset + 0x10);
	F.Read(PB(&CodeRawSize), 4);
	SectionOffset += 0x28;

	F.SetPos(SectionOffset + 0x10);
	F.Read(PB(&RDataRawSize), 4);
	SectionOffset += 0x28;

	F.SetPos(SectionOffset + 0x10);
	F.Read(PB(&DataRawSize), 4);

	dword FS = F.GetSize();
	
	byte Buf[4096] = {0};

	// 0  - Magic
	// 4  - Ver
	// 8  - EP
	// 12 - CodePageCount
	// 16 - RDataPageCount
	// 20 - DataPageCount
	// 24 - CRC32

	Buf[0] = 'V';
	Buf[1] = 'K';
	Buf[2] = 'r';
	Buf[3] = 'n';
	Buf[4] = 0x02;
	Buf[5] = 0x00;
	Buf[6] = 0x00;
	Buf[7] = 0x00;
	*PD(&Buf[8]) = ImageBase + EP;
	*PD(&Buf[12]) = CodeRawSize >> 12;
	*PD(&Buf[16]) = RDataRawSize >> 12;
	*PD(&Buf[20]) = DataRawSize >> 12;

	F.SetPos(4096);
	CArray<byte> CRCBuf(byte(0), FS - 4096);
	F.Read(CRCBuf._ptr(), FS - 4096);
	dword CRC32 = CCRC32().GetCRC32(CRCBuf._ptr(), CRCBuf.Size());

	*PD(&Buf[24]) = CRC32;

	F.SetPos(0);
	F.Write(Buf, 4096);
}