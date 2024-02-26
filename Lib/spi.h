#ifndef _SPI_H_INCLUDED
#define _SPI_H_INCLUDED

#include <msp430.h>
#include <inttypes.h>

inline static void SPI_begin();
inline static uint8_t SPI_transfer(uint8_t data);
inline static void SPI_end();

void SPI_begin(void) {
    /* Configure P1.5, 1.6, 1.7 for SPI. */
    P1SEL |= (BIT5 | BIT6 | BIT7);
    P1SEL2 |= (BIT5 | BIT6 | BIT7);

    /* Setup SPI on USCI B */
    UCB0CTL1 = (UCSSEL_2 | UCSWRST);               // SMCLK + Reset
    UCB0CTL0 = (UCCKPL | UCMSB | UCMST | UCSYNC);  // 3-pin, 8-bit SPI master
    UCB0BR0 = 8;                                   // /8
    UCB0BR1 = 0;                                   //

    UCB0CTL1 &= ~UCSWRST;                          // release USCI for operation
}

uint8_t SPI_transfer(uint8_t data) {
    UCB0TXBUF = data; // Setting TXBUF clears the TXIFG

    while(UCB0STAT & UCBUSY); // wait for SPI TX/RX to finish

    return UCB0RXBUF; // Reading clears RXIFG
}

void SPI_end()
{
    UCB0CTL1 |= UCSWRST;                // Put USCI in reset mode
}

#endif
