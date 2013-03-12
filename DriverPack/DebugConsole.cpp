// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// DebugConsole.cpp
#include "API.h"
#include "String2.h"

// ----------------------------------------------------------------------------
class CDebugConsole
{
public:
	CDebugConsole()
	{
		KeWaitForSymbol(Sm_InitStage2);

		m_CursorX = 0;
		m_CursorY = 0;
		m_Width = 192;
		m_Height = 588;

		m_SurfaceID = CreateSurface(600, 6, m_Width, m_Height);
		FillSurface(m_SurfaceID, 0xFF000000);
		ShowSurface(m_SurfaceID);

		KeEnableNotification(Nf_DebugOut);
		KeEnableNotification(Nf_VirtualKey);
		KeEnableNotification(NfKe_ExceptionInfo);
		KeEnableNotification(NfKe_TerminateProcess);

		CNotification<0x100> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();
			if (N.GetID() == Nf_DebugOut)
			{
				FitOut((char*)N.GetBuf(), N.GetSize());
			}
			else if (N.GetID() == Nf_VirtualKey)
			{
				if (N.GetByte(1) == 1)
				{
					EVirtualKey VK = EVirtualKey(N.GetByte(0));
					if (VK == VK_C)
						Clear();
				}
			}
			else if (N.GetID() == NfKe_ExceptionInfo)
			{
				dword PID = N.GetDword(0);
				dword Exc = N.GetDword(1);
				dword EIP = N.GetDword(2);

				char B1[3] = {0};
				char B2[9] = {0};
				ByteToString(byte(Exc), B1);
				WordToString((EIP >> 16) & 0xFFFF, B2);
				WordToString((EIP >> 0) & 0xFFFF, B2 + 4);
				CStringA S = "[Exception 0x";
				S.Add(B1);
				S.Add(" in '");
				S.Add(CStringA(PC(N.GetBuf()) + 12, N.GetSize() - 12));
				S.Add("' at 0x");
				S.Add(B2);
				S.Add(']');

				FitOut(S._ptr(), S.Len(), 0xFFFA7060);
			}
			else if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}

	void Clear()
	{
		m_CursorX = 0;
		m_CursorY = 0;
		FillSurface(m_SurfaceID, 0xFF000000);
	}

	void FitOut(char* str, int strSize, dword color = 0xFFFFFFFF)
	{
		CStringA chunk1;
		CArray<CStringA> chunks1;
		for (int i = 0; i < strSize; i++)
		{
			if (str[i] != '\n')
			{
				chunk1.Add(str[i]);
			}
			else
			{
				if (chunk1.Len() != 0)
					chunks1.PushBack(chunk1);
				chunks1.PushBack("\n");
				chunk1 = "";
			}
		}
		if (chunk1.Len() != 0)
			chunks1.PushBack(chunk1);


		CArray<CStringA> chunks2;
		int startPosition = m_CursorX;
		for (int i = 0; i < chunks1.Size(); i++)
		{
			if (chunks1[i].GetCh(0) == '\n')
			{
				chunks2.PushBack(chunks1[i]);
				startPosition = 0;
				continue;
			}

			CStringA chunkPart = chunks1[i];
			for (;;)
			{
				int widthLeft = m_Width - startPosition;
				int fitCharCount = FitText(widthLeft,
					chunkPart._ptr(), chunkPart.Len());
				if (fitCharCount == chunkPart.Len())
				{
					chunks2.PushBack(chunkPart);
					break;
				}
				else
				{
					chunks2.PushBack(chunkPart.Left(fitCharCount));
					chunkPart = chunkPart.RightAbs(fitCharCount);

					chunks2.PushBack("\n");
					startPosition = 0;
				}
			}
		}


		for (int i = 0; i < chunks2.Size(); i++)
		{
			if (chunks2[i].GetCh(0) == '\n')
			{
				m_CursorX = 0;
				m_CursorY += 14;
				if (m_CursorY + 14 > m_Height)
					m_CursorY = 0;
				DrawRect(m_SurfaceID, 0, m_CursorY, m_Width, 14, 0xFF000000);
				continue;
			}

			OutText(m_SurfaceID, m_CursorX, m_CursorY,
				color, chunks2[i]._ptr(), chunks2[i].Len());

			dword printedTextWidth = 0;
			KeRequestCall(ClFont_GetTextWidth,
				(byte*)chunks2[i]._ptr(), chunks2[i].Len(),
				(byte*)&printedTextWidth, 4);
			m_CursorX += printedTextWidth;
		}
	}

private:
	dword m_CursorX;
	dword m_CursorY;

	dword m_SurfaceID;

	dword m_Width;
	dword m_Height;
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_DbgCons))
		return;

	CDebugConsole DC;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=