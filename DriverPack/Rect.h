// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Rect.h
#include "Defs.h"
#include <MinMax.h>
#pragma once

// ----------------------------------------------------------------------------
class CRect
{
public:
	CRect()
	{
		m_X = 0;
		m_Y = 0;
		m_Width = 0;
		m_Height = 0;
	}

	CRect(int X, int Y, dword Width, dword Height)
	{
		m_X = X;
		m_Y = Y;
		m_Width = Width;
		m_Height = Height;
	}

	bool IsPtInRect(int X, int Y)
	{
		if (X < m_X) return false;
		if (Y < m_Y) return false;
		if (X > m_X + m_Width) return false;
		if (Y > m_Y + m_Height) return false;
		return true;
	}

	bool IntersectWith(const CRect& V2, CRect& Result) const
	{
		const CRect& V1 = *this;

		int X11 = V1.m_X;
		int Y11 = V1.m_Y;
		int X12 = V1.m_X + V1.m_Width;
		int Y12 = V1.m_Y + V1.m_Height;
		int X21 = V2.m_X;
		int Y21 = V2.m_Y;
		int X22 = V2.m_X + V2.m_Width;
		int Y22 = V2.m_Y + V2.m_Height;

		int X1 = max(min(X11, X12), min(X21, X22));
		int X2 = min(max(X11, X12), max(X21, X22));
		if (X1 > X2) return false;
		int Y1 = max(min(Y11, Y12), min(Y21, Y22));
		int Y2 = min(max(Y11, Y12), max(Y21, Y22));
		if (Y1 > Y2) return false;
		
		Result.m_X = X1;
		Result.m_Y = Y1;
		Result.m_Width = X2 - X1;
		Result.m_Height = Y2 - Y1;
		return true;
	}

	bool operator ==(const CRect& R)
	{
		if (m_X != R.m_X) return false;
		if (m_Y != R.m_Y) return false;
		if (m_Width != R.m_Width) return false;
		if (m_Height != R.m_Height) return false;
		return true;
	}

	int m_X;
	int m_Y;
	dword m_Width;
	dword m_Height;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=