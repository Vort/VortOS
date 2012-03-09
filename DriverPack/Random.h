// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Random.h
#pragma once
#include "Defs.h"

// ----------------------------------------------------------------------------
class CRandom
{
public:	
	CRandom()
	{
		// Инициализация начального значения выборки числом тактов процесоора
		dword Seed;
		__asm rdtsc __asm mov Seed, eax
		m_Seed = Seed;
	}

	CRandom(dword Seed)
	{
		// Инициализация выборки заданным значением
		m_Seed = Seed;
	}

	word GetNextWord()
	{
		// Получение word в пределе [0, 32767]
		// Желательно dword в пределе [0, 4294967295] :)
		Pulse();
		return (m_Seed / 65536) % 32768;
	}

	float GetNextFloat1()
	{
		// Получение float в пределе [0.0f, 1.0f]
		return GetNextWord() / 32767.0f;
	}

	float GetNextFloat2()
	{
		// Получение float в пределе [-1.0f, 1.0f]
		return GetNextFloat1() * 2.0f - 1.0f;
	}

private:
	void Pulse()
	{
		// Определение следующего значения выборки
		m_Seed = m_Seed * 1103515245 + 12345;
	}

	// Пока dword, но может (!) быть и qword и byte[256]
	dword m_Seed;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=