// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Symbols.h
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
#define SmFileSystem_Ready 0x381D71D3
#define SmFileSystem_DiskReady 0xA4192180

#define SmCache_Ready 0x600F9932
#define SmPartition_Ready 0x9D3CC8D8
#define SmDMA_Ready 0x4151D847
#define SmSurfMgr_Ready 0x461558F4
#define SmRenderer_Ready 0x201CF56F
#define SmCursor_Ready 0xB69A5C4C
#define SmDesktop_Ready 0x2C9C9C65
#define SmFont_Ready 0x1E06B3B1
#define SmPCI_Ready 0x7CB5C2B7
#define SmNetwork_Waiting 0x40EBF855
#define SmNetwork_Ready 0x32817017

#define SmCursor_Terminated 0xF3C55B2E
#define SmDesktop_Terminated 0x5AAB30D0
#define SmSurfMgr_Terminated 0xCB480962

#define SmVideo_OK 0x25030598
#define SmVideo_Fail 0x52A323ED

#define SmVMWareMouse_DetectOK 0x2C5A012C
#define SmVMWareMouse_DetectFail 0x3D8738E9

#define Sm_InitStage0 0x3208F6AA // All Drivers Loaded
//#define Sm_InitStage1 0x7407D39F // Video Subsystem Ready
#define Sm_InitStage2 0x32FDDA6E // User Soft Allowed To Run

#define Sm_Lock_Console   0xBB10D102
#define Sm_Lock_IsaDMA    0x2997ACA3
#define Sm_Lock_ATA       0xFD0DB86C
#define Sm_Lock_FAT       0x8DD0DC8A
#define Sm_Lock_CDFS      0x11F2E475
#define Sm_Lock_Serial    0xAEC0AAB4
#define Sm_Lock_PCI       0x10B1EDC5
#define Sm_Lock_FileSys   0x33F8579D
#define Sm_Lock_Cache     0x7A8A2C05
#define Sm_Lock_Partition 0x86B16F37
#define Sm_Lock_Network   0x5BDCC256
#define Sm_Lock_Floppy    0x073CD70D
#define Sm_Lock_i8042     0x5FC779E1
#define Sm_Lock_NE2000    0x16B7AB3C
#define Sm_Lock_RTL8139   0xE0D476CB
#define Sm_Lock_AM79C970  0x54839F6C
#define Sm_Lock_SysInit   0x23A54B79
#define Sm_Lock_DbgCons   0x39047190
#define Sm_Lock_Ps2Keyb   0x7761238D
#define Sm_Lock_Ps2Mouse  0x92A3F1FF
#define Sm_Lock_SerMouse  0xC62B9B85
#define Sm_Lock_Viewer    0x01209E3E
#define Sm_Lock_Cursor    0xA0EB91EF
#define Sm_Lock_Video     0x9E36B100
#define Sm_Lock_Font      0x120390AC
#define Sm_Lock_SurfMgr   0xDD4684E0
#define Sm_Lock_ProcInfo  0x0E1E4EFB
#define Sm_Lock_Desktop   0x918185DB

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=