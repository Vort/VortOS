// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Renderer.cpp
#include "API.h"
#include "Array2.h"
#include "SmartPtr.h"

#include "Color.h"
#include "Rect.h"
#include "VideoStruct.h"

// ----------------------------------------------------------------------------
class CSurface
{
public:
	CSurface(dword Width, dword Height, dword SMID, dword SMOffset)
		: m_TrashColor(0)
	{
		SharedInit(Width, Height);
		m_Buf = KeMapSharedMem(SMID) + SMOffset;
	}

	CSurface(dword Width, dword Height)
		: m_TrashColor(0)
	{
		SharedInit(Width, Height);
		m_Buf = new byte[m_BS];
	}

	void SharedInit(dword Width, dword Height)
	{
		m_Width = Width;
		m_Height = Height;
		m_IsOwner = true;

		m_WH = Width * Height;
		m_BS = m_WH * 4;

		static dword g_UID = 0;
		m_UID = g_UID;
		g_UID++;
	}

	~CSurface()
	{
		if (m_IsOwner)
			delete m_Buf;
	}

	dword GetUID()
	{
		return m_UID;
	}

	void GetSize(dword& Width, dword& Height)
	{
		Width = m_Width;
		Height = m_Height;
	}

	void FillData(byte* Data, dword Size)
	{
		if (Size == 0)   return;
		if (Size > m_BS) return;
		for (dword i = 0; i < Size; i++)
			m_Buf[i] = Data[i];
	}

	void Fill(const C32bppColor& Color)
	{
		dword DColor = Color.ToDword();
		for (dword i = 0; i < m_WH; i++)
			(PD(m_Buf))[i] = DColor;
	}

	void DrawRect(const CRect& R, dword Color)
	{
		CRect TR;
		R.IntersectWith(CRect(0, 0, m_Width, m_Height), TR);

		dword Delta = TR.m_X + TR.m_Y * m_Width;
		dword* DstOfs = PD(m_Buf) + Delta;

		bool IsAlpha = (Color >> 24) != 0xFF;

		for (dword j = 0; j < TR.m_Height; j++)
		{
			if (IsAlpha)
			{
				for (dword i = 0; i < TR.m_Width; i++)
					((C32bppColor*)(&DstOfs[i]))->BlendWith(Color);
					//DstOfs[i] = C32bppColor(Color).AlphaBlend(DstOfs[i]).ToDword();
			}
			else
			{
				for (dword i = 0; i < TR.m_Width; i++)
					DstOfs[i] = Color;
			}
			DstOfs += m_Width;
		}
	}

	void EqBlit(CSurface& Dst, dword X, dword Y, dword Width, dword Height)
	{
		dword BlitW = Width;
		dword BlitH = Height;

		if (m_Width != Dst.m_Width)
			return;
		if (m_Height != Dst.m_Height)
			return;

		if (X + BlitW > m_Width)
			BlitW = m_Width - X;
		if (Y + BlitH > m_Height)
			BlitH = m_Height - Y;

		dword StartDelta = X + m_Width * Y;
		dword* DstBuf = PD(Dst.m_Buf) + StartDelta;
		dword* SrcBuf = PD(m_Buf) + StartDelta;
		for (dword y = 0; y < BlitH; y++)
		{
			for (dword x = 0; x < BlitW; x++)
				DstBuf[x] = SrcBuf[x];
			DstBuf += m_Width;
			SrcBuf += m_Width;
		}
	}

	void Blit(int SrcX, int SrcY, CSurface& Dst,
		int DstX, int DstY, dword Width, dword Height)
	{
		CRect R11(SrcX, SrcY, Width, Height);
		CRect R12(0, 0, m_Width, m_Height);
		CRect R13;
		if (!R11.IntersectWith(R12, R13)) return;
		CRect R21(DstX, DstY, Width, Height);
		CRect R22(0, 0, Dst.m_Width, Dst.m_Height);
		CRect R23;
		if (!R21.IntersectWith(R22, R23)) return;

		dword TrWidth = min(R13.m_Width, R23.m_Width);
		dword TrHeight = min(R13.m_Height, R23.m_Height);
		dword TrSrcX = max(SrcX, R13.m_X);
		dword TrSrcY = max(SrcY, R13.m_Y);
		dword TrDstX = max(DstX, R23.m_X);
		dword TrDstY = max(DstY, R23.m_Y);

		byte* OfsS = m_Buf + (TrSrcY * m_Width + TrSrcX) * 4;
		byte* OfsD = Dst.m_Buf + (TrDstY * Dst.m_Width + TrDstX) * 4;
		dword HDelta1 = (m_Width - TrWidth) * 4;
		dword HDelta2 = (Dst.m_Width - TrWidth) * 4;
		for (dword j = 0; j < TrHeight; j++)
		{
			for (dword i = 0; i < TrWidth; i++)
			{
				C32bppColor& DstC = *((C32bppColor*)OfsD);
				C32bppColor& SrcC = *((C32bppColor*)OfsS);
				DstC.BlendWith(SrcC);
				OfsS += 4;
				OfsD += 4;
			}
			OfsS += HDelta1;
			OfsD += HDelta2;
		}
	}

