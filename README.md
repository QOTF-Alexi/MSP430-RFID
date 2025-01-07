Warning: this code does not work yet, you will need to find the issues and correct those.
At least it is possible to read two values from the tag at once.
Testing code and debugging info is provided.

Since there's no sectors defined in the library, you will need to count up the blocks (hint: the blocks start at 1).
See the NXP datasheets for info on how many sectors and blocks there are.

Has been thoroughly tested with the MSP430G2553IN20 microcontroller and the MSP-EXP430G2ET Launchpad

```
/*
 * Defused slightly by Foxydiot
 * This code is Unlicensed. See http://unlicense.org and the UNLICENSE file for more info.
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
```
