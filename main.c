#include <msp430.h> 
//
// Define symbolic LCM - MCU pin mappings
// We've set DATA PIN TO 4,5,6,7 for easy translation
//
#define     LCM_PIN_RS            BIT0          // P2.0
#define     LCM_PIN_EN            BIT1          // P2.1
#define     LCM_PIN_D4            BIT2          // P2.2
#define     LCM_PIN_D5            BIT3          // P2.3
#define     LCM_PIN_D6            BIT4          // P2.4
#define     LCM_PIN_D7            BIT5          // P2.5
#define IRLED1 BIT6
#define RED BIT4
#define GREEN BIT5

#define     FALSE                 0
#define     TRUE                  1
#define UART_TXD 0x02                                  // TXD on P1.1 (Timer0_A.OUT0)
#define UART_RXD 0x04                                  // RXD on P1.2 (Timer0_A.CCI1A)

#define UART_TBIT_DIV_2 (1000000 / (9600 * 2))         // Conditions for 9600 Baud SW UART, SMCLK = 1MHz
#define UART_TBIT (1000000 / 9600)

int print =0;
int count =0;
int first =0;
int second =0;
int third=0;
int iiii =0;
int resetcount =1005;
unsigned int txData;                                   // UART internal variable for TX
unsigned int rxBuffer[40];                                // Received UART character
unsigned int BlueData =0;
unsigned long irdata=0;
int bit =0;
int i =0;
int ii =0;
int iii=0;
int bitt =0;

void TimerA_UART_tx(unsigned char byte);               // Function prototypes
void TimerA_UART_print(char *string);
void tx_send(long irdata);

void PulseLcm()
{
    //
    // pull EN bit low
    //
    P2OUT &= ~LCM_PIN_EN;
    __delay_cycles(200);

    //
    // pull EN bit high
    //
    P2OUT |= LCM_PIN_EN;
    __delay_cycles(200);

    //
    // pull EN bit low again
    //
    P2OUT &= ~LCM_PIN_EN;
    __delay_cycles(200);
}



//
// Routine Desc:
//
// Send a byte on the data bus in the 4 bit mode
// This requires sending the data in two chunks.
// The high nibble first and then the low nible
//
// Parameters:
//
//    ByteToSend - the single byte to send
//
//    IsData - set to TRUE if the byte is character data
//                  FALSE if its a command
//
// Return
//
//     void.
//
void SendByte(char ByteToSend, int IsData)
{
    //
    // clear out all pins
    //
    P2OUT &= ~(LCM_PIN_RS + LCM_PIN_EN);
    P2OUT &= ~(LCM_PIN_D7 + LCM_PIN_D6 + LCM_PIN_D5 + LCM_PIN_D4);
    //
    // set High Nibble (HN) -
    // usefulness of the identity mapping
    // apparent here. We can set the
    // DB7 - DB4 just by setting P2.7 - P2.4
    // using a simple assignment
    //
    P2OUT |= ((ByteToSend & 0xF0) >> 2);

    if (IsData == TRUE)
    {
        P2OUT |= LCM_PIN_RS;
    }
    else
    {
        P2OUT &= ~LCM_PIN_RS;
    }

    //
    // we've set up the input voltages to the LCM.
    // Now tell it to read them.
    //
    PulseLcm();
     //
    // set Low Nibble (LN) -
    // usefulness of the identity mapping
    // apparent here. We can set the
    // DB7 - DB4 just by setting P2.7 - P2.4
    // using a simple assignment
    //
    P2OUT &= ~(LCM_PIN_RS + LCM_PIN_EN);
    P2OUT &= ~(LCM_PIN_D7 + LCM_PIN_D6 + LCM_PIN_D5 + LCM_PIN_D4);

    P2OUT |= ((ByteToSend & 0x0F) << 2);

    if (IsData == TRUE)
    {
        P2OUT |= LCM_PIN_RS;
    }
    else
    {
        P2OUT &= ~LCM_PIN_RS;
    }

    //
    // we've set up the input voltages to the LCM.
    // Now tell it to read them.
    //
    PulseLcm();
}


//
// Routine Desc:
//
// Set the position of the cursor on the screen
//
// Parameters:
//
//     Row - zero based row number
//
//     Col - zero based col number
//
// Return
//
//     void.
//
void LcmSetCursorPosition(char Row, char Col)
{
    char address;

    //
    // construct address from (Row, Col) pair
    //
    if (Row == 0)
    {
        address = 0;
    }
    else
    {
        address = 0x40;
    }

    address |= Col;

    SendByte(0x80 | address, FALSE);
}


//
// Routine Desc:
//
// Clear the screen data and return the
// cursor to home position
//
// Parameters:
//
//    void.
//
// Return
//
//     void.
//
void ClearLcmScreen()
{
    //
    // Clear display, return home
    //
    SendByte(0x01, FALSE);
    SendByte(0x02, FALSE);
}


