// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defs.h
#pragma once

// ----------------------------------------------------------------------------
typedef unsigned char    byte;
typedef unsigned short   word;
typedef unsigned int     dword;
typedef unsigned __int64 qword;

// ----------------------------------------------------------------------------
#define Pi 3.14159265358979323846
#define hMyInst ((HINSTANCE)0x400000)

// ----------------------------------------------------------------------------
#define PV(Value) (void*)(Value)
#define PC(Value) (char*)(Value)
#define PB(Value) (byte*)(Value)
#define PW(Value) (word*)(Value)
#define PD(Value) (dword*)(Value)
#define null 0L
#define Case break; case
#define Default break; default

// ----------------------------------------------------------------------------
#define ErrIf(Value) {if (Value) __asm hlt}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=