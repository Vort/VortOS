// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Random.h
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
extern "C" unsigned __int64 __rdtsc();
#pragma intrinsic(__rdtsc)

// ----------------------------------------------------------------------------
class Random
{
public:	
	Random()
	{
		qword tsc = __rdtsc();
		seed = tsc ^ (tsc >> 32);
	}

	Random(dword seed)
	{
		// Инициализация выборки заданным значением
		this->seed = seed;
	}

	// Получение word в пределе [0, 32767]
	word GetNextWord()
	{
		Pulse();
		return (seed / 65536) % 32768;
	}

	// Получение float в пределе [0.0f, 1.0f]
	float GetNextFloat1()
	{
		return GetNextWord() / 32767.0f;
	}

	// Получение float в пределе [-1.0f, 1.0f]
	float GetNextFloat2()
	{
		return GetNextFloat1() * 2.0f - 1.0f;
	}

private:
	void Pulse()
	{
		// Определение следующего значения выборки
		seed = seed * 1103515245 + 12345;
	}

	dword seed;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=