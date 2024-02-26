/*
	File: Mfrc522.cpp
	Mfrc522 - Library for communicating with MFRC522 based NFC Reader/Writers
	Created by Eelco Rouw - Mainly based on code from Grant Gibson (www.grantgibson.co.uk) and Dr.Leong ( WWW.B2CQSHOP.COM )
	Lightly modified by Frank Milburn
	Lightly modified by Roy Bakker C++ --> C
	Released into the public domain
*/

#include "Mfrc522.h"
#include "SPI.h"

void RC522_init(int chipSelectPin, int NRSTPD)
{
    _chipSelectPin = chipSelectPin;
    P1DIR |= _chipSelectPin;
    P1OUT |= _chipSelectPin;

    _NRSTPD = NRSTPD;
	P1DIR |= _NRSTPD;
	P1OUT |= _NRSTPD;

    RC522_reset();

    //Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
    RC522_writeReg(TModeReg, 0x8D);       //Tauto=1; f(Timer) = 6.78MHz/TPreScaler
    RC522_writeReg(TPrescalerReg, 0x3E);  //TModeReg[3..0] + TPrescalerReg
    RC522_writeReg(TReloadRegL, 30);
    RC522_writeReg(TReloadRegH, 0);

    RC522_writeReg(TxAutoReg, 0x40);      //100%ASK
    RC522_writeReg(ModeReg, 0x3D);

    //RC522_clearBitMask(Status2Reg, 0x08);       //MFCrypto1On=0
    //RC522_writeReg(RxSelReg, 0x86);     //RxWait = RxSelReg[5..0]
    //RC522_writeReg(RFCfgReg, 0x7F);         //RxGain = 48dB

    RC522_antennaOn();
}

/*
 * Function: WriteReg
 * Description: write a byte data into one register of MR RC522
 * Input parameter: addr--register addressï¼›val--the value that need to write in
 * Return: Null
 */
void RC522_writeReg(unsigned char addr, unsigned char val)
{
	P1OUT &= ~(_chipSelectPin);

	SPI_transfer((addr<<1)&0x7E);
	SPI_transfer(val);
	
	P1OUT |= _chipSelectPin;
}

/*
 * Function: ReadReg
 * Description: read a byte data into one register of MR RC522
 * Input parameter: addr--register address
 * Return: return the read value
 */
unsigned char RC522_readReg(unsigned char addr)
{
	unsigned char val;
    P1OUT &= ~(_chipSelectPin);

	SPI_transfer(((addr<<1)&0x7E) | 0x80);
	val = SPI_transfer(0x00);
	
	P1OUT |= _chipSelectPin;
	
	return val;	
}

/*
 * Function: SetBitMask
 * Description: set RC522 register bit
 * Input parameter: reg--register address;mask--value
 * Return: null
 */
void RC522_setBitMask(unsigned char reg, unsigned char mask)
{
	unsigned char tmp;
	tmp = RC522_readReg(reg);
	RC522_writeReg(reg, tmp | mask);  // set bit mask
}

/*
 * Function: ClearBitMask
 * Description: clear RC522 register bit
 * Input parameter: reg--register address;mask--value
 * Return: null
 */
void RC522_clearBitMask(unsigned char reg, unsigned char mask)
{
	unsigned char tmp;
	tmp = RC522_readReg(reg);
	RC522_writeReg(reg, tmp & (~mask));  // clear bit mask
} 


/*
 * Function: AntennaOn
 * Description: Turn on antenna, every time turn on or shut down antenna need at least 1ms delay
 * Input parameter: null
 * Return: null
 */
void RC522_antennaOn(void)
{
	unsigned char temp;

	temp = RC522_readReg(TxControlReg);
	if (!(temp & 0x03))
	{
		RC522_setBitMask(TxControlReg, 0x03);
	}
}


/*
 * Function: AntennaOff
 * Description: Turn off antenna, every time turn on or shut down antenna need at least 1ms delay
 * Input parameter: null
 * Return: null
 */
void RC522_antennaOff(void)
{
	RC522_clearBitMask(TxControlReg, 0x03);
}

/**
 * Set the MFRC522 Receiver Gain (RxGain) to value specified by given mask.
 * See 9.3.3.6 / table 98 in http://www.nxp.com/documents/data_sheet/MFRC522.pdf
 * NOTE: Given mask is scrubbed with (0x07<<4)=01110000b as RCFfgReg may use reserved bits.
 */
void RC522_setAntennaGain(unsigned char mask) {
    RC522_clearBitMask(RFCfgReg, (0x07<<4));      // clear needed to allow 000 pattern
    RC522_setBitMask(RFCfgReg, (mask & 0x07)<<4); // only set RxGain[2:0] bits
}

/*
 * Function: Reset
 * Description:  reset RC522
 * Input parameter: null
 * Return: null
 */
void RC522_reset(void)
{
	RC522_writeReg(CommandReg, PCD_RESETPHASE);
}

