// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// API.h
#pragma once
#include "KeCalls.h"
#include "Symbols.h"
#include "Notifications.h"
#include "UserCalls.h"
#include "VitrualKey.h"
#include "BitOp.h"
#include "NumericConverter.h"

// ----------------------------------------------------------------------------
extern "C" size_t strlen(const char *str);
#pragma intrinsic(strlen)

// ----------------------------------------------------------------------------
template <dword S>
class CNotification
{
public:
	void Recv()
	{
		if (!KeGetNotification(m_Buf, S, m_ID, m_Size, m_SrcPID))
		{
			m_ID = 0;
			m_Size = 0;
			m_SrcPID = 0;
		}
	}

	dword GetID()
	{
		return m_ID;
	}

	dword GetSize()
	{
		return m_Size;
	}

	dword GetSrcPID()
	{
		return m_SrcPID;
	}

	dword GetByte(dword ByteIndex)
	{
		return m_Buf[ByteIndex];
	}

	dword GetDword(dword DwordIndex)
	{
		return (PD(m_Buf))[DwordIndex];
	}

	byte* GetBuf()
	{
		return m_Buf;
	}

private:
	dword m_ID;
	dword m_Size;
	dword m_SrcPID;
	byte m_Buf[S];
};

// ----------------------------------------------------------------------------
template <dword S>
class CCallRequest
{
public:
	void Recv()
	{
		KeGetCallRequest(m_Buf, S, m_TypeID, m_Size, m_SrcPID);
	}

	void Respond(byte* RespBuf, dword RespBufSize)
	{
		KeRespondCall2(m_SrcPID, RespBuf, RespBufSize);
	}

	void Respond(dword Value)
	{
		KeRespondCall2(m_SrcPID, PB(&Value), 4);
	}

	void Respond(byte Value)
	{
		KeRespondCall2(m_SrcPID, &Value, 1);
	}

	void Respond()
	{
		KeRespondCall2(m_SrcPID, null, 0);
	}

	dword GetTypeID()
	{
		return m_TypeID;
	}

	dword GetSize()
	{
		return m_Size;
	}

	dword GetSrcPID()
	{
		return m_SrcPID;
	}

	dword GetByte(dword ByteIndex)
	{
		return m_Buf[ByteIndex];
	}

	dword GetDword(dword DwordIndex)
	{
		return (PD(m_Buf))[DwordIndex];
	}

	byte* GetBuf()
	{
		return m_Buf;
	}

private:
	dword m_TypeID;
	dword m_Size;
	dword m_SrcPID;
	byte m_Buf[S];
};

// ----------------------------------------------------------------------------
dword CHStoLBA(byte Cyl, byte Head, byte Sect)
{
	return ((Cyl * 2 + Head) * 18) + Sect - 1;
}

// ----------------------------------------------------------------------------
void LBAtoCHS(dword LBA, byte& Cyl, byte& Head, byte& Sect)
{
	dword CYL = LBA / (2 * 18);
	dword TEMP = LBA % (2 * 18);
	dword HEAD = TEMP / 18;
	dword SECT = TEMP % 18 + 1;

	Cyl = CYL & 0xFF;
	Head = HEAD & 0xFF;
	Sect = SECT & 0xFF;
}

// ----------------------------------------------------------------------------
void ByteToString(byte Byte, char* Buf)
{
	Buf[0] = '0';
	Buf[1] = '0';
	dword ByteLo = Byte & 0xF;
	dword ByteHi = Byte >> 4;
	if (ByteHi < 0xA)
		Buf[0] += ByteHi;
	else
		Buf[0] += 'A' - '0' + ByteHi - 10;

	if (ByteLo < 0xA)
		Buf[1] += ByteLo;
	else
		Buf[1] += 'A' - '0' + ByteLo - 10;
}

// ----------------------------------------------------------------------------
void WordToString(word Word, char* Buf)
{
	ByteToString((Word >> 8) & 0xFF, Buf);
	ByteToString(Word & 0xFF, Buf + 2);
}

// ----------------------------------------------------------------------------
void DebugOut(const char* str)
{
	KeNotify(Nf_DebugOut, (byte*)str, strlen(str));
}

// ----------------------------------------------------------------------------
void DebugOut(const char* str, int strLen)
{
	KeNotify(Nf_DebugOut, (byte*)str, strLen);
}

// ----------------------------------------------------------------------------
void DebugOutLine()
{
	char newLine = '\n';
	KeNotify(Nf_DebugOut, (byte*)&newLine, 1);
}

// ----------------------------------------------------------------------------
void DebugOutLine(const char* str)
{
	DebugOut(str);
	DebugOutLine();
}

// ----------------------------------------------------------------------------
void DebugOutDec(dword val)
{
	char buf[12];
	dword len = CNumericConverter().DwordToString(val, buf);
	KeNotify(Nf_DebugOut, (byte*)buf, len);
}

