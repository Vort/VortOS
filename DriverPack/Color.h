// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Color.h
#include "Defs.h"
#pragma once

// ----------------------------------------------------------------------------
class C32bppColor
{
public:
	C32bppColor(byte R, byte G, byte B, byte A)
	{
		m_B = B;
		m_G = G;
		m_R = R;
		m_A = A;
	}

	C32bppColor(dword Color)
	{
		m_Color = Color;
	}

	C32bppColor AlphaBlend(const C32bppColor& Color)
	{
		// Result = This (+) Color

		dword A2 = Color.m_A;
		if (A2 == 0xFF)
			return Color;
		if (A2 == 0x00)
			return *this;

		dword M  = 255 - A2;

		dword B1 = m_B;
		dword G1 = m_G;
		dword R1 = m_R;
		dword A1 = m_A;
		dword B2 = Color.m_B;
		dword G2 = Color.m_G;
		dword R2 = Color.m_R;

		dword B3 = (B2 * A2 + B1 * M) / 255;
		dword G3 = (G2 * A2 + G1 * M) / 255;
		dword R3 = (R2 * A2 + R1 * M) / 255;
		dword A3 = A1 + A2 - (A1 * A2) / 255;

		return C32bppColor(R3, G3, B3, A3);
	}

	void BlendWith(const C32bppColor& Color)
	{
		// This = This (+) Color

		if (Color.m_A == 0xFF)
		{
			m_Color = Color.m_Color;
			return;
		}
		if (Color.m_A == 0x00)
			return;

		byte M  = 255 - Color.m_A;
		m_R = (Color.m_R * Color.m_A + m_R * M) / 255;
		m_G = (Color.m_G * Color.m_A + m_G * M) / 255;
		m_B = (Color.m_B * Color.m_A + m_B * M) / 255;
		if (m_A != 0xFF)
			m_A += Color.m_A - (m_A * Color.m_A) / 255;
	}

	void SetAlpha(byte A)
	{
		m_A = A;
	}

	dword ToDword() const
	{
		return m_Color;
	}

private:
	union
	{
		dword m_Color;
		struct
		{
			byte m_B;
			byte m_G;
			byte m_R;
			byte m_A;
		};
	};
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=