/*
 * Function: Request
 * Description: Searching card, read card type
 * Input parameter: reqMode--search methodsï¼Œ
 *			 TagType--return card types
 *			 	0x4400 = Mifare_UltraLight
 *				0x0400 = Mifare_One(S50)
 *				0x0200 = Mifare_One(S70)
 *				0x0800 = Mifare_Pro(X)
 *				0x4403 = Mifare_DESFire
 * Return: return MI_OK if successed
 */
unsigned char RC522_request(unsigned char reqMode, unsigned char *TagType)
{
	unsigned char status;  
	unsigned int backBits;

	RC522_writeReg(BitFramingReg, 0x07);		//TxLastBists = BitFramingReg[2..0]	???
	
	TagType[0] = reqMode;
	status = RC522_toCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10))
	{    
		status = MI_ERR;
	}
   
	return status;
}

/*
 * Function: ToCard
 * Description: communicate between RC522 and ISO14443
 * Input parameter: command--MF522 command bits
 *			 sendData--send data to card via rc522
 *			 sendLen--send data length		 
 *			 backData--the return data from card
 *			 backLen--the length of return data
 * Return: return MI_OK if successed
 */
unsigned char RC522_toCard(unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen)
{
	unsigned char status = MI_ERR;
	unsigned char irqEn = 0x00;
	unsigned char waitIRq = 0x00;
	unsigned char lastBits;
	unsigned char n;
	unsigned int i;

	switch (command)
	{
		case PCD_AUTHENT:
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE:
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
			break;
	}
   
	RC522_writeReg(CommIEnReg, irqEn|0x80);
	RC522_clearBitMask(CommIrqReg, 0x80);
	RC522_setBitMask(FIFOLevelReg, 0x80);
	
	RC522_writeReg(CommandReg, PCD_IDLE);

	for (i=0; i<sendLen; i++)
	{   
		RC522_writeReg(FIFODataReg, sendData[i]);
	}

	RC522_writeReg(CommandReg, command);
	if (command == PCD_TRANSCEIVE)
	{    
		RC522_setBitMask(BitFramingReg, 0x80);		//StartSend=1,transmission of data starts
	}   

	i = 2000;
	do 
	{
		n = RC522_readReg(CommIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x01) && !(n&waitIRq));

	RC522_clearBitMask(BitFramingReg, 0x80);			//StartSend=0
	
	if (i != 0)
	{    
		if(!(RC522_readReg(ErrorReg) & 0x1B))	//BufferOvfl Collerr CRCErr ProtecolErr
		{
			status = MI_OK;
			if (n & irqEn & 0x01)
			{   
				status = MI_NOTAGERR;			//??   
			}

			if (command == PCD_TRANSCEIVE)
			{
				n = RC522_readReg(FIFOLevelReg);
				lastBits = RC522_readReg(ControlReg) & 0x07;
				if (lastBits)
				{   
					*backLen = (n-1)*8 + lastBits;   
				}
				else
				{   
					*backLen = n*8;   
				}
				if (n == 0)
				{   
					n = 1;    
				}
				if (n > MAX_LEN)
				{   
					n = MAX_LEN;   
				}
				
				for (i=0; i<n; i++)
				{   
					backData[i] = RC522_readReg(FIFODataReg);
				}
			}
		}
		else
		{   
			status = MI_ERR;  
		}
	}
	
//RC522_setBitMask(ControlReg,0x80);           //timer stops
//RC522_writeReg(CommandReg, PCD_IDLE);

	return status;
}


/*
 * Function: MFRC522_Anticoll
 * Description: Prevent conflict, read the card serial number 
 * Input parameter: serNum--return the 4 bytes card serial number, the 5th byte is recheck byte
 * Return: return MI_OK if successed
 */
unsigned char RC522_anticoll(unsigned char *serNum)
{
	unsigned char status;
	unsigned char i;
	unsigned char serNumCheck=0;
	unsigned int unLen;


//RC522_clearBitMask(Status2Reg, 0x08);		//TempSensclear
//RC522_clearBitMask(CollReg,0x80);			//ValuesAfterColl
	RC522_writeReg(BitFramingReg, 0x00);		//TxLastBists = BitFramingReg[2..0]
	serNum[0] = PICC_ANTICOLL;
	serNum[1] = 0x20;
	status = RC522_toCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

	if (status == MI_OK)
	{
		for (i=0; i<4; i++)
		{   
			serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i])
		{   
			status = MI_ERR;    
		}
	}

	//RC522_setBitMask(CollReg, 0x80);		//ValuesAfterColl=1

	return status;
}


/*
 * Function: CalulateCRC
 * Description: Use MF522 to calculate CRC
 * Input parameter: pIndata--the CRC data need to be readï¼Œlen--data lengthï¼ŒpOutData-- the caculated result of CRC
 * Return: Null
 */
