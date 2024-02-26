#ifndef _UART_H_INCLUDED
#define _UART_H_INCLUDED

#include "stdarg.h"

#define RXD BIT1 // RX P1.1
#define TXD BIT2 // TX P1.2

void UART_init(void)
{
    P1SEL  |= (RXD | TXD);                  // Configure RXD and TXD pins
    P1SEL2 |= (RXD | TXD);                  // Configure RXD and TXD pins
    UCA0CTL1 = UCSWRST;                     // Reset
    UCA0CTL0 = 0;                           // Nothing to set
    UCA0CTL1 |= UCSSEL_2;                   // SMCLK
    UCA0BR0 = 6;                            // Assume DCO 1MHz --> 9600
    UCA0BR1 = 0;                            // Assume DCO 1MHz --> 9600
    UCA0MCTL = UCBRS_0 | UCBRF_8 | UCOS16;  // Modulation
    UCA0CTL1 &= ~UCSWRST;                   // Initialize USCI state machine
}

void UART_putc(unsigned char c)
{
    while (!(IFG2 & UCA0TXIFG));            // USCI_A0 TX buffer ready?
    UCA0TXBUF = c;                          // TX
}

void UART_puts(const char *str)
{
     while(*str) UART_putc(*str++);
}

static const unsigned long _dv[] = {
//  4294967296      // 32 bit unsigned max
   1000000000,     // +0
    100000000,     // +1
     10000000,     // +2
      1000000,     // +3
       100000,     // +4
//       65535      // 16 bit unsigned max
        10000,     // +5
         1000,     // +6
          100,     // +7
           10,     // +8
            1,     // +9
};

static void _xtoa(unsigned long x, const unsigned long *dp)
{
   char c;
   unsigned long d;
   if(x) {
       while(x < *dp) ++dp;
       do {
           d = *dp++;
           c = '0';
           while(x >= d) ++c, x -= d;
           UART_putc(c);
       } while(!(d & 1));
   } else
       UART_putc('0');
}

static void _puth(unsigned n)
{
   static const char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
   UART_putc(hex[n & 15]);
}

void UART_printf(char *format, ...)
{
   char c;
   int i;
   long n;

   va_list a;
   va_start(a, format);
   while(c = *format++) {
       if(c == '%') {
           switch(c = *format++) {
               case 's':                       // String
                   UART_puts(va_arg(a, char*));
                   break;
               case 'c':                       // Char
                   UART_putc(va_arg(a, char));
                   break;
               case 'i':                       // 16 bit Integer
               case 'u':                       // 16 bit Unsigned
                   i = va_arg(a, int);
                   if(c == 'i' && i < 0) i = -i, UART_putc('-');
                   _xtoa((unsigned)i, _dv + 5);
                   break;
               case 'l':                       // 32 bit Long
               case 'n':                       // 32 bit uNsigned loNg
                   n = va_arg(a, long);
                   if(c == 'l' &&  n < 0) n = -n, UART_putc('-');
                   _xtoa((unsigned long)n, _dv);
                   break;
               case 'x':                       // 16 bit heXadecimal
                   i = va_arg(a, int);
                   _puth(i >> 12);
                   _puth(i >> 8);
                   _puth(i >> 4);
                   _puth(i);
                   break;
               case 'y':                       // 8 bit heXadecimal
                   i = va_arg(a, int);
                   _puth(i >> 4);
                   _puth(i);
                   break;
               case 0: return;
               default: goto bad_fmt;
           }
       } else
bad_fmt:    UART_putc(c);
   }
   va_end(a);
}

#endif
