// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SurfMgr.cpp
#include "API.h"
#include "Array2.h"
#include "SmartPtr.h"
#include "SMArray.h"
#include "VideoStruct.h"

// ----------------------------------------------------------------------------
class CSurface
{
public:
	CSurface(dword SurfaceID, dword Width, dword Height,
		int X, int Y, dword Layer)
	{
		m_X = X;
		m_Y = Y;
		m_Width = Width;
		m_Height = Height;
		m_Layer = Layer;
		m_IsVisible = false;

		m_UID = SurfaceID;
		m_SrcPID = 0;
	}

	CSurface(dword Width, dword Height, int X, int Y, dword Layer, dword SrcPID)
	{
		m_X = X;
		m_Y = Y;
		m_Width = Width;
		m_Height = Height;
		m_Layer = Layer;
		m_IsVisible = false;
		m_SrcPID = SrcPID;

		dword InBuf[2];
		InBuf[0] = Width;
		InBuf[1] = Height;
		KeRequestCall(ClRenderer_CreateSurface, PB(InBuf), 8, PB(&m_UID), 4);
	}

	~CSurface()
	{
		KeRequestCall(ClRenderer_RemoveSurface, PB(&m_UID), 4, null, 0);
	}

	dword GetID()
	{
		return m_UID;
	}

	dword GetSrcPID()
	{
		return m_SrcPID;
	}

	bool IsVisible()
	{
		return m_IsVisible;
	}

	void Show()
	{
		m_IsVisible = true;
	}

	dword GetLayer()
	{
		return m_Layer;
	}

	void GetSize(dword& Width, dword& Height)
	{
		Width = m_Width;
		Height = m_Height;
	}

	void GetPosition(int& X, int& Y)
	{
		X = m_X;
		Y = m_Y;
	}

	void SetPosition(int X, int Y)
	{
		m_X = X;
		m_Y = Y;
	}

	void Fill(dword Color)
	{
		dword InBuf[2];
		InBuf[0] = m_UID;
		InBuf[1] = Color;
		KeNotify(NfRenderer_FillSurface, PB(InBuf), 8);
	}

	void DrawRect(int X, int Y, dword W, dword H, dword Color)
	{
		dword InBuf[6];
		InBuf[0] = m_UID;
		InBuf[1] = dword(X);
		InBuf[2] = dword(Y);
		InBuf[3] = W;
		InBuf[4] = H;
		InBuf[5] = Color;
		KeNotify(NfRenderer_DrawRect, PB(InBuf), 4*6);
	}

	void FillData(dword SMID, dword Size)
	{
		dword InBuf[3];
		InBuf[0] = m_UID;
		InBuf[1] = SMID;
		InBuf[2] = Size;
		KeRequestCall(ClRenderer_SetSurfaceData, PB(InBuf), 12, null, 0);
	}

private:
	dword m_UID;
	dword m_Layer;
	dword m_SrcPID;
	bool m_IsVisible;

	int m_X;
	int m_Y;
	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
class CSurfUpdateInfo
{
public:
	CSurfUpdateInfo(dword SurfID, int SrcX, int SrcY, int DstX, int DstY)
	{
		m_SrcX = SrcX;
		m_SrcY = SrcY;
		m_DstX = DstX;
		m_DstY = DstY;
		m_SurfID = SurfID;
	}

	void GetUpdatePoints(int& SrcX, int& SrcY, int& DstX, int& DstY)
	{
		SrcX = m_SrcX;
		SrcY = m_SrcY;
		DstX = m_DstX;
		DstY = m_DstY;
	}

	void SetDstPoint(int X, int Y)
	{
		m_DstX = X;
		m_DstY = Y;
	}