	C32bppColor& GetPixel(int X, int Y)
	{
		if (X < 0) return m_TrashColor;
		if (Y < 0) return m_TrashColor;
		if (X >= m_Width) return m_TrashColor;
		if (Y >= m_Height) return m_TrashColor;
		return *(C32bppColor*)(m_Buf + (X + Y * m_Width) * 4);
	}

private:
	byte* m_Buf;
	dword m_BS;
	dword m_WH;
	dword m_UID;
	dword m_Width;
	dword m_Height;
	C32bppColor m_TrashColor;
	bool m_IsOwner;
};

// ----------------------------------------------------------------------------
class CFontSurface
{
public:
	CFontSurface(dword Width, dword Height, dword SMID)
	{
		static dword g_UID = 0;
		m_UID = g_UID;
		g_UID++;

		m_Width = Width;
		m_Height = Height;

		dword WH = Width * Height;
		m_Buf = new byte[WH];

		byte* SMData = KeMapSharedMem(SMID);

		for (dword i = 0; i < WH; i++)
			m_Buf[i] = SMData[i];
	}

	~CFontSurface()
	{
		delete m_Buf;
	}

	void Blit(int SrcX, int SrcY, CSurface& Dst, int DstX, int DstY,
		dword Width, dword Height, C32bppColor Color)
	{
		if (Width == 0) return;
		if (Height == 0) return;
		for (dword i = 0; i < Width; i++)
			for (dword j = 0; j < Height; j++)
			{
				byte I = PGet(SrcX + i, SrcY + j);
				C32bppColor& Color1 = Dst.GetPixel(DstX + i, DstY + j);
				C32bppColor Color2(Color);
				Color2.SetAlpha(I);
				Color1.BlendWith(Color2);
			}
	}

	byte PGet(int X, int Y)
	{
		if (X < 0) return 0;
		if (Y < 0) return 0;
		if (X >= m_Width) return 0;
		if (Y >= m_Height) return 0;

		byte* Offset = m_Buf + X + Y * m_Width;
		return Offset[0];
	}

