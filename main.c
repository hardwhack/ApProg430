#include  "msp430g2553.h"
#define uint8_t unsigned char

#define MCLR BIT2
#define PGD BIT1
#define PGC BIT0
// MCLR	P2.2
// PGD	P2.1
// PGC	P2.0

#define RX_SIZE 4		// receive buff
static uint8_t Reply[2];
static uint8_t RxBuff[RX_SIZE];
static uint8_t Invitation[] = "Avr Board";


void SendBuffer(uint8_t[], uint8_t);
void ReceiveBuffer(uint8_t[], uint8_t);
void Initialize(void);
void WriteIcsp(uint8_t);
void ReadIcsp(uint8_t);
void SetMclr(uint8_t value); // 0x1X
void ClockControlBits(uint8_t value); // 0x2X
void ClockLowXTimes(uint8_t value); // 0x3X
void ClockByte(uint8_t value); // partially 0x5X
void PinByte(uint8_t *value); // partially 0x6X


void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  BCSCTL1 = CALBC1_8MHZ;                    // Set DCO
  DCOCTL = CALDCO_8MHZ;
  P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
  P1SEL2 = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  UCA0BR0 = 65;                            // 8MHz 57600
  UCA0BR1 = 3;                              // 8MHz 57600
  UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**


  Initialize();
  	while(1)
      {
  		ReceiveBuffer(RxBuff, 1);
  		uint8_t command = RxBuff[0] & 0xF0;
  		switch(command)
  		{
  			case 0x00:
  				SendBuffer(RxBuff, 1);
  			break;
  			case 0x10:
  				SetMclr(RxBuff[0]);
  				SendBuffer(Reply, 1);
  			break;
  			case 0x20:
  				ClockControlBits(RxBuff[0]);
  				SendBuffer(Reply, 1);
  			break;
  			case 0x30:
  				ClockLowXTimes(RxBuff[0]);
  				SendBuffer(Reply, 1);
  			break;
  			case 0x50:
  				WriteIcsp(RxBuff[0] & 0x0F);
  			break;
  			case 0x60:
  				ReadIcsp(RxBuff[0] & 0x0F);
  			break;
  		}
      }
}


void Initialize(void)
{
	Reply[0] = 0x01;
	Reply[1] = 0x00;

	P2DIR = BIT0+BIT1+BIT2;					// P2.0, 2.1, 2.2 as output
	P2OUT &= ~(BIT0+BIT1+BIT2);  // low

	SendBuffer(Invitation, 9);
}

void WriteIcsp(uint8_t length)
{
	uint8_t index;

	if(length > 4)
	{
		SendBuffer(Reply + 1, 1);
		return;
	}

	ReceiveBuffer(RxBuff, length);
	for(index = 0; index < length; index++)
	{
		ClockByte(RxBuff[index]);
	}
	SendBuffer(Reply, 1);
}

void ReadIcsp(uint8_t length)
{
	uint8_t index;

	if(length > 4)
	{
		SendBuffer(Reply + 1, 1);
		return;
	}

	for(index = 0; index < length; index++)
	{
		PinByte(RxBuff + index);
	}
	SendBuffer(RxBuff, length);
}

void SendBuffer(unsigned char buffer[], unsigned char count)
{
	uint8_t index;
	for(index = 0; index < count; index++)
	{
		while ((IFG2&UCA0TXIFG) == 0)
			; // Do nothing until UDR is ready for more data to be written to it
		UCA0TXBUF = buffer[index]; // Send out the byte value in the variable "ByteToSend"
	}
}

void ReceiveBuffer(unsigned char buffer[], unsigned char count)
{
	uint8_t index;
	for(index = 0; index < count; index++)
	{
		while ((IFG2&UCA0RXIFG) == 0)
			; // Do nothing until data have been received and is ready to be read from UDR
		buffer[index] = UCA0RXBUF;
	}
}

void SetMclr(uint8_t value)
{
	if((value & 0x0F))
		P2OUT |= MCLR; // MCLR High
	else
		P2OUT &= ~MCLR; // MCLR Low
}

void ClockControlBits(uint8_t value)
{
	P2DIR |= 0x02;				//PGD
	uint8_t mask = 0x01;
	while (mask != 0x10)
	{
		if(value & mask)
			P2OUT = (P2OUT & ~0x01) | 0x02;
		else
			P2OUT &= 0xFC;
		mask <<= 1;
		P2OUT |= 0x01; //pop clock
	}
	P2OUT &= ~0x03; // drop clock & PGD
}

void ClockLowXTimes(uint8_t value)
{
	P2DIR |= 0x02;
	uint8_t length = value & 0x0F;
	while(length)
	{
		P2OUT &= ~0x03; //PGD + PGC low
		length--;
		P2OUT |= 0x01;
	}
	P2OUT &= ~0x03;
}

void ClockByte(uint8_t value)
{
	P2DIR |= 0x02; // switch PGD to output

	uint8_t mask = 0x01;
	while(mask)
	{
		if(value & mask)
			P2OUT = (P2OUT & ~0x01) | 0x02;
		else
			P2OUT &= ~0x03;
		mask <<= 1;
		P2OUT |= 0x01; //pop clock
	}
	P2OUT &= ~0x03;
}

void PinByte(uint8_t *value)
{
	P2DIR &= ~0x02; // Switch PGD to input
	P2OUT |= 0x02; // activate pull-up...
	P2REN |= 0x02; // enable pullup resistor

	uint8_t mask = 0x01;
	while(mask)
	{

		P2OUT |= 0x01; //pop clock

		if(P2IN & 0x02)
			*value |= mask;
		else
			*value &= ~mask;
		P2OUT &= ~0x01; //drop clock
		mask <<= 1;
	}
	P2OUT &= ~0x02; //deactivate pull-up...
	P2REN &= ~0x02; // deactivate pullup resistor
}
