/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#ifndef BB_SPI_H_
#define BB_SPI_H_

enum pin {
    PIN_MOSI = 0,
    PIN_CLK = 1,
    PIN_CS = 2,
    PIN_MISO_EN = 3,
    PIN_MISO = 4, // Value is ignored
};

void spiPause(void);
#define spiBegin() spiflash_bitbang_write(0)  //((0 << PIN_CLK) | (0 << PIN_CS))
#define spiEnd() spiflash_bitbang_write(1 << PIN_CS)
#define spiFree() spiflash_bitbang_en_write(0)

int spiRead(uint32_t addr, uint8_t *data, unsigned int count);
int spiIsBusy(void);

void spiEraseSecurityRegister(uint8_t reg);
int8_t spiWriteSecurityRegister(uint8_t reg, uint8_t addr, uint8_t *data, uint32_t size);
int8_t spiReadSecurityRegister(uint8_t reg, uint8_t addr, uint32_t size, uint8_t *buf);

int spiBeginErase4(uint32_t erase_addr);
int spiBeginErase32(uint32_t erase_addr);
int spiBeginErase64(uint32_t erase_addr);
int spiBeginWrite(uint32_t addr, const void *data, unsigned int count);
void spiEnableQuad(void);

uint32_t spiId(void);
uint64_t spiUniqueID(void);
void printSerial(void);
uint64_t getSerial(void);

int8_t setMotorDirMem(uint8_t *data);
int8_t getMotorDirMem(uint8_t *data);

int spiWrite(uint32_t addr, const uint8_t *data, unsigned int count);
uint8_t spiReset(void);
int spiInit(void);

void spi_single_tx(uint8_t out);
uint8_t spi_single_rx(void);

void spiHold(void);
void spiUnhold(void);
void spiSwapTxRx(void);


void IMUBegin(void);
void ADCBegin(void);
void IMUADCEnd(void);

#endif /* BB_SPI_H_ */