//
// Routine Desc:
//
// Initialize the LCM after power-up.
//
// Note: This routine must not be called twice on the
//           LCM. This is not so uncommon when the power
//           for the MCU and LCM are separate.
//
// Parameters:
//
//    void.
//
// Return
//
//     void.
//
void InitializeLcm(void)
{
    //
    // set the MSP pin configurations
    // and bring them to low
    //
    P2DIR |= (LCM_PIN_RS + LCM_PIN_EN);
    P2DIR |= (LCM_PIN_D7 + LCM_PIN_D6 + LCM_PIN_D5 + LCM_PIN_D4);
    P2OUT &= ~(LCM_PIN_RS + LCM_PIN_EN);
    P2OUT &= ~(LCM_PIN_D7 + LCM_PIN_D6 + LCM_PIN_D5 + LCM_PIN_D4);


    //
    // wait for the LCM to warm up and reach
    // active regions. Remember MSPs can power
    // up much faster than the LCM.
    //
    __delay_cycles(100000);


    //
    // initialize the LCM module
    //
    // 1. Set 4-bit input
    //
    P2OUT &= ~LCM_PIN_RS;
    P2OUT &= ~LCM_PIN_EN;

    P2OUT |= LCM_PIN_D5;
    PulseLcm();

    //
    // set 4-bit input - second time.
    // (as reqd by the spec.)
    //
    SendByte(0x28, FALSE);

    //
    // 2. Display on, cursor on, blink cursor
    //
    SendByte(0x0C, FALSE);

    //
    // 3. Cursor move auto-increment
    //
    SendByte(0x06, FALSE);
}


//
// Routine Desc
//
// Print a string of characters to the screen
//
// Parameters:
//
//    Text - null terminated string of chars
//
// Returns
//
//     void.
//
void PrintStr(char *Text)
{
    char *c;

    c = Text;

    while ((c != 0) && (*c != 0))
    {
        SendByte(*c, TRUE);
        c++;
    }
}
void PrintNUM(int number)
{

first = ((number %1000)%100)%10;
second = ((number %1000)%100)/10;
third = number %1000/100;

SendByte(third + 48, TRUE);
SendByte(second + 48, TRUE);
SendByte(first + 48, TRUE);

}

//
// Routine Desc
//
// main entry point to the sketch
//
// Parameters
//
//     void.
//
// Returns
//
//     void.
//
void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer

    P1SEL |= UART_TXD + UART_RXD;
    P1DIR |= (UART_TXD + RED + GREEN + IRLED1);

    TA0CCTL0 = OUT;                                      // Set TXD Idle as Mark = '1'
     TA0CCTL1 = SCS + CM1 + CAP + CCIE;                   // Sync, Neg Edge, Capture, Int
     TA0CTL = TASSEL_2 + MC_2;                            // SMCLK, start in continuous mode

    InitializeLcm();

    ClearLcmScreen();
    LcmSetCursorPosition(0,0);
    PrintStr("Starting");
    __delay_cycles(1000000);
    ClearLcmScreen();
    LcmSetCursorPosition(0,0);
    PrintStr("Swipe Card!");
    __enable_interrupt(); // enable all interrupts

    while (1)
    {
    	if(print ==1)
    	{
    		ClearLcmScreen();
    		LcmSetCursorPosition(0,0);
    		PrintStr("Card Number is:");
    		LcmSetCursorPosition(1,0);
    		PrintNUM(rxBuffer[10]);
    		SendByte('-', TRUE);
    		PrintNUM(rxBuffer[11]);
    		SendByte('-', TRUE);
    		PrintNUM(rxBuffer[12]);
    		SendByte('-', TRUE);
    		PrintNUM(rxBuffer[13]);
    		__delay_cycles(10000);
    		if((rxBuffer[10] == 54) && (rxBuffer[11] == 246) && (rxBuffer[12] == 44) && (rxBuffer[13] == 246 ))
    		{
    			P1OUT |= GREEN;
    			P1OUT &= ~RED;
    			tx_send(0xFF46731);
    		}
    		if((rxBuffer[10] == 31) && (rxBuffer[11] == 31) && (rxBuffer[12] == 27) && (rxBuffer[13] == 21 ))
    		{
    			P1OUT |= GREEN;
    			P1OUT &= ~RED;
    			tx_send(0xFF17942);
    		}
    		__delay_cycles(2700000);
    		ClearLcmScreen();
    		LcmSetCursorPosition(0,0);
    		PrintStr("Swipe Card!");
    		iiii=0;
    		print =0;
    		P1OUT &= ~GREEN;
    		P1OUT |= RED;

    	}
    	resetcount++;
    	if(resetcount == 1000)
    	{
    		print =1;
    	}
    	if(resetcount >1010)
    	{
    		resetcount =1005;
    	}
        __delay_cycles(400);

    }

}

