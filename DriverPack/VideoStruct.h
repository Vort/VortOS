// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// VideoStruct.h
#include "Defs.h"
#pragma once

// ----------------------------------------------------------------------------
class CFontBlitTableEntry
{
public:
	dword m_X;
	dword m_Y;
	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
class CUpdateInfo
{
public:
	CUpdateInfo(dword X, dword Y, dword Width, dword Height)
	{
		m_X = X;
		m_Y = Y;
		m_Width = Width;
		m_Height = Height;
	}

public:
	dword m_X;
	dword m_Y;
	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
class CBlitInfo
{
public:
	CBlitInfo(dword SrcSID, dword DstSID, int SrcX, int SrcY,
		int DstX, int DstY, dword Width, dword Height)
	{
		m_SrcSID = SrcSID;
		m_DstSID = DstSID;
		m_SrcX = SrcX;
		m_SrcY = SrcY;
		m_DstX = DstX;
		m_DstY = DstY;
		m_Width = Width;
		m_Height = Height;
	}

public:
	dword m_SrcSID;
	dword m_DstSID;
	int m_SrcX;
	int m_SrcY;
	int m_DstX;
	int m_DstY;
	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
class CFontBlitInfo
{
public:
	int m_SrcX;
	int m_SrcY;
	int m_DstX;
	int m_DstY;
	dword m_Width;
	dword m_Height;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=