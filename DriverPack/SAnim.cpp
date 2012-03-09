// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// SAnim.cpp
#include "API.h"
extern "C" double sin(double x);
extern "C" double sqrt(double x);
#pragma intrinsic(sin, sqrt)

// ----------------------------------------------------------------------------
extern "C" int _fltused = 0;

// ----------------------------------------------------------------------------
void Entry()
{
	dword SurfaceID = CreateSurface(0, 0, 32, 32);
	dword SMID = KeAllocSharedMem(32 * 32 * 4);
	byte* Texture = KeMapSharedMem(SMID);

	dword Ofs = 0;
	for (dword y = 0; y < 32; y++)
		for (dword x = 0; x < 32; x++)
		{
			double dx = x - 16.0;
			double dy = y - 16.0;
			
			double R = sqrt(dx*dx+dy*dy);
			R *= 16;
			if (R > 255)
				R = 255;
			Texture[Ofs+0] = 0xFE;
			Texture[Ofs+1] = 0xC0;
			Texture[Ofs+2] = 0xB8;
			Texture[Ofs+3] = 255 - byte(R);
			Ofs += 4;
		}

	SetSurfaceData(SurfaceID, SMID, 32 * 32 * 4);
	KeReleaseSharedMem(SMID);

	bool IsShowed = false;

	KeEnableNotification(NfKe_TerminateProcess);

	double T = Pi/4;
	double X = 0;
	double Y = 0;

	dword W;
	dword H;
	GetResolution(W, H);
	double HalfW = W / 2;
	double HalfH = H / 2;

	for (;;)
	{
		T += 0.01;
		X = (HalfW - 64.0) * sin(3 * T + Pi / 2) + HalfW;
		Y = (HalfH - 64.0) * sin(4 * T) + HalfH;
		MoveSurface(SurfaceID, dword(X), dword(Y));
		if (!IsShowed)
		{
			ShowSurface(SurfaceID);
			IsShowed = true;
		}
		KeWaitTicks(1);

		if (KeGetNotificationCount())
			return;
	}
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=