// ----------------------------------------------------------------------------
void DebugOutHex(byte Byte)
{
	char Buf[2];
	ByteToString(Byte, Buf);
	KeNotify(Nf_DebugOut, PB(Buf), 2);
}

// ----------------------------------------------------------------------------
void DebugOutHex(dword Dword)
{
	char Buf[8];
	WordToString(Dword >> 16, Buf);
	WordToString(Dword & 0xFFFF, Buf + 4);
	KeNotify(Nf_DebugOut, PB(Buf), 8);
}

// ----------------------------------------------------------------------------
dword GetFileSize(char* FileName)
{
	dword FileSize = 0;
	dword FileNameLen = strlen(FileName) + 1;
	KeRequestCall(ClFileSystem_GetFileSize, PB(FileName),
		FileNameLen, PB(&FileSize), 4);
	return FileSize;
}

// ----------------------------------------------------------------------------
void ReadFile(dword SMID, char* FileName)
{
	byte Buf[256];
	*PD(Buf) = SMID;

	dword i = 0;
	for (; FileName[i]; i++)
		Buf[4+i] = FileName[i];
	Buf[4+i] = 0;

	KeRequestCall(ClFileSystem_ReadFile, Buf, i + 4 + 1, null, 0);
}

// ----------------------------------------------------------------------------
dword FitText(dword Width, char* Text, dword TextSize)
{
	byte Buf[128];
	*PD(Buf) = Width;
	for (dword i = 0; i < TextSize; i++)
		Buf[4+i] = Text[i];

	dword FitCount = 0;
	KeRequestCall(ClFont_FitText, Buf, TextSize + 4, PB(&FitCount), 4);
	return FitCount;
}

// ----------------------------------------------------------------------------
void OutText(dword surfId, int x, int y, dword color, const wchar_t* text)
{
	byte textBuf[276];
	*(dword*)&textBuf[0] = surfId;
	*(dword*)&textBuf[4] = x;
	*(dword*)&textBuf[8] = y;
	*(dword*)&textBuf[12] = color;
	*(dword*)&textBuf[16] = 1;

	dword i = 0;
	for (; (i < 128) && (text[i] != L'\0'); i++)
		((wchar_t*)textBuf)[10 + i] = text[i];
	while (!KeNotify(NfSurfMgr_TextBlit, textBuf, 20 + i * 2))
		KeWaitTicks(0);
}

// ----------------------------------------------------------------------------
void OutText(dword surfId, int x, int y, dword color, const char* text)
{
	byte textBuf[148];
	*(dword*)&textBuf[0] = surfId;
	*(dword*)&textBuf[4] = x;
	*(dword*)&textBuf[8] = y;
	*(dword*)&textBuf[12] = color;
	*(dword*)&textBuf[16] = 0;

	dword i = 0;
	for (; (i < 128) && (text[i] != '\0'); i++)
		textBuf[20 + i] = text[i];
	while (!KeNotify(NfSurfMgr_TextBlit, textBuf, 20 + i))
		KeWaitTicks(0);
}

// ----------------------------------------------------------------------------
void OutText(dword SurfID, int X, int Y, dword Color, const char* Text, dword TextSize)
{
	char Buf[256];
	for (dword i = 0; i < TextSize; i++)
		Buf[i] = Text[i];
	Buf[TextSize] = 0;

	OutText(SurfID, X, Y, Color, Buf);
}

// ----------------------------------------------------------------------------
void WaitRedraw()
{
	KeRequestCall(ClSurfMgr_WaitRedraw, null, 0, null, 0);
}

// ----------------------------------------------------------------------------
void GetResolution(dword& Width, dword& Height)
{
	byte Resolution[8];
	KeRequestCall(ClRenderer_GetResolution, null, 0, Resolution, 8);
	Width = *PD(&Resolution[0]);
	Height = *PD(&Resolution[4]);
}

// ----------------------------------------------------------------------------
dword CreateSurface(int X, int Y, dword Width, dword Height, dword Layer)
{
	byte CallBuf[20];
	*PD(&CallBuf[0]) = X;
	*PD(&CallBuf[4]) = Y;
	*PD(&CallBuf[8]) = Width;
	*PD(&CallBuf[12]) = Height;
	*PD(&CallBuf[16]) = Layer;
	dword SurfaceID = 0;

	KeRequestCall(ClSurfMgr_CreateSurface, CallBuf, 20, PB(&SurfaceID), 4);
	return SurfaceID;
}

// ----------------------------------------------------------------------------
dword CreateSurface(int X, int Y, dword Width, dword Height)
{
	return CreateSurface(X, Y, Width, Height, 1);
}

