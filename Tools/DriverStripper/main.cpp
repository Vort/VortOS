#pragma comment(linker, "-entry:main")
#include "Defs.h"
#include "File.h"
#include "String2.h"
#include "DigitConvert.h"
#include "CRC32.h"

#include <windows.h>
using namespace Lib;

void main()
{
	dword CodeVSize = 0;
	dword CodeRawOffset = 0;

	dword DataVSize = 0;
	dword DataRawOffset = 0;

	CStringA S = GetCommandLineA();
 
	dword A = S.Find('(', 0);
	if (A == -1)
		return;
	dword B = S.Find(')', A+1);
	if (B == -1)
		return;

	CStringA XX = S.MidAbs(A+1, B);
	XX.Trim(" \"");
	if (XX == "")
		return;

	dword AccessLevel = CDigitConvert().StringToDWord(XX._ptr());

	A = S.Find('|', 0);
	if (A == -1)
		return;
	B = S.Find('|', A+1);
	if (B == -1)
		return;

	CStringA Z = S.MidAbs(A+1, B);
	Z.Trim(" \"");
	if (Z == "")
		return;

	dword Priority = CDigitConvert().StringToDWord(Z._ptr());

	S = S.Left(A);
	S.Trim(" \"");

	CStringA X;
	dword SLen = S.Len();
	for (dword i = SLen; i > 0; i--)
		if (S.GetCh(i) == ' ')
		{
			X = S.RightAbs(i);
			break;
		}
	X.Trim(" \"");
	X.Trim(" \"");

	if (X == "")
		return;

	CFile F(X._ptr(), Read);
	if (!F.IsOpened())
		return;

	dword PEOffset = 0;
	F.SetPos(0x3C);
	F.Read(PB(&PEOffset), 4);

	dword SectionOffset = PEOffset + 0xF8;

	F.SetPos(SectionOffset+0x8);
	F.Read(PB(&CodeVSize), 4);
	F.SetPos(SectionOffset+0x14);
	F.Read(PB(&CodeRawOffset), 4);

	F.SetPos(SectionOffset+0x30);
	F.Read(PB(&DataVSize), 4);
	F.SetPos(SectionOffset+0x3C);
	F.Read(PB(&DataRawOffset), 4);

	byte* Code = new byte[CodeVSize];
	byte* Data = new byte[DataVSize];

	F.SetPos(CodeRawOffset);
	F.Read(Code, CodeVSize);

	F.SetPos(DataRawOffset);
	F.Read(Data, DataVSize);

	char* Sign = "VExe";
	dword Ver = 2;

	dword Zer = 0;

	X = X.Left(X.Find('.', 0));
	X.Add(".bin");
	CFile F2(X._ptr(), ReadWrite);
	F2.Clear();
	F2.Write(PB(Sign), 4);             // 0
	F2.Write(PB(&Ver), 4);             // 4
	F2.Write(PB(&Zer), 4);             // 8
	F2.Write(PB(&CodeVSize), 4);       // 12
	F2.Write(PB(&DataVSize), 4);       // 16
	F2.Write(PB(&Priority), 2);        // 20
	F2.Write(PB(&AccessLevel), 2);     // 22
	F2.Write(Code, CodeVSize);
	F2.Write(Data, DataVSize);

	byte* Zu = new byte[CodeVSize + DataVSize];
	F2.SetPos(24);
	F2.Read(Zu, CodeVSize + DataVSize);

	dword CRC32 = CCRC32().GetCRC32(Zu, CodeVSize + DataVSize);
	F2.SetPos(8);
	F2.Write(PB(&CRC32), 4);             // 8
}