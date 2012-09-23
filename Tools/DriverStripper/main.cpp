#pragma comment(linker, "-entry:main")
#include "Defs.h"
#include "File.h"
#include "String2.h"
#include "DigitConvert.h"
#include "CRC32.h"

#include <windows.h>
#include "UnicodeConvert2.h"
using namespace Lib;

void main()
{
	dword EntryPoint = 0;

	dword CodeVSize = 0;
	dword CodeRawOffset = 0;

	dword RDataVSize = 0;
	dword RDataRawOffset = 0;

	dword DataVSize = 0;
	dword DataRawSize = 0;
	dword DataRawOffset = 0;

	int nArgs;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if (szArglist == NULL || nArgs < 4)
	{
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), "Error", 5, null, null);
		return;
	}

	CStringA SourceFilename(CUnicodeConvert2().StringWToA(szArglist[1]));

	dword priority = CDigitConvert().StringToDWord(
		CUnicodeConvert2().StringWToA(szArglist[2])._ptr());
	dword accessLevel = CDigitConvert().StringToDWord(
		CUnicodeConvert2().StringWToA(szArglist[3])._ptr());

	CFile F(SourceFilename._ptr(), Read);
	if (!F.IsOpened())
		return;

	dword PEOffset = 0;
	F.SetPos(0x3C);
	F.Read(PB(&PEOffset), 4);

	word NumOfSect = 0;
	dword NumOfSectOffset = PEOffset + 6;

	F.SetPos(NumOfSectOffset);
	F.Read(PB(&NumOfSect), 2);

	F.SetPos(PEOffset + 0x18 + 0x10);
	F.Read(PB(&EntryPoint), 4);
	dword ImageBase = 0;
	F.SetPos(PEOffset + 0x18 + 0x1C);
	F.Read(PB(&ImageBase), 4);
	EntryPoint += ImageBase;

	dword SectionOffset = PEOffset + 0xF8;

	for (dword i = 0; i < NumOfSect; i++)
	{
		char SectNameBuf[9] = {0};
		F.SetPos(SectionOffset + i * 0x28);
		F.Read(PB(SectNameBuf), 8);
		CStringA SectName(SectNameBuf);

		dword VSize = 0;
		F.Read(PB(&VSize), 4);
		dword VOffset = 0;
		F.Read(PB(&VOffset), 4);
		dword RawSize = 0;
		F.Read(PB(&RawSize), 4);
		dword RawOffset = 0;
		F.Read(PB(&RawOffset), 4);

		if (SectName == ".text")
		{
			CodeVSize = VSize;
			CodeRawOffset = RawOffset;
		}
		else if (SectName == ".rdata")
		{
			RDataVSize = VSize;
			RDataRawOffset = RawOffset;
		}
		else if (SectName == ".data")
		{
			DataVSize = VSize;
			DataRawSize = RawSize;
			if (DataVSize < DataRawSize)
				DataRawSize = DataVSize;
			DataRawOffset = RawOffset;
		}
		else
		{
			CStringA S("Unknown section: '");
			S.Add(SectName);
			S.Add("'\n");
			WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), S._ptr(), S.Len(), null, null);
			ExitProcess(-1);
		}
	}

	byte* Code = new byte[CodeVSize];
	byte* RData = new byte[RDataVSize];
	byte* Data = new byte[DataRawSize];

	F.SetPos(CodeRawOffset);
	F.Read(Code, CodeVSize);

	F.SetPos(RDataRawOffset);
	F.Read(RData, RDataVSize);

	F.SetPos(DataRawOffset);
	F.Read(Data, DataRawSize);

	char* Sign = "VExe";
	dword Ver = 4;

	dword Zer = 0;

	CStringA DestFilename = SourceFilename;
	DestFilename.Replace("bi_", "bin");
	CFile F2(DestFilename._ptr(), ReadWrite);
	F2.Clear();
	F2.Write(PB(Sign), 4);             // 0
	F2.Write(PB(&Ver), 4);             // 4
	F2.Write(PB(&Zer), 4);             // 8
	F2.Write(PB(&EntryPoint), 4);      // 12
	F2.Write(PB(&CodeVSize), 4);       // 16
	F2.Write(PB(&RDataVSize), 4);      // 20
	F2.Write(PB(&DataVSize), 4);       // 24
	F2.Write(PB(&DataRawSize), 4);     // 28
	F2.Write(PB(&priority), 2);        // 32
	F2.Write(PB(&accessLevel), 2);     // 34
	F2.Write(Code, CodeVSize);         // 36
	F2.Write(RData, RDataVSize);
	F2.Write(Data, DataRawSize);

	byte* Zu = new byte[CodeVSize + RDataVSize + DataRawSize];
	F2.SetPos(36);
	F2.Read(Zu, CodeVSize + RDataVSize + DataRawSize);

	dword CRC32 = CCRC32().GetCRC32(Zu, CodeVSize + RDataVSize + DataRawSize);
	F2.SetPos(8);
	F2.Write(PB(&CRC32), 4);             // 8
}