// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// VMwareMouse.cpp
#include "API.h"

// ----------------------------------------------------------------------------
extern "C" int _fltused = 0;

// ----------------------------------------------------------------------------
class VMBackdoor
{
public:
	dword eax;
	dword ebx;
	dword ecx;
	dword edx;
	dword esi;
	dword edi;

	void InOut()
	{
		dword veax = 0x564D5868;
		dword vebx = ebx;
		dword vecx = ecx;
		dword vedx = (edx & 0xFFFF0000) | 0x5658;
		dword vesi = esi;
		dword vedi = edi;
		__asm
		{
			mov eax, veax
			mov ebx, vebx
			mov ecx, vecx
			mov edx, vedx
			mov esi, vesi
			mov edi, vedi
			in eax, dx
			mov veax, eax
			mov vebx, ebx
			mov vecx, ecx
			mov vedx, edx
			mov vesi, esi
			mov vedi, edi
		}
		eax = veax;
		ebx = vebx;
		ecx = vecx;
		edx = vedx;
		esi = vesi;
		edi = vedi;
	}
};

// ----------------------------------------------------------------------------
class VMwareMouse
{
private:
	int packetIndex;
	VMBackdoor bkdr;

	bool buttonPressed[3];

public:
	VMwareMouse()
	{
		packetIndex = 0;
		buttonPressed[0] = false;
		buttonPressed[1] = false;
		buttonPressed[2] = false;

		// turn on absolute mode
		bkdr.ebx = 0x53424152;
		bkdr.ecx = 41;
		bkdr.InOut();

		KeEnableNotification(NfKe_TerminateProcess);
		KeEnableNotification(Nfi8043_MouseData);

		CNotification<1> n;
		for (;;)
		{
			KeWaitFor(1);
			n.Recv();
			if (n.GetID() == Nfi8043_MouseData)
			{
				byte param = n.GetByte(0);
				if (packetIndex == 0)
				{
					if (param & 0x8)
					{
						packetIndex++;
					}
				}
				else if (packetIndex == 1)
				{
					packetIndex++;
				}
				else if (packetIndex == 2)
				{
					packetIndex = 0;

					bkdr.ebx = 0;
					bkdr.ecx = 40;
					bkdr.InOut();

					int wordsToRead = bkdr.eax & 0xFFFF;
					while (wordsToRead >= 4)
					{
						wordsToRead -= 4;
						bkdr.ebx = 4;
						bkdr.ecx = 39;
						bkdr.InOut();

						int x = bkdr.ebx;
						int y = bkdr.ecx;
						int z = bkdr.edx;
						word flags = bkdr.eax >> 16;
						word buttons = bkdr.eax & 0xFFFF;

						if (flags & 1)
						{
							if ((x != 0) || (y != 0))
							{
								int moveData[2] = {x, -y};
								KeNotify(Nf_MouseDeltaMove, (byte*)moveData, 8);
							}
						}
						else
						{
							float moveData[2] = {x / 65535.0, y / 65535.0};
							KeNotify(Nf_MouseAbsMove, (byte*)moveData, 8);
						}

						bool buttonPressedNew[3] =
						{
							buttons & 0x20,
							buttons & 0x10,
							buttons & 0x08
						};

						for (int i = 0; i < 3; i++)
						{
							if (buttonPressed[i] != buttonPressedNew[i])
							{
								if (buttonPressedNew[i])
									KeNotify(Nf_MouseButtonDown, (byte*)(&i), 1);
								else
									KeNotify(Nf_MouseButtonUp, (byte*)(&i), 1);
								buttonPressed[i] = buttonPressedNew[i];
							}
						}
					}
				}
			}
			else if (n.GetID() == NfKe_TerminateProcess)
			{
				return;
			}
		}
	}
};

// ----------------------------------------------------------------------------
void OnDetectFail()
{
	KeSetSymbol(SmVMWareMouse_DetectFail);
	KeExitProcess();
}

// ----------------------------------------------------------------------------
void Entry()
{
	KeSetGeneralProtectionExceptionHandler(OnDetectFail);

	// check vm backdoor availability
	VMBackdoor b;
	b.ecx = 10;
	b.InOut();

	if (b.ebx != 0x564D5868)
		OnDetectFail();

	// request vm mouse version
	b.ebx = 0x45414552;
	b.ecx = 41;
	b.InOut();

	// check for response
	b.ebx = 0;
	b.ecx = 40;
	b.InOut();

	if ((b.eax & 0xFFFF) == 0)
		OnDetectFail();

	// get version
	b.ebx = 1;
	b.ecx = 39;
	b.InOut();

	if (b.eax != 0x3442554a)
		OnDetectFail();

	KeSetSymbol(SmVMWareMouse_DetectOK);

	VMwareMouse vmwareMouse;
}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=