/*
 * Defused slightly by Foxydiot
 * This code is Unlicensed. See http://unlicense.org for more info.
 * Last change made on: 2023-06-06 11:31
 *
 * Connections:
 *
 * P1.1 UART RX (UART Jumper on MSPEXPG2 placed horizontally)
 * P1.2 UART TX (UART Jumper on MSPEXPG2 placed horizontally)
 *
 * MSP430      RC522
 * P1.3  <-->  RST
 * P1.4  <-->  SDA    (SPI CS)
 * P1.5  <-->  SCK    (SPI CLK)
 * P1.6  <-->  MISO   (SPI MISO)
 * P1.7  <-->  MOSI   (SPI MOSI)
 * VCC   <-->  VCC
 * GND   <-->  GND
 */

#include <msp430.h>
#include "./Lib/spi.h"
#include "./Lib/Mfrc522.h"
#include "./Lib/UART.h"
#include "./Lib/delay.h"

#define NFC_chipSelect  BIT4 // P1.4
#define NFC_reset       BIT3 // P1.3

#define rL              BIT0
#define rR              BIT1
#define cR              BIT2
#define cG              BIT3
#define cB              BIT4

#define PxOUT           P2OUT

char NFC_colour;
char NFC_direction;
char dir;
char color;

int numberR = 0;
int numberG = 0;
int numberB = 0;

// Sets up the RFID reader
int rfSetup(void) {
    SPI_begin();
    RC522_init(NFC_chipSelect, NFC_reset);
    UART_init();
    RC522_setAntennaGain(7); // Maximal gain
    return(0);
}

/* This function returns the following information:
 * char NFC_direction:   L, R, D, S, O
 * char NFC_colour:      R, B, B
 */
int readTag(void) {
        uint8_t tagType[MAX_LEN];
        uint8_t serNum[MAX_LEN];
        if(RC522_request(PICC_REQALL, tagType) == MI_OK){
            if(RC522_anticoll(serNum) == MI_OK){
                UART_printf("Card has been detected\n");

                uint8_t l = RC522_selectTag(serNum);
                uint8_t cLoc[18];
                uint8_t keyA[6] = {key here};

                if(RC522_auth(PICC_AUTHENT1A, 1, keyA, serNum) == MI_OK){
                    if(RC522_readBlock(block here, cLoc) == MI_OK){
                        NFC_colour = cLoc[byte here];
                        UART_printf("Colour: %c\r\n", NFC_colour);
                        if (NFC_colour == 'R') {numberR += 1;}
                        else if (NFC_colour == 'G') {numberG += 1;}
                        else if (NFC_colour == 'B') {numberB += 1;}
                    }
                    RC522_stopCrypto();
                    RC522_halt();
                }
            }
        }

        if(RC522_request(PICC_REQALL, tagType) == MI_OK){
            if(RC522_anticoll(serNum) == MI_OK){

                uint8_t l = RC522_selectTag(serNum);
                uint8_t rLoc[18];
                uint8_t keyA[6] = {key here};

                if(RC522_auth(PICC_AUTHENT1A, 9, keyA, serNum) == MI_OK){
                    if(RC522_readBlock(block here, rLoc) == MI_OK){
                        NFC_direction = rLoc[byte here];
                        UART_printf("Direction: %c\r\n", NFC_direction);
                    }
                    RC522_stopCrypto();
                    RC522_halt();
                }
            }
        }
        return NFC_direction + NFC_colour + numberR + numberG + numberB;
    }

// Sets the LEDs to the correct colour and direction
int setLed(dir, color) {
    PxOUT &= ~(rL | rR | cR | cG | cB);
    if (dir == 'L') {
        if (color == 'R') {
            PxOUT |= (rL | cR);
        }
        else if (color == 'G') {
            PxOUT |= (rL | cG);
        }
        else if (color == 'B') {
            PxOUT |= (rL | cB);
        }
    }
    else if (dir == 'R') {
        if (color == 'R') {
            PxOUT |= (rR | cR);
        }
        else if (color == 'G') {
            PxOUT |= (rR | cG);
        }
        else if (color == 'B') {
            PxOUT |= (rR | cB);
        }
    }
    else {
        if (color == 'R') {
            PxOUT |= cR;
        }
        else if (color == 'G') {
            PxOUT |= cG;
        }
        else if (color == 'B') {
            PxOUT |= cB;
        }
    }
    return(0);
}

int main(void){
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT

    // Set clock speed to 1MHz or otherwise the code will not function.
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL  = CALDCO_1MHZ;

    P2DIR |= (rL | rR | cR | cG | cB);
    rfSetup();

    while(1){
        readTag();
        UART_printf("Test return values: %c\r\n", NFC_direction);
        UART_printf("and %c\r\n", NFC_colour);
        setLed(NFC_direction, NFC_colour);
        delay_ms(400);
    }
}