	dword GetSurfID()
	{
		return m_SurfID;
	}

private:
	int m_SrcX;
	int m_SrcY;
	int m_DstX;
	int m_DstY;
	dword m_SurfID;
};

// ----------------------------------------------------------------------------
class CSurfaceManager
{
public:
	CSurfaceManager()
		: m_UpdateSMArray(1024), m_BlitSMArray(1024)
	{
		KeWaitForSymbol(SmRenderer_Ready);

		m_CursorX = -1;
		m_CursorY = -1;
		m_ActiveSurfaceID = -1;

		dword ID;
		GetFrameSurface(ID, m_Width, m_Height);
		m_FrameSurface = new CSurface(ID, m_Width, m_Height, 0, 0, 0);

		m_DirtyQ = 8;
		KeRequestCall(ClVideo_GetQuantSize, null, 0, PB(&m_DirtyQ), 4);

		m_DirtyW = (m_Width + m_DirtyQ - 1) / m_DirtyQ;
		m_DirtyH = (m_Height + m_DirtyQ - 1) / m_DirtyQ;
		m_Dirty = new bool[m_DirtyW * m_DirtyH];
		m_DirtyTemp = new bool[m_DirtyW * m_DirtyH];

		KeEnableCallRequest(ClSurfMgr_CreateSurface);
		KeEnableCallRequest(ClSurfMgr_SetSurfaceData);
		KeEnableCallRequest(ClSurfMgr_WaitRedraw);
		KeEnableCallRequest(ClSurfMgr_SetFont);
		KeEnableNotification(NfSurfMgr_ShowSurface);
		KeEnableNotification(NfSurfMgr_MoveSurface);
		KeEnableNotification(NfSurfMgr_FillSurface);
		KeEnableNotification(NfSurfMgr_DrawRect);
		KeEnableNotification(NfSurfMgr_TextBlit);
		KeEnableNotification(Nf_MouseButtonDown);
		KeEnableNotification(Nf_CursorMoveTo);
		KeEnableNotification(NfKe_ProcessExited);

		dword FontSurfaceID = 0;
		dword BlitTableSMID = 0;
		CFontBlitTableEntry* FontBlitTable = null;

		dword BlitSMID = KeAllocSharedMem(170 * sizeof(CFontBlitInfo));
		CFontBlitInfo* FontBlitInfo = (CFontBlitInfo*)KeMapSharedMem(BlitSMID);

		CArray<dword> WaitCltPIDs;
		KeSetSymbol(SmSurfMgr_Ready);

		bool IsTerminating = false;

		dword NfCount = 0;
		dword CallCount = 0;
		CNotification<0x1000> N;
		CCallRequest<0x100> CR;
		for (;;)
		{
			ClearDirty(false);
			m_SurfUpdateInfos.Clear();

			KeWaitFor(3);
			for (;;)
			{
				KeGetNfClCount(NfCount, CallCount);
				if ((NfCount == 0) && (CallCount == 0))
					break;

				for (dword z = 0; z < CallCount; z++)
				{
					CR.Recv();
					if (CR.GetTypeID() == ClSurfMgr_CreateSurface)
					{
						int X        = int(CR.GetDword(0));
						int Y        = int(CR.GetDword(1));
						dword Width  = CR.GetDword(2);
						dword Height = CR.GetDword(3);
						dword Layer  = CR.GetDword(4);

						CSmartPtr<CSurface> S = new CSurface(
							Width, Height, X, Y, Layer, CR.GetSrcPID());
						m_Surfaces.PushBack(S);
						if (Layer == 1)
						{
							m_ActiveSurfaceID = S->GetID();
							KeNotify(Nf_SurfaceActivated, (byte*)&m_ActiveSurfaceID, 4);
						}

						CR.Respond(S->GetID());
					}
					else if (CR.GetTypeID() == ClSurfMgr_SetSurfaceData)
					{
						dword SurfID = CR.GetDword(0);
						dword SMID   = CR.GetDword(1);
						dword Size   = CR.GetDword(2);

						CSmartPtr<CSurface> Surf = GetSurfaceByID(SurfID);
						if (!!Surf)
						{
							Surf->FillData(SMID, Size);
							if (Surf->IsVisible())
							{
								int X, Y;
								Surf->GetPosition(X, Y);
								AddSurfUpdateInfo(Surf->GetID(), X, Y);
							}
						}
						CR.Respond();
					}
					else if (CR.GetTypeID() == ClSurfMgr_SetFont)
					{
						KeRequestCall(ClRenderer_CreateFontSurface,
							CR.GetBuf(), 12, PB(&FontSurfaceID), 4);
						BlitTableSMID = CR.GetDword(3);
						FontBlitTable = (CFontBlitTableEntry*)
							KeMapSharedMem(BlitTableSMID);
						CR.Respond();
					}
					else if (CR.GetTypeID() == ClSurfMgr_WaitRedraw)
					{
						WaitCltPIDs.PushBack(CR.GetSrcPID());
					}
				}

				for (dword z = 0; z < NfCount; z++)
				{
					N.Recv();
					if (N.GetID() == Nf_MouseButtonDown)
					{
						dword newActivatedSurfID = -1;
						for (int i = 0; i < m_Surfaces.Size(); i++)
						{
							CSmartPtr<CSurface> surf = m_Surfaces[i];
							if (surf->GetLayer() != 1)
								continue;

							int surfX;
							int surfY;
							dword surfW;
							dword surfH;
							surf->GetPosition(surfX, surfY);
							surf->GetSize(surfW, surfH);
							if (m_CursorX < surfX)
								continue;
							if (m_CursorY < surfY)
								continue;
							if (m_CursorX >= surfX + surfW)
								continue;
							if (m_CursorY >= surfY + surfH)
								continue;

							newActivatedSurfID = surf->GetID();
						}
						if ((newActivatedSurfID != -1) &&
							(newActivatedSurfID != m_ActiveSurfaceID))
						{
							m_ActiveSurfaceID = newActivatedSurfID;
							KeNotify(Nf_SurfaceActivated, (byte*)&m_ActiveSurfaceID, 4);
							// HACK: Пока окно неактивно, нажатия не принимаются -> повторяем отправку после активации
							KeNotify(Nf_MouseButtonDown, N.GetBuf(), 1);
						}
					}
					else if (N.GetID() == Nf_CursorMoveTo)
					{
						m_CursorX = N.GetDword(0);
						m_CursorY = N.GetDword(1);
					}
					else if (N.GetID() == NfKe_ProcessExited)
					{
						if (KeIsSymbolSet(SmDesktop_Terminated))
							IsTerminating = true;

						dword ExPID = N.GetDword(0);
						bool IsClear = false;
						while (!IsClear)
						{
							IsClear = true;
							for (dword i = 0; i < m_Surfaces.Size(); i++)
								if (m_Surfaces[i]->GetSrcPID() == ExPID)
								{
									int X, Y;
									dword W, H;
									m_Surfaces[i]->GetPosition(X, Y);
									m_Surfaces[i]->GetSize(W, H);
									AddDirty(X, Y, W, H, false);
									m_Surfaces.Delete(i);
									IsClear = false;
								}
						}
					}
					else if (N.GetID() == NfSurfMgr_ShowSurface)
					{
						dword SurfID = N.GetDword(0);
						CSmartPtr<CSurface> Surf = GetSurfaceByID(SurfID);
						if (!!Surf)
						{
							int X, Y;
							Surf->GetPosition(X, Y);
							AddSurfUpdateInfo(SurfID, X, Y);
							Surf->Show();
						}
					}
					else if (N.GetID() == NfSurfMgr_MoveSurface)
					{
						dword SurfID = N.GetDword(0);
						int X        = int(N.GetDword(1));
						int Y        = int(N.GetDword(2));
						CSmartPtr<CSurface> Surf = GetSurfaceByID(SurfID);
						if (!!Surf)
						{
							int oX;
							int oY;
							Surf->GetPosition(oX, oY);
							AddSurfUpdateInfo(SurfID, oX, oY);
							Surf->SetPosition(X, Y);
							AddSurfUpdateInfo(SurfID, X, Y);
						}
					}
					else if (N.GetID() == NfSurfMgr_FillSurface)
					{
						dword SurfID = N.GetDword(0);
						dword Color = N.GetDword(1);
						CSmartPtr<CSurface> Surf = GetSurfaceByID(SurfID);
						if (!!Surf)
						{
							Surf->Fill(Color);
							if (Surf->IsVisible())
							{
								int X, Y;
								Surf->GetPosition(X, Y);
								AddSurfUpdateInfo(Surf->GetID(), X, Y);
							}
						}
					}
					else if (N.GetID() == NfSurfMgr_DrawRect)
					{
						dword SurfID = N.GetDword(0);
						int X        = int(N.GetDword(1));
						int Y        = int(N.GetDword(2));
						dword W      = N.GetDword(3);
						dword H      = N.GetDword(4);
						dword Color  = N.GetDword(5);

						CSmartPtr<CSurface> Surf = GetSurfaceByID(SurfID);
						if (!!Surf)
						{
							Surf->DrawRect(X, Y, W, H, Color);
							if (Surf->IsVisible())
							{
								int sX, sY;
								Surf->GetPosition(sX, sY);
								AddDirty(sX + X, sY + Y, W, H, false);
							}
						}
					}
					else if (N.GetID() == NfSurfMgr_TextBlit)
					{
						dword SurfID = N.GetDword(0);
						dword X      = N.GetDword(1);
						dword Y      = N.GetDword(2);
						dword Color  = N.GetDword(3);

						CSmartPtr<CSurface> S = GetSurfaceByID(SurfID);

						if (!!S)
						{
							int SX, SY;
							S->GetPosition(SX, SY);

							char* Text = PC(N.GetBuf() + 16);
							//dword TextSize = strlen(Text);
							dword TextSize = N.GetSize() - 16;

							dword OutBuf[5];
							OutBuf[0] = FontSurfaceID;
							OutBuf[1] = SurfID;
							OutBuf[2] = Color;
							OutBuf[3] = BlitSMID;
							OutBuf[4] = TextSize;

							dword DstX = X;
							for (dword i = 0; i < TextSize; i++)
							{
								char c = Text[i];
								FontBlitInfo[i].m_SrcX = FontBlitTable[c].texSrcX;
								FontBlitInfo[i].m_SrcY = 0;
								FontBlitInfo[i].m_DstX = DstX + FontBlitTable[c].offsetX;
								FontBlitInfo[i].m_DstY = Y + FontBlitTable[c].offsetY;
								FontBlitInfo[i].m_Width = FontBlitTable[c].bitmapWidth;
								FontBlitInfo[i].m_Height = FontBlitTable[c].bitmapHeight;
								DstX += FontBlitTable[c].advanceX;

								AddDirty(
									SX + FontBlitInfo[i].m_DstX,
									SY + FontBlitInfo[i].m_DstY,
									FontBlitInfo[i].m_Width,
									FontBlitInfo[i].m_Height,
									false);
							}

							KeRequestCall(ClRenderer_BlitFontSurface,
								PB(OutBuf), 20, null, 0);
						}
					}
				}
			}

			dword W, H;
			int X1, Y1, X2, Y2;
			for (dword i = 0; i < m_SurfUpdateInfos.Size(); i++)
			{
				CSmartPtr<CSurface> S = GetSurfaceByID(
					m_SurfUpdateInfos[i].GetSurfID());
				if (!!S)
				{
					S->GetSize(W, H);
					m_SurfUpdateInfos[i].GetUpdatePoints(X1, Y1, X2, Y2);
					AddDirty(X1, Y1, W, H, false);
					if ((X1 != X2) || (Y1 != Y2))
						AddDirty(X2, Y2, W, H, false);
				}
			}
			BlitDirty();
			WaitRedraw();

			for (dword i = 0; i < WaitCltPIDs.Size(); i++)
				KeRespondCall2(WaitCltPIDs[i], null, 0);
			WaitCltPIDs.Clear();

			if (IsTerminating)
				return;
		}
	}

private:
	void WaitRedraw()
	{
		KeRequestCall(ClRenderer_Wait, null, 0, null, 0);
	}

