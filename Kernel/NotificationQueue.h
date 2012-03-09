// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// NotificationQueue.h
#include "Library/Defs.h"
#include "PagedQueue.h"
#pragma once

/*
// ----------------------------------------------------------------------------
class CMessageQueue
{
public:
	CMessageQueue(CPhysMemManager& PMM, dword PageCount)
		: m_Data(PMM, PageCount)
	{
		m_MessageCount = 0;
	}

	bool AddMessage(dword SrcPID, dword MsgID, dword MsgSize, byte* MsgData)
	{
		dword MRawSize = MsgSize + 12;
		if (MRawSize > m_Data.GetFreeSpace())
			return false;
		
		m_Data.AddData(CUniPtr(PB(&MsgSize)), 4);
		m_Data.AddData(CUniPtr(PB(&MsgID)), 4);
		m_Data.AddData(CUniPtr(PB(&SrcPID)), 4);
		m_Data.AddData(CUniPtr(MsgData), MsgSize);
		m_MessageCount++;

		return true;
	}

	bool GetLastMessageSize(dword& MsgSize)
	{
		return m_Data.PeekData(PB(&MsgSize), 4);
	}

	bool PopMessage(dword& SrcPID, dword& MsgID, byte* MsgData)
	{
		dword MsgSize = 0;
		m_Data.GetData(PB(&MsgSize), 4);
		m_Data.GetData(PB(&MsgID), 4);
		m_Data.GetData(PB(&SrcPID), 4);
		m_Data.GetData(MsgData, MsgSize);
		return true;
	}

private:
	dword m_MessageCount;
	CPagedQueue m_Data;
};
*/


// ----------------------------------------------------------------------------
class CNotificationQueue
{
public:
	CNotificationQueue(CPhysMemManager& PMM, dword PageCount)
		: m_Data(PMM, PageCount)
	{
		m_NotificationCount = 0;
	}

	dword GetUsedPageCount()
	{
		return m_Data.GetUsedPageCount();
	}


	bool AddNotification(dword SrcPID, dword NfID, dword NfSize, CUniPtr& NfData)
	{
		dword NFRawSize = NfSize + 12;
		if (NFRawSize > m_Data.GetFreeSpace())
			return false;

		dword NfInfo[3];
		NfInfo[0] = NfSize;
		NfInfo[1] = SrcPID;
		NfInfo[2] = NfID;

		m_Data.AddData(CUniPtr(PB(&NfInfo)), 12);

		ErrIf(!m_Data.AddData(NfData, NfSize));

		m_NotificationCount++;

		return true;
	}

	bool PopNotification(dword& SrcPID, dword& NfID, CUniPtr& NfData)
	{
		if (m_NotificationCount == 0)
			return false;

		dword NfInfo[3];
		m_Data.GetData(CUniPtr(PB(&NfInfo)), 12);

		dword NfSize = NfInfo[0];
		SrcPID = NfInfo[1];
		NfID = NfInfo[2];

		m_Data.GetData(NfData, NfSize);
		m_NotificationCount--;

		return true;
	}
	
	dword GetFrontNotificationSize()
	{
		if (m_NotificationCount == 0)
			return 0;

		dword NfSize;
		m_Data.PeekData(PB(&NfSize), 4);
		return NfSize;
	}

	dword GetNotificationCount()
	{
		return m_NotificationCount;
	}

private:
	dword m_NotificationCount;
	CPagedQueue m_Data;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=