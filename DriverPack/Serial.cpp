// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Serial.cpp
#include "API.h"

// ----------------------------------------------------------------------------
class CSerial
{
public:
	CSerial()
	{
		dword PortBase = 0x3F8;
		KeEnableNotification(NfKe_IRQ4);
		KeEnableNotification(NfKe_TerminateProcess);
		KeUnmaskIRQ(4);

		KeOutPortByte(PortBase + 1, 0x00); // Int Off
		KeOutPortByte(PortBase + 3, 0x80); // DLAB On
		KeOutPortByte(PortBase + 0, 0x60); // Divisor Latch Low  [1200]
		KeOutPortByte(PortBase + 1, 0x00); // Divisor Latch High [1200]
		KeOutPortByte(PortBase + 3, 0x02); // 7 Bits, No Parity, 1 Stop Bit, DLAB Off

		KeOutPortByte(PortBase + 1, 0x01); // Int On
		KeOutPortByte(PortBase + 4, 0x00); // DTR Off, RTS Off

		KeWaitTicks(5);
		KeOutPortByte(PortBase + 4 , 0x0B); // DTR On, RTS On, OUT2 On

		CNotification<4> N;
		for (;;)
		{
			KeWaitFor(1);
			N.Recv();
			
			if (N.GetID() == NfKe_IRQ4)
			{
				byte B = KeInPortByte(PortBase);
				KeNotify(NfCom_Data, &B, 1);
				KeEndOfInterrupt(4);
			}
			else if (N.GetID() == NfKe_TerminateProcess)
				return;
		}
	}
};

// ----------------------------------------------------------------------------
void Entry()
{
	if (!KeSetSymbol(Sm_Lock_Serial))
		return;

	CSerial S;
}

// ----------------------------------------------------------------------------
//outportb(PORT1 + 1 , 0);   /* Turn off interrupts - Port1 */
// 
// /*         PORT 1 - Communication Settings         */
// 
// outportb(PORT1 + 3 , 0x80);  /* SET DLAB ON */
// outportb(PORT1 + 0 , 0x03);  /* Set Baud rate - Divisor Latch Low Byte */
//			      /* Default 0x03 =  38,400 BPS */
//			      /*         0x01 = 115,200 BPS */
//			      /*         0x02 =  57,600 BPS */
//			      /*         0x06 =  19,200 BPS */
//			      /*         0x0C =   9,600 BPS */
//			      /*         0x18 =   4,800 BPS */
//			      /*         0x30 =   2,400 BPS */
// outportb(PORT1 + 1 , 0x00);  /* Set Baud rate - Divisor Latch High Byte */
// outportb(PORT1 + 3 , 0x03);  /* 8 Bits, No Parity, 1 Stop Bit */
// outportb(PORT1 + 2 , 0xC7);  /* FIFO Control Register */
// outportb(PORT1 + 4 , 0x0B);  /* Turn on DTR, RTS, and OUT2 */


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=