	void ClearDirty(bool IsTemp)
	{
		dword S = m_DirtyH * m_DirtyW;
		bool* Dirty = m_Dirty;
		if (IsTemp)
			Dirty = m_DirtyTemp;
		for (dword i = 0; i < S; i++)
			Dirty[i] = false;
	}

	void AddDirty(int X, int Y, dword W, dword H, bool IsTemp)
	{
		if (W == 0) return;
		if (H == 0) return;

		int FX = X + W - 1;
		int FY = Y + H - 1;
		dword AlSX = X / m_DirtyQ;
		dword AlSY = Y / m_DirtyQ;
		dword AlFX = FX / m_DirtyQ;
		dword AlFY = FY / m_DirtyQ;

		if (X < 0) AlSX = 0;
		if (Y < 0) AlSY = 0;
		if (FX < 0) AlFX = 0;
		if (FY < 0) AlFY = 0;

		if (AlSX >= m_DirtyW) AlSX = m_DirtyW - 1;
		if (AlSY >= m_DirtyH) AlSY = m_DirtyH - 1;
		if (AlFX >= m_DirtyW) AlFX = m_DirtyW - 1;
		if (AlFY >= m_DirtyH) AlFY = m_DirtyH - 1;

		bool* Dirty = m_Dirty;
		if (IsTemp)
			Dirty = m_DirtyTemp;

		for (dword i = AlSX; i < AlFX + 1; i++)
			for (dword j = AlSY; j < AlFY + 1; j++)
				Dirty[i + m_DirtyW*j] = true;
	}