void TimerA_UART_tx(unsigned char byte) {              // Outputs one byte using the Timer_A UART


  while (TACCTL0 & CCIE);                              // Ensure last char got TX'd

  TA0CCR0 = TAR;                                       // Current state of TA counter

  TA0CCR0 += UART_TBIT;                                // One bit time till first bit

  TA0CCTL0 = OUTMOD0 + CCIE;                           // Set TXD on EQU0, Int

  txData = byte;                                       // Load global variable

  txData |= 0x100;                                     // Add mark stop bit to TXData

  txData <<= 1;                                        // Add space start bit


}


void TimerA_UART_print(char *string) {                 // Prints a string using the Timer_A UART

  while (*string)
    TimerA_UART_tx(*string++);
}


#pragma vector = TIMER0_A0_VECTOR                      // Timer_A UART - Transmit Interrupt Handler

   __interrupt void Timer_A0_ISR(void) {

  static unsigned char txBitCnt = 10;
  TA0CCR0 += UART_TBIT;                                // Add Offset to CCRx

  if (txBitCnt == 0) {                                 // All bits TXed?

    TA0CCTL0 &= ~CCIE;                                 // All bits TXed, disable interrupt

    txBitCnt = 10;                                     // Re-load bit counter
  }
  else {
    if (txData & 0x01)
      TA0CCTL0 &= ~OUTMOD2;                            // TX Mark '1'
    else
      TA0CCTL0 |= OUTMOD2;                             // TX Space '0'
  }
  txData >>= 1;                                        // Shift right 1 bit
  txBitCnt--;
}


#pragma vector = TIMER0_A1_VECTOR                      // Timer_A UART - Receive Interrupt Handler

  __interrupt void Timer_A1_ISR(void) {

  static unsigned char rxBitCnt = 8;

  static unsigned char rxData = 0;

  switch (__even_in_range(TA0IV, TA0IV_TAIFG)) {       // Use calculated branching

    case TA0IV_TACCR1:                                 // TACCR1 CCIFG - UART RX

         TA0CCR1 += UART_TBIT;                         // Add Offset to CCRx

         if (TA0CCTL1 & CAP) {                         // Capture mode = start bit edge

           TA0CCTL1 &= ~CAP;                           // Switch capture to compare mode

           TA0CCR1 += UART_TBIT_DIV_2;                 // Point CCRx to middle of D0
         }
         else {
           rxData >>= 1;

           if (TA0CCTL1 & SCCI)                        // Get bit waiting in receive latch
             rxData |= 0x80;
           rxBitCnt--;

           if (rxBitCnt == 0) {                        // All bits RXed?
        	   if(iiii<19)
        	   {
             rxBuffer[iiii] = rxData;                        // Store in global variable
        	   }
             rxBitCnt = 8;                             // Re-load bit counter

             TA0CCTL1 |= CAP;                          // Switch compare to capture mode
             iiii++;
           }
         }
         break;
   }
resetcount =0;
}

  void tx_send(long irrealdata)
  {


    __disable_interrupt(); // enable all interrupts
  //  irdata = irrealdata;

  for(iii=32;iii>0;iii--)
  {
  	bitt = irrealdata & 1;
  	irdata |= bitt;
  	irrealdata>>=1;
  	irdata<<=1;
  }

    for (i = 322;i>0;i--)
    {
      P1OUT |= IRLED1;
      __delay_cycles(4);
      P1OUT &= ~IRLED1;
      __delay_cycles(4);

    }

      __delay_cycles(3830);


    for(ii = 32; ii>0;ii--)
    {
      bit = irdata & 1;
      if(bit == 0)
      {
          __delay_cycles(500);
        for (i = 21;i>0;i--)
        {
          P1OUT |= IRLED1;
          __delay_cycles(4);
          P1OUT &= ~IRLED1;
          __delay_cycles(4);

        }



      }
      else
      {

          __delay_cycles(1590);

        for (i = 21;i>0;i--)
        {
          P1OUT |= IRLED1;
          __delay_cycles(4);
          P1OUT &= ~IRLED1;
          __delay_cycles(4);


        }



      }
      irdata >>= 1;
    }

    for (i = 5;i>0;i--)
    {
      P1OUT |= IRLED1;
      __delay_cycles(4);
      P1OUT &= ~IRLED1;
      __delay_cycles(4);


    }


      __delay_cycles(1590);

      for (i = 20;i>0;i--)
      {
        P1OUT |= IRLED1;
        __delay_cycles(4);
        P1OUT &= ~IRLED1;
        __delay_cycles(4);

      }

    __enable_interrupt(); // enable all interrupts
  }