	dword GetID()
	{
		return m_UID;
	}

private:
	byte* m_Buf;
	dword m_UID;
	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
class CRenderer
{
public:
	CRenderer()
	{
		KeWaitForSymbol(SmVideo_OK);

		byte Caps[2];
		KeRequestCall(ClVideo_GetCaps, null, 0, Caps, 2);
		m_IsDoubleBuffering = Caps[0] == 1;
		m_IsDriverUpdate = Caps[1] == 1;

		dword OutBuf2[3];
		KeRequestCall(ClVideo_GetFrameSurface, null, 0, PB(OutBuf2), 12);

		dword FrontSMID = OutBuf2[0];
		dword Width = OutBuf2[1];
		dword Height = OutBuf2[2];

		m_FrontSurface = new CSurface(Width, Height, FrontSMID, 0);

		dword BackSMID;
		if (m_IsDoubleBuffering)
		{
			BackSMID = KeAllocSharedMem(Width * Height * 4);
			m_BackSurface = new CSurface(Width, Height, BackSMID, 0);
			m_BackSurface->Fill(0xFF000000);
		}

		KeEnableCallRequest(ClRenderer_CreateSurface);
		KeEnableCallRequest(ClRenderer_RemoveSurface);
		KeEnableCallRequest(ClRenderer_GetFrameSurface);
		KeEnableCallRequest(ClRenderer_SetSurfaceData);
		KeEnableCallRequest(ClRenderer_UpdateFrameSurface);
		KeEnableCallRequest(ClRenderer_BlitSurfaces);
		KeEnableCallRequest(ClRenderer_Wait);
		KeEnableCallRequest(ClRenderer_CreateFontSurface);
		KeEnableCallRequest(ClRenderer_BlitFontSurface);
		KeEnableCallRequest(ClRenderer_GetResolution);
		KeEnableNotification(NfRenderer_FillSurface);
		KeEnableNotification(NfRenderer_DrawRect);
		KeEnableNotification(NfKe_ProcessExited);

		CArray<dword> WaitCltPIDs;
		KeSetSymbol(SmRenderer_Ready);

		bool IsTerminating = false;
		CNotification<0x100> N;
		CCallRequest<0x1000> CR;

		for (;;)
		{
			KeWaitFor(3);
			dword NfCount;
			dword CallCount;
			KeGetNfClCount(NfCount, CallCount);

			for (dword z = 0; z < NfCount; z++)
			{
				N.Recv();
				if (N.GetID() == NfRenderer_FillSurface)
				{
					dword SurfID = N.GetDword(0);
					dword Color  = N.GetDword(1);
					FillSurface(SurfID, Color);
				}
				else if (N.GetID() == NfRenderer_DrawRect)
				{
					dword SurfID = N.GetDword(0);
					int X        = int(N.GetDword(1));
					int Y        = int(N.GetDword(2));
					dword W      = N.GetDword(3);
					dword H      = N.GetDword(4);
					dword Color  = N.GetDword(5);
					DrawRect(SurfID, CRect(X, Y, W, H), Color);
				}
				else if (N.GetID() == NfKe_ProcessExited)
				{
					if (KeIsSymbolSet(SmSurfMgr_Terminated))
						IsTerminating = true;
				}
			}

			for (dword z = 0; z < CallCount; z++)
			{
				CR.Recv();
				if (CR.GetTypeID() == ClRenderer_GetResolution)
				{
					dword OutBuf[2];
					OutBuf[0] = Width;
					OutBuf[1] = Height;
					CR.Respond(PB(OutBuf), 8);
				}
				else if (CR.GetTypeID() == ClRenderer_GetFrameSurface)
				{
					dword OutBuf[3];
					if (m_IsDoubleBuffering)
						OutBuf[0] = m_BackSurface->GetUID();
					else
						OutBuf[0] = m_FrontSurface->GetUID();
					m_FrontSurface->GetSize(OutBuf[1], OutBuf[2]);
					CR.Respond(PB(OutBuf), 12);
				}
				else if (CR.GetTypeID() == ClRenderer_CreateSurface)
				{
					dword Width  = CR.GetDword(0);
					dword Height = CR.GetDword(1);
					dword SurfID = CreateSurface(Width, Height);
					CR.Respond(SurfID);
				}
				else if (CR.GetTypeID() == ClRenderer_CreateFontSurface)
				{
					dword Width  = CR.GetDword(0);
					dword Height = CR.GetDword(1);
					dword SMID   = CR.GetDword(2);

					dword SurfID = CreateFontSurface(Width, Height, SMID);
					CR.Respond(SurfID);
				}
				else if (CR.GetTypeID() == ClRenderer_BlitFontSurface)
				{
					dword FontSurfID  = CR.GetDword(0);
					dword SurfID      = CR.GetDword(1);
					C32bppColor Color = CR.GetDword(2);
					dword SMID        = CR.GetDword(3);
					dword BlitCount   = CR.GetDword(4);

					CSmartPtr<CFontSurface> FS = GetFontSurfaceByUID(FontSurfID);
					CSmartPtr<CSurface> S = GetSurfaceByUID(SurfID);

					if ((!!FS) && (!!S))
					{
						CFontBlitInfo* FBI = (CFontBlitInfo*)KeMapSharedMem(SMID);
						for (dword i = 0; i < BlitCount; i++)
						{
							FS->Blit(FBI[i].m_SrcX, FBI[i].m_SrcY,
								*S, FBI[i].m_DstX, FBI[i].m_DstY,
								FBI[i].m_Width, FBI[i].m_Height, Color);
						}
						KeUnmapSharedMem(SMID);
					}
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClRenderer_RemoveSurface)
				{
					dword SurfID = CR.GetDword(0);
					for (dword i = 0; i < m_Surfaces.Size(); i++)
						if (m_Surfaces[i]->GetUID() == SurfID)
						{
							m_Surfaces.Delete(i);
							break;
						}
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClRenderer_SetSurfaceData)
				{
					dword SurfID = CR.GetDword(0);
					dword SMID   = CR.GetDword(1);
					dword TexDataSize = CR.GetDword(2);

					CSmartPtr<CSurface> Surf = GetSurfaceByUID(SurfID);
					if (!!Surf)
					{
						byte* TexData = KeMapSharedMem(SMID);
						Surf->FillData(TexData, TexDataSize);
					}
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClRenderer_UpdateFrameSurface)
				{
					if (m_IsDoubleBuffering)
					{
						dword UpdateInfoArrSMID   = CR.GetDword(0);
						dword UpdateInfoTopIndex  = CR.GetDword(1);

						CUpdateInfo* UI =
							(CUpdateInfo*)KeMapSharedMem(UpdateInfoArrSMID);

						for (dword i = 0; i < UpdateInfoTopIndex; i++)
						{
							UpdateFrameSurface(
								UI[i].m_X,
								UI[i].m_Y,
								UI[i].m_Width,
								UI[i].m_Height);
						}
					}
					if (m_IsDriverUpdate)
					{
						KeRequestCall(ClVideo_UpdateFrameSurface,
							CR.GetBuf(), 8, null, 0);
					}
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClRenderer_BlitSurfaces)
				{
					dword BlitInfoArrSMID   = CR.GetDword(0);
					dword BlitInfoTopIndex  = CR.GetDword(1);

					CBlitInfo* BI =
						(CBlitInfo*)KeMapSharedMem(BlitInfoArrSMID);

					dword PrevID1 = -1;
					dword PrevID2 = -1;

					CSmartPtr<CSurface> SrcS;
					CSmartPtr<CSurface> DstS;

					for (dword i = 0; i < BlitInfoTopIndex; i++)
					{
						dword SID1 = BI[i].m_SrcSID;
						dword SID2 = BI[i].m_DstSID;
						if (PrevID1 != SID1)
						{
							PrevID1 = SID1;
							SrcS = GetSurfaceByUID(SID1);
						}
						if (PrevID2 != SID2)
						{
							PrevID2 = SID2;
							DstS = GetSurfaceByUID(SID2);
						}

						BlitSurface(*SrcS, *DstS, BI[i].m_SrcX, BI[i].m_SrcY,
							BI[i].m_DstX, BI[i].m_DstY,
							BI[i].m_Width, BI[i].m_Height);
					}
					CR.Respond();
				}
				else if (CR.GetTypeID() == ClRenderer_Wait)
				{
					WaitCltPIDs.PushBack(CR.GetSrcPID());
				}
			}

			for (dword i = 0; i < WaitCltPIDs.Size(); i++)
				KeRespondCall2(WaitCltPIDs[i], null, 0);
			WaitCltPIDs.Clear();

			if (IsTerminating)
				return;
		}
	}

private:
	CSmartPtr<CFontSurface> GetFontSurfaceByUID(dword UID)
	{
		for (dword i = 0; i < m_FontSurfaces.Size(); i++)
			if (m_FontSurfaces[i]->GetID() == UID)
				return m_FontSurfaces[i];
		return null;
	}