	void MulDirty()
	{
		dword DirWH = m_DirtyH * m_DirtyW;
		for (dword i = 0; i < DirWH; i++)
			m_DirtyTemp[i] &= m_Dirty[i];

	}

	void BlitDirty()
	{
		dword WH = m_Width * m_Height;
		dword SurfCount = m_Surfaces.Size();

		for (dword l = 0; l < 3; l++)
		{
			for (dword k = 0; k < SurfCount; k++)
			{
				if ((m_Surfaces[k]->GetLayer() == l) &&
					m_Surfaces[k]->IsVisible())
				{
					int SPX, SPY;
					dword SW, SH;
					m_Surfaces[k]->GetPosition(SPX, SPY);
					m_Surfaces[k]->GetSize(SW, SH);

					ClearDirty(true);
					AddDirty(SPX, SPY, SW, SH, true);
					MulDirty();

					dword DirtyIndex = 0;
					for (dword j = 0; j < m_DirtyH; j++)
						for (dword i = 0; i < m_DirtyW; i++)
						{
							if (m_DirtyTemp[DirtyIndex])
							{
								dword MX = i * m_DirtyQ;
								dword MY = j * m_DirtyQ;
								int DX = int(MX) - SPX;
								int DY = int(MY) - SPY;
								if (DX < 0) MX -= DX;
								if (DY < 0) MY -= DY;
								CBlitInfo BI(m_Surfaces[k]->GetID(),
									m_FrameSurface->GetID(), DX, DY,
									MX, MY, m_DirtyQ, m_DirtyQ);
								BlitSurface(BI);
							}
							DirtyIndex++;
						}
					}
				}
			}

			BlitFlush();

			dword DirtyIndex = 0;
			for (dword j = 0; j < m_DirtyH; j++)
				for (dword i = 0; i < m_DirtyW; i++)
				{
					if (m_Dirty[DirtyIndex])
					{
						CUpdateInfo UI(i * m_DirtyQ,
							j * m_DirtyQ, m_DirtyQ, m_DirtyQ);
						UpdateFrameSurface(UI);
					}
					DirtyIndex++;
				}
			UpdateFlush();
	}