// ----------------------------------------------------------------------------
void SetSurfaceData(dword SurfaceID, dword SMID, dword Size)
{
	dword CursorTextureRequest[3];
	CursorTextureRequest[0] = SurfaceID;
	CursorTextureRequest[1] = SMID;
	CursorTextureRequest[2] = Size;
	KeRequestCall(ClSurfMgr_SetSurfaceData,
		PB(CursorTextureRequest), 12, null, 0);
}

// ----------------------------------------------------------------------------
void DrawRect(dword SurfID, int X, int Y, dword W, dword H, dword Color)
{
	byte NfBuf[24];
	*PD(&NfBuf[0]) = SurfID;
	*PD(&NfBuf[4]) = X;
	*PD(&NfBuf[8]) = Y;
	*PD(&NfBuf[12]) = W;
	*PD(&NfBuf[16]) = H;
	*PD(&NfBuf[20]) = Color;
	KeNotify(NfSurfMgr_DrawRect, NfBuf, 24);
}

// ----------------------------------------------------------------------------
void DrawFrameRect(dword SurfID, int X, int Y, dword W, dword H, dword Color)
{
	// TODO: 1. Check it. 2. Rewrite it
	DrawRect(SurfID, X, Y, W, 1, Color);
	DrawRect(SurfID, X, Y, 1, H, Color);
	DrawRect(SurfID, X, Y + H - 1, W, 1, Color);
	DrawRect(SurfID, X + W - 1, Y, 1, H, Color);
}

// ----------------------------------------------------------------------------
void MoveSurface(dword SurfID, int X, int Y)
{
	dword NfBuf[3];
	NfBuf[0] = SurfID;
	NfBuf[1] = X;
	NfBuf[2] = Y;
	KeNotify(NfSurfMgr_MoveSurface, PB(NfBuf), 12);
}

// ----------------------------------------------------------------------------
void ShowSurface(dword SurfID)
{
	KeNotify(NfSurfMgr_ShowSurface, PB(&SurfID), 4);
}

// ----------------------------------------------------------------------------
void FillSurface(dword SurfID, dword Color)
{
	byte NfBuf[8];
	*PD(&NfBuf[0]) = SurfID;
	*PD(&NfBuf[4]) = Color;
	KeNotify(NfSurfMgr_FillSurface, NfBuf, 8);
}

// ----------------------------------------------------------------------------
byte ReadPCIConfByte(byte bus, byte device, byte function, byte regOffset)
{
	byte regVal = 0;
	byte reqBuf[4] = {bus, device, function, regOffset};
	KeRequestCall(ClPCI_ReadConfByte, reqBuf, 4, (byte*)(&regVal), 1);
	return regVal;
}

// ----------------------------------------------------------------------------
word ReadPCIConfWord(byte bus, byte device, byte function, byte regOffset)
{
	word regVal = 0;
	byte reqBuf[4] = {bus, device, function, regOffset};
	KeRequestCall(ClPCI_ReadConfWord, reqBuf, 4, (byte*)(&regVal), 2);
	return regVal;
}

// ----------------------------------------------------------------------------
dword ReadPCIConfDword(byte bus, byte device, byte function, byte regOffset)
{
	dword regVal = 0;
	byte reqBuf[4] = {bus, device, function, regOffset};
	KeRequestCall(ClPCI_ReadConfDword, reqBuf, 4, (byte*)(&regVal), 4);
	return regVal;
}

// ----------------------------------------------------------------------------
void WritePCIConfByte(byte bus, byte device, byte function, byte regOffset, byte regValue)
{
	byte reqBuf[5] = {bus, device, function, regOffset, regValue};
	KeNotify(NfPCI_WriteConfByte, reqBuf, 5);
}

// ----------------------------------------------------------------------------
void WritePCIConfWord(byte bus, byte device, byte function, byte regOffset, word regValue)
{
	byte reqBuf[6] = {bus, device, function, regOffset, regValue & 0xFF, regValue >> 8};
	KeNotify(NfPCI_WriteConfWord, reqBuf, 6);
}

// ----------------------------------------------------------------------------
bool GetPCIDeviceByID(word VendorID, word DeviceID,
					  byte& Bus, byte& Device, byte& Function)
{
	byte LocBuf[3];
	dword ReqBuf[2];
	dword RespSize = 0;
	ReqBuf[0] = VendorID;
	ReqBuf[1] = DeviceID;
	KeRequestCall(ClPCI_GetDeviceByID, PB(ReqBuf), 8, LocBuf, 3, RespSize);
	if (RespSize == 0)
		return false;

	Bus = LocBuf[0];
	Device = LocBuf[1];
	Function = LocBuf[2];
	return true;
}

// ----------------------------------------------------------------------------
bool GetEmptyNotification()
{
	dword NotificationID = 0;
	dword NotificationSize = 0;
	dword SrcPID = 0;
	return KeGetNotification(null, 0,
		NotificationID, NotificationSize, SrcPID);
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=