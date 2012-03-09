// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Storage.h
#include "Defs.h"
#pragma once

// ----------------------------------------------------------------------------
class CStorageDeviceInfo
{
public:
	dword m_DriverID;
	dword m_PhysicalDeviceID;
	dword m_LogicalDeviceID;
	dword m_SectorSize;
	dword m_StartSector;
	dword m_SectorCount;
	dword m_CachedSectorsCount;
	dword m_ReadSectorFunc;
};

// ----------------------------------------------------------------------------
class CFileSystemDeviceInfo
{
public:
	dword m_DeviceID;
	dword m_ReadFileFunc;
	dword m_GetFileSizeFunc;
	dword m_FindFirstFileFunc;
	dword m_FindNextFileFunc;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=