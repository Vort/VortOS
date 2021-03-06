// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Notifications.h
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
#define NfKe_Irq 0x7598B2C6
#define NfKe_TimerTick 0xC47BA43B
#define NfKe_ProcessExited 0x389DF145
#define NfKe_TerminateProcess 0xDD41ACFB
#define NfKe_ExceptionInfo 0x35B11433

#define Nf_DebugOut 0x0612B45E

#define NfKeyboard_SwitchLEDStatus 0x0009A7AB
#define Nfi8043_KeyboardData 0x0104119E
#define Nfi8043_MouseData 0xB33B6FA1

#define NfCom_Data 0x46C7EBC5

#define NfPCI_WriteConfByte 0xF3A20E32
#define NfPCI_WriteConfWord 0x82C56C07
#define NfPCI_WriteConfDword 0x1214C0E0

#define NfNetwork_SendPacket 0x8CB8FB1A
#define NfNetwork_RecvdPacket 0xBAC9148A

#define NfFileSystem_AddDisk 0x0BF271E9

#define NfRenderer_FillSurface 0x4AC4B280
#define NfRenderer_DrawRect 0xE0F07A75

#define NfSurfMgr_MoveSurface 0xD339A85A
#define NfSurfMgr_FillSurface 0x0152595C
#define NfSurfMgr_DrawRect 0x38C02474
#define NfSurfMgr_ShowSurface 0xE7D8741E
#define NfSurfMgr_TextBlit 0xCB72B032

#define Nf_VirtualKey 0x42C4923A
#define Nf_StorageDeviceDetected 0x64704FF4
#define Nf_StorageDeviceCached 0xCC000302
#define Nf_MouseDeltaMove 0x2A9FA8AC
#define Nf_MouseAbsMove 0x1082C471
#define Nf_MouseButtonDown 0x30B48F80
#define Nf_MouseButtonUp 0x2B148460
#define Nf_SurfaceActivated 0x7976D15A

#define Nf_MouseClick 0x512451C8
#define Nf_MouseDoubleClick 0xBDA476AB

#define Nf_CursorMoveTo 0xBDCC5332
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=