// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GenericVideo.h
#include "API.h"
#pragma once

// ----------------------------------------------------------------------------
class CGenericVideo
{
public:
	bool Init(dword VideoID, dword Priority)
	{
		dword NotificationID;
		dword NotificationSize;
		dword SrcPID;

		KeWaitForSymbol(SmVideo_Identify);

		dword InBuf[3];
		InBuf[0] = VideoID;
		InBuf[1] = Priority;
		KeNotify(NfVideo_IdentifyResp, PB(InBuf), 8);

		dword Detected = Detect() ? 1 : 0;
		InBuf[1] = Detected;

		KeWaitForSymbol(SmVideo_Info);
		KeNotify(NfVideo_InfoResp, PB(InBuf), 8);

		if (!Detected)
			return false;

		byte OutBuf[5];
		KeEnableNotification(NfVideo_FlowResp);
		for (;;)
		{
			KeWaitFor(1);
			KeGetNotification(OutBuf, 5, NotificationID, NotificationSize, SrcPID);
			if (*PD(OutBuf) == VideoID)
				break;
		}
		KeDisableNotification(NfVideo_FlowResp);

		byte Exiting = OutBuf[4];
		if (Exiting)
			return false;
		return true;
	}

	virtual bool Detect()
	{
		return true;
	}
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=