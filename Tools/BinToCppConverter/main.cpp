#pragma comment(linker, "-entry:main")
#include "Defs.h"
#include "File.h"
#include "String2.h"
#include "DigitConvert.h"

#include <windows.h>
using namespace Lib;

void main()
{
	CStringA S = GetCommandLineA();
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

	X = X.Left(X.Find('.', 0));
	CStringA Z = X;
	X.Add(".h");
	CStringA XX("_");
	XX.Add(X);
	CFile F2(XX._ptr(), Write);
	F2.Clear();

	CStringA D("// ");
	D.Add(XX);
	D.Add("\r\n\r\n");
	D.Add("static byte ");
	D.Add(Z);
	D.Add("Image[");
	dword FS = F.GetSize();
	D.Add(CDigitConvert().DWordToString(FS));
	D.Add("] =\r\n");
	D.Add("{\r\n");
	for (dword i = 0; i < FS; i++)
	{
		byte B;
		F.Read(&B, 1);
		D.Add(CDigitConvert().DWordToString(B));
		if (i != (FS-1))
			D.Add(", ");
		if ((i % 16 == 0) && (i != 0))
			D.Add("\r\n");
	}
	D.Add("\r\n");
	D.Add("};");

	F2.Write(PB(D._ptr()), D.Len());
}