	void AddSurfUpdateInfo(dword SurfID, int X, int Y)
	{
		dword FoundIndex = -1;
		for (dword i = 0; i < m_SurfUpdateInfos.Size(); i++)
			if (m_SurfUpdateInfos[i].GetSurfID() == SurfID)
			{
				FoundIndex = i;
				break;
			}

		if (FoundIndex == -1)
			m_SurfUpdateInfos.PushBack(CSurfUpdateInfo(SurfID, X, Y, X, Y));
		else
			m_SurfUpdateInfos[FoundIndex].SetDstPoint(X, Y);
	}

	CSmartPtr<CSurface> GetSurfaceByID(dword ID)
	{
		for (dword i = 0; i < m_Surfaces.Size(); i++)
			if (m_Surfaces[i]->GetID() == ID)
				return m_Surfaces[i];
		return null;
	}

	void GetFrameSurface(dword& ID, dword& Width, dword& Height)
	{
		dword OutBuf[3];
		KeRequestCall(ClRenderer_GetFrameSurface, null, 0, PB(OutBuf), 12);
		ID = OutBuf[0];
		Width = OutBuf[1];
		Height = OutBuf[2];
	}

	void BlitSurface(const CBlitInfo& BI)
	{
		m_BlitSMArray.Push(BI);
		if (m_BlitSMArray.IsFull())
			BlitFlush();
	}

	void UpdateFrameSurface(const CUpdateInfo& UI)
	{
		m_UpdateSMArray.Push(UI);
		if (m_UpdateSMArray.IsFull())
			UpdateFlush();
	}

	void BlitFlush()
	{
		if (m_BlitSMArray.GetSize() == 0)
			return;

		dword InBuf[2];
		InBuf[0] = m_BlitSMArray.GetSMID();
		InBuf[1] = m_BlitSMArray.GetSize();
		KeRequestCall(ClRenderer_BlitSurfaces, PB(InBuf), 8, null, 0);
		m_BlitSMArray.Clear();
	}

	void UpdateFlush()
	{
		if (m_UpdateSMArray.GetSize() == 0)
			return;

		dword InBuf[2];
		InBuf[0] = m_UpdateSMArray.GetSMID();
		InBuf[1] = m_UpdateSMArray.GetSize();
		KeRequestCall(ClRenderer_UpdateFrameSurface, PB(InBuf), 8, null, 0);
		m_UpdateSMArray.Clear();
	}

private:
	CSmartPtr<CSurface> m_FrameSurface;
	CArray<CSmartPtr<CSurface> > m_Surfaces;
	CArray<CSurfUpdateInfo> m_SurfUpdateInfos;

	dword m_ActiveSurfaceID;

	dword m_Width;
	dword m_Height;

	dword m_CursorX;
	dword m_CursorY;

	bool* m_Dirty;
	bool* m_DirtyTemp;
	dword m_DirtyW;
	dword m_DirtyH;
	dword m_DirtyQ;

	CSMArray<CUpdateInfo> m_UpdateSMArray;
	CSMArray<CBlitInfo> m_BlitSMArray;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_SurfMgr))
		return;
	CSurfaceManager SM;
	KeSetSymbol(SmSurfMgr_Terminated);
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=