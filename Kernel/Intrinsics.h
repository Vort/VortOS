// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Intrinsics.h
#pragma once

// ----------------------------------------------------------------------------
extern "C" int _outp(unsigned short port, int databyte);
extern "C" int _inp(unsigned short port);
extern "C" unsigned short _outpw(unsigned short port, unsigned short dataword);
extern "C" unsigned short _inpw(unsigned short port);
extern "C" unsigned long _outpd(unsigned short port, unsigned long dataword);
extern "C" unsigned long _inpd(unsigned short port);
extern "C" unsigned __int64 __rdtsc();
extern "C" void __lidt(void *Source);
extern "C" unsigned long __readcr0();
extern "C" void __writecr0(unsigned Data);
extern "C" void __writecr3(unsigned Data);
extern "C" void __invlpg(void* Address);

// ----------------------------------------------------------------------------
#pragma intrinsic(_outp, _inp, _outpw, _inpw, _outpd, _inpd)
#pragma intrinsic(__rdtsc, __lidt, __readcr0, __writecr0, __writecr3, __invlpg)

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=