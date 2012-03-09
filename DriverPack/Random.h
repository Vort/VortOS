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
		// ������������� ���������� �������� ������� ������ ������ ����������
		dword Seed;
		__asm rdtsc __asm mov Seed, eax
		m_Seed = Seed;
	}

	CRandom(dword Seed)
	{
		// ������������� ������� �������� ���������
		m_Seed = Seed;
	}

	word GetNextWord()
	{
		// ��������� word � ������� [0, 32767]
		// ���������� dword � ������� [0, 4294967295] :)
		Pulse();
		return (m_Seed / 65536) % 32768;
	}

	float GetNextFloat1()
	{
		// ��������� float � ������� [0.0f, 1.0f]
		return GetNextWord() / 32767.0f;
	}

	float GetNextFloat2()
	{
		// ��������� float � ������� [-1.0f, 1.0f]
		return GetNextFloat1() * 2.0f - 1.0f;
	}

private:
	void Pulse()
	{
		// ����������� ���������� �������� �������
		m_Seed = m_Seed * 1103515245 + 12345;
	}

	// ���� dword, �� ����� (!) ���� � qword � byte[256]
	dword m_Seed;
};
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=