	CSmartPtr<CSurface> GetSurfaceByUID(dword UID)
	{
		if (m_IsDoubleBuffering)
		{
			if (m_BackSurface->GetUID() == UID)
				return m_BackSurface;
		}
		else
		{
			if (m_FrontSurface->GetUID() == UID)
				return m_FrontSurface;
		}
		for (dword i = 0; i < m_Surfaces.Size(); i++)
			if (m_Surfaces[i]->GetUID() == UID)
				return m_Surfaces[i];
		return null;
	}

	dword CreateSurface(dword Width, dword Height)
	{
		CSmartPtr<CSurface> S = new CSurface(Width, Height);
		m_Surfaces.PushBack(S);
		return S->GetUID();
	}

	dword CreateFontSurface(dword Width, dword Height, dword SMID)
	{
		CSmartPtr<CFontSurface> FS = new CFontSurface(Width, Height, SMID);
		m_FontSurfaces.PushBack(FS);
		return FS->GetID();
	}

	void UpdateFrameSurface(dword X, dword Y, dword W, dword H)
	{
		//m_BackSurface->Blit(X, Y,
		//	*m_FrontSurface, X, Y, W, H);
		m_BackSurface->EqBlit(*m_FrontSurface, X, Y, W, H);
	}

	void BlitSurface(CSurface& SrcSurf, CSurface& DstSurf,
		int SrcX, int SrcY, int DstX, int DstY, dword Width, dword Height)
	{
		SrcSurf.Blit(SrcX, SrcY, DstSurf, DstX, DstY, Width, Height);
	}

	void FillSurface(dword SID, dword Color)
	{
		CSmartPtr<CSurface> S = GetSurfaceByUID(SID);
		if (!!S)
			S->Fill(Color);
	}

	void DrawRect(dword SID, CRect& R, dword Color)
	{
		CSmartPtr<CSurface> S = GetSurfaceByUID(SID);
		if (!!S)
			S->DrawRect(R, Color);
	}

	void FillSurfaceRect(dword SID, dword Color,
		int X, int Y, dword Width, dword Height)
	{
	}

private:
	bool m_IsDoubleBuffering;
	bool m_IsDriverUpdate;
	CSmartPtr<CSurface> m_FrontSurface;
	CSmartPtr<CSurface> m_BackSurface;
	CArray<CSmartPtr<CSurface> > m_Surfaces;
	CArray<CSmartPtr<CFontSurface> > m_FontSurfaces;
};

// ----------------------------------------------------------------------------
void Entry()
{
	CRenderer R;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=