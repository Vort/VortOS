// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// CRC32.h
//
// v 1.00 (2008.10.09)
//
#pragma once
#include "Defs.h"
#include "Array2.h"

// ----------------------------------------------------------------------------
namespace Lib {
// ----------------------------------------------------------------------------
class CCRC32
{
public:
	CCRC32()
	{
		dword c;
		int n, k;
		dword poly;

		static byte p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

		poly = 0L;
		for (n = 0; n < sizeof(p)/sizeof(byte); n++)
			poly |= 1L << (31 - p[n]);

		for (n = 0; n < 256; n++)
		{
			c = (dword)n;
			for (k = 0; k < 8; k++)
				c = c & 1 ? poly ^ (c >> 1) : c >> 1;
			m_Table[n] = c;
		}
	}

	dword GetCRC32(byte* Data, dword Size)
	{
		dword CRC32 = -1;
		for (dword i = 0; i < Size; i++)
			CRC32 = m_Table[(CRC32 ^ Data[i]) & 0xff] ^ (CRC32 >> 8);
		return CRC32 ^ (-1);
	}

private:
	dword m_Table[256];
};
// ----------------------------------------------------------------------------
} // namespace Lib
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=