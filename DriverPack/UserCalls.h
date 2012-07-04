// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// UserCalls.h
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
#define ClDMA_Reset 0x58813CA3

#define ClFloppy_ReadSector 0xE4B910A3
#define ClATA_ReadSector 0x24338048

#define ClCache_GetSector 0xB8B682DE
#define ClCache_GetDeviceList 0xBBC1C173

#define ClFAT_ReadFile 0x5008E39E
#define ClFAT_GetFileSize 0xD38E9528
#define ClFAT_FindFirstFile 0x9FA63A54
#define ClFAT_FindNextFile 0xD48EF46C

#define ClCDFS_ReadFile 0x50C96B0C
#define ClCDFS_GetFileSize 0x6F234728
#define ClCDFS_FindFirstFile 0xC84AA9C0
#define ClCDFS_FindNextFile 0xD3232391

#define ClFileSystem_GetDiskCount 0x7AE68B67
#define ClFileSystem_ReadFile 0xA670AC5E
#define ClFileSystem_GetFileSize 0x78EB9C74
#define ClFileSystem_FindFirstFile 0xC061C1FD
#define ClFileSystem_FindNextFile 0xD6121DCB

#define ClPCI_GetDeviceByID 0xA1E10FC1
#define ClPCI_GetDeviceReg 0xA42CB9A4
#define ClPCI_GetDeviceCount 0x693E5154
#define ClPCI_GetDeviceInfoByIndex 0x54323DBF

#define ClNetwork_GetSelfMACAddress 0xC6B234D1

#define ClFont_FitText 0xB168AC62
#define ClFont_GetTextWidth 0x41041051

#define ClVideo_GetFrameSurface 0x6D576866
#define ClVideo_GetCaps 0x71839E38
#define ClVideo_GetQuantSize 0x6828AB30
#define ClVideo_UpdateFrameSurface 0xB7B9C049

#define ClRenderer_GetResolution 0x0BA3486C
#define ClRenderer_GetFrameSurface 0x5BD1972C
#define ClRenderer_CreateSurface 0x2AAAEC90
#define ClRenderer_RemoveSurface 0xADF4DB28
#define ClRenderer_SetSurfaceData 0xF7EF8A7B
#define ClRenderer_BlitSurfaces 0xE0A9C107
#define ClRenderer_UpdateFrameSurface 0xDEEBF911
#define ClRenderer_CreateFontSurface 0x068502FE
#define ClRenderer_BlitFontSurface 0xA3493CEF
#define ClRenderer_Wait 0xCA07B54F

#define ClSurfMgr_SetFont 0xE6887E97
#define ClSurfMgr_CreateSurface 0x2B5FCE0B
#define ClSurfMgr_CreateTextSurface 0x463EE0F5
#define ClSurfMgr_SetSurfaceData 0xC0E01032
#define ClSurfMgr_WaitRedraw 0xA7E39A85
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=