void RC522_calculateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
	unsigned char i, n;

	RC522_clearBitMask(DivIrqReg, 0x04);			//CRCIrq = 0
	RC522_setBitMask(FIFOLevelReg, 0x80);
	//RC522_writeReg(CommandReg, PCD_IDLE);

	for (i=0; i<len; i++)
	{   
		RC522_writeReg(FIFODataReg, *(pIndata+i));
	}
	RC522_writeReg(CommandReg, PCD_CALCCRC);

	i = 0xFF;
	do 
	{
		n = RC522_readReg(DivIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x04));			//CRCIrq = 1

	pOutData[0] = RC522_readReg(CRCResultRegL);
	pOutData[1] = RC522_readReg(CRCResultRegM);
}


/*
 * Function: SelectTag
 * Description: Select card, read card storage volume
 * Input parameter: serNum--Send card serial number
 * Return: return the card storage volume
 */
unsigned char RC522_selectTag(unsigned char *serNum)
{
	unsigned char i;
	unsigned char status;
	unsigned char size;
	unsigned int recvBits;
	unsigned char buffer[9]; 

	//RC522_clearBitMask(Status2Reg, 0x08);			//MFCrypto1On=0

	buffer[0] = PICC_SElECTTAG;
	buffer[1] = 0x70;
	for (i=0; i<5; i++)
	{
		buffer[i+2] = *(serNum+i);
	}
	RC522_calculateCRC(buffer, 7, &buffer[7]);		//??
	status = RC522_toCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
	
	if ((status == MI_OK) && (recvBits == 0x18))
	{   
		size = buffer[0]; 
	}
	else
	{   
		size = 0;    
	}

	return size;
}


/*
 * Function: Auth
 * Description: verify card password
 * Input parametersï¼šauthMode--password verify mode
                 0x60 = verify A password key 
                 0x61 = verify B password key 
             BlockAddr--Block address
             Sectorkey--Block password
             serNum--Card serial number ï¼Œ4 bytes
 * Return: return MI_OK if successed
 */
unsigned char RC522_auth(unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum)
{
	unsigned char status;
	unsigned int recvBits;
	unsigned char i;
	unsigned char buff[12]; 

	buff[0] = authMode;
	buff[1] = BlockAddr;
	for (i=0; i<6; i++)
	{    
		buff[i+2] = *(Sectorkey+i);   
	}
	for (i=0; i<4; i++)
	{    
		buff[i+8] = *(serNum+i);   
	}
	status = RC522_toCard(PCD_AUTHENT, buff, 12, buff, &recvBits);

	if ((status != MI_OK) || (!(RC522_readReg(Status2Reg) & 0x08)))
	{   
		status = MI_ERR;   
	}
    
	return status;
}

/**
 * Used to exit card from authenticated state.
 * Remember to call this function after communicating with an authenticated card, otherwise no new communications can start.
 */
void RC522_stopCrypto() {
    // Clear MFCrypto1On bit
     RC522_clearBitMask(Status2Reg, 0x08); // Status2Reg[7..0] bits are: TempSensClear I2CForceHS reserved reserved MFCrypto1On ModemState[2:0]
}

/*
 * Function: ReadBlock
 * Description: Read data 
 * Input parametersï¼šblockAddr--block address;recvData--the block data which are read
 * Return: return MI_OK if successed
 */
unsigned char RC522_readBlock(unsigned char blockAddr, unsigned char *recvData)
{
	unsigned char status;
	unsigned int unLen;

	recvData[0] = PICC_READ;
	recvData[1] = blockAddr;
	RC522_calculateCRC(recvData,2, &recvData[2]);
	status = RC522_toCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

	if ((status != MI_OK) || (unLen != 0x90))
	{
		status = MI_ERR;
	}
	
	return status;
}


/*
 * Function: WriteBlock
 * Description: write block data
 * Input parametersï¼šblockAddr--block address;writeData--Write 16 bytes data into block
 * Return: return MI_OK if successed
 */
unsigned char RC522_writeBlock(unsigned char blockAddr, unsigned char *writeData)
{
	unsigned char status;
	unsigned int recvBits;
	unsigned char i;
	unsigned char buff[18]; 
    
	buff[0] = PICC_WRITE;
	buff[1] = blockAddr;
	RC522_calculateCRC(buff, 2, &buff[2]);
	status = RC522_toCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

	if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
	{   
		status = MI_ERR;   
	}

	if (status == MI_OK)
	{
		for (i=0; i<16; i++)
		{    
			buff[i] = *(writeData+i);   
		}
		RC522_calculateCRC(buff, 16, &buff[16]);
		status = RC522_toCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

		if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
		{   
			status = MI_ERR;   
		}
	}
	
	return status;
}


/*
 * Function: Halt
 * Description: Command the cards into sleep mode
 * Input parameters: null
 * Return: status
 */
unsigned char RC522_halt(void)
{
	unsigned char status;
	unsigned int unLen;
	unsigned char buff[4]; 

	buff[0] = PICC_HALT;
	buff[1] = 0;
	RC522_calculateCRC(buff, 2, &buff[2]);
 
	status = RC522_toCard(PCD_TRANSCEIVE, buff, 4, buff,&unLen);

	return status;
}
