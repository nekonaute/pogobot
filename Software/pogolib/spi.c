/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <generated/csr.h>
#include <spiflash.h>

#include <spi.h>

#ifdef CSR_SPI_CS_BASE
void IMUBegin(void)
{
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode

    spi_cs_spi_cs_n_write(2);           // IMU_CS_n low (bit 0)
	spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS)); // Ensure CLK is low
}

void ADCBegin(void)
{
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode

    spi_cs_spi_cs_n_write(1);           // ADC_CS_n low (bit 1)
	spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS)); // Ensure CLK is low
}

void IMUADCEnd(void)
{
	spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS));
    spi_cs_spi_cs_n_write(3);           // Both CS_n bit high

	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
}
#endif

void spi_single_tx(uint8_t out) {
	int bit;

	for (bit = 7; bit >= 0; bit--) {    // Get MSB first (7 bits only)
		if (out & (1 << bit)) {
			spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI));
			spiflash_bitbang_write((1 << PIN_CLK) | (1 << PIN_MOSI));
			spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI));
		} else {
			spiflash_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI));
			spiflash_bitbang_write((1 << PIN_CLK) | (0 << PIN_MOSI));
			spiflash_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI));
		}
	}
}

uint8_t spi_single_rx(void) {
	int bit = 0;
	uint8_t in = 0;

	spiflash_bitbang_write(1 << PIN_MISO_EN);

	while (bit++ < 8) {
		spiflash_bitbang_write((1 << PIN_MISO_EN) | (1 << PIN_CLK));
		in = (in << 1) | spiflash_miso_read();
		spiflash_bitbang_write((1 << PIN_MISO_EN) | (0 << PIN_CLK));
	}

	return in;
}

static uint8_t spi_read_status(void) {
	uint8_t val;

	spiBegin();
	spi_single_tx(0x05);
	val = spi_single_rx();
	spiEnd();
	return val;
}

static void wait_for_device_ready(void) {
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
    while( spiIsBusy() != 0 );
}
int spiIsBusy(void) {
  	return spi_read_status() & 1;
}

uint32_t spi_id;

uint32_t spiId(void) {
	spi_id = 0;
    uint8_t i;

	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
	spiBegin();
	spi_single_tx(0x9f);                // Read device id
    for(i=0; i<24; i++) {               // 64 bits data
		spiflash_bitbang_write((1 << PIN_MISO_EN) | (1 << PIN_CLK));
		spi_id = (spi_id << 1) | spiflash_miso_read();
		spiflash_bitbang_write(1 << PIN_MISO_EN);
	}
	spiEnd();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
	return spi_id;
}

uint64_t unique_id=0, serial_number=0;

uint64_t spiUniqueID(void) {
    uint8_t i;
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
    spiBegin();
    spi_single_tx(0x4b);                // Read Unique ID
    for(i=0; i<4; i++) {
        spi_single_tx(0x00);            // Dummy bytes
    }
    for(i=0; i<64; i++) {               // 64 bits data
		spiflash_bitbang_write((1 << PIN_MISO_EN) | (1 << PIN_CLK));
		unique_id = (unique_id << 1) | spiflash_miso_read();
		spiflash_bitbang_write(1 << PIN_MISO_EN);
	}
    spiEnd();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
    return unique_id;
}

void printSerial(void) {
    // Same as getSerial but print the string instead of just returning the number
    char buf[16];
    spiReadSecurityRegister(1, 0, 16, (uint8_t *)buf);
    if((buf[0] == 'S') & (buf[1] == 'N')) {
        for(uint8_t i=0; i<13; i++) {
            putchar(buf[i+3]);
        }
    }
}

uint64_t getSerial(void) {
    char buf[16], ser[13];
    char *c;
    uint8_t i;
    spiReadSecurityRegister(1, 0, 16, (uint8_t *)buf);
    if((buf[0] == 'S') & (buf[1] == 'N')) {
        for(i=0; i<6; i++) {
            ser[i] = buf[i+3];
            ser[i+6] = buf[i+10];
        }
        ser[12] = 0;
        serial_number = strtoull(ser, &c, 10);    // 10-base
        return serial_number;
    }
    else {
        return 0;
    }
}


/*
* use the second Security Register to memorize the motor direction
* [uint8_t R, uint8_t L, uint8_t M]
*/
int8_t setMotorDirMem(uint8_t *data) {

	spiEraseSecurityRegister(2);
	uint32_t size = sizeof(uint8_t) * 3;
	spiWriteSecurityRegister(2, 0, data, size);
	// writing a key to confirm
	uint8_t key = 42;
	spiWriteSecurityRegister(2, 3, &key, 1);

}

int8_t getMotorDirMem(uint8_t *data) {

	uint8_t key = 0;
	spiReadSecurityRegister(2, 3, 1, &key);
	if (key != 42)
		return;

	uint32_t size = sizeof(uint8_t) * 3;
	return spiReadSecurityRegister(2, 0, size, data);


}

void spiEraseSecurityRegister(uint8_t reg) {
    /* reg is 1, 2 or 3 to chose the corresponding register to be erased */
    if(reg > 3) {
        return;
    }
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode

	spiBegin();
	spi_single_tx(0x06);                // Enable Write-Enable Latch (WEL)
	spiEnd();

	spiBegin();
	spi_single_tx(0x44);
	spi_single_tx(0x0);                 // Address A23-A16
	spi_single_tx(reg<<4);              // Address A15-A08
	spi_single_tx(0x0);                 // Address A07-A00
	spiEnd();
    wait_for_device_ready();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
}

int8_t spiWriteSecurityRegister(uint8_t reg, uint8_t addr, uint8_t *data, uint32_t size) {
    /* reg is 1, 2 or 3 to chose the corresponding register to write to
     * addr is the address in that register (0 to 255)
     * data are the bytes to put in the security register
     * size is the number of bytes to copy */

    uint8_t i;
    if( (reg > 3) | (addr+size > 256) ) {
        return -1;
    }
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode

	spiBegin();
	spi_single_tx(0x06);                // Enable Write-Enable Latch (WEL)
	spiEnd();

	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
	spiBegin();
	spi_single_tx(0x42);
	spi_single_tx(0x0);                 // Address A23-A16
	spi_single_tx(reg<<4);              // Address A15-A08
	spi_single_tx(addr);                // Address A07-A00
    for( i=0; i< size; i++) {
        spi_single_tx(data[i]);
    }
	spiEnd();
    wait_for_device_ready();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
    return 0;
}

int8_t spiReadSecurityRegister(uint8_t reg, uint8_t addr, uint32_t size, uint8_t *buf) {
    /* reg is 1, 2 or 3 to chose the corresponding register to read from
     * addr is the address in that register (0 to 255)
     * size is the number of bytes to read */
    /* Example : read entire security register number 3 :
     *    spiReadSecurityRegister(3, 0, 256);                         */
    
    uint32_t i;
    if( (reg > 3) | (addr+size > 256) ) {
        return -1;
    }
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
	spiBegin();
	spi_single_tx(0x48);
	spi_single_tx(0x0);                 // Address A23-A16
	spi_single_tx(reg<<4);              // Address A15-A08
	spi_single_tx(addr);                // Address A07-A00
	spi_single_tx(0x0);                 // Dummy byte
    for(i=0; i<size; i++) {
        buf[i] = spi_single_rx();
    }
	spiEnd();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
    return 0;
}

int spiBeginErase4(uint32_t erase_addr) {
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
	spiBegin();
	spi_single_tx(0x06);                // Enable Write-Enable Latch (WEL)
	spiEnd();

	spiBegin();
	spi_single_tx(0x20);
	spi_single_tx(erase_addr >> 16);
	spi_single_tx(erase_addr >> 8);
	spi_single_tx(erase_addr >> 0);
	spiEnd();
    wait_for_device_ready();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
	return 0;
}

int spiBeginErase32(uint32_t erase_addr) {
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
	// Enable Write-Enable Latch (WEL)
	spiBegin();
	spi_single_tx(0x06);
	spiEnd();

	spiBegin();
	spi_single_tx(0x52);
	spi_single_tx(erase_addr >> 16);
	spi_single_tx(erase_addr >> 8);
	spi_single_tx(erase_addr >> 0);
	spiEnd();
    wait_for_device_ready();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
	return 0;
}

int spiBeginErase64(uint32_t erase_addr) {
	spiflash_bitbang_en_write(1);       // Enable bit-bang mode
	// Enable Write-Enable Latch (WEL)
	spiBegin();
	spi_single_tx(0x06);
	spiEnd();

	spiBegin();
	spi_single_tx(0xD8);
	spi_single_tx(erase_addr >> 16);
	spi_single_tx(erase_addr >> 8);
	spi_single_tx(erase_addr >> 0);
	spiEnd();
    wait_for_device_ready();
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
	return 0;
}

int spiBeginWrite(uint32_t addr, const void *v_data, unsigned int count) {
	const uint8_t write_cmd = 0x02;
	const uint8_t *data = v_data;
	unsigned int i;

	// Enable Write-Enable Latch (WEL)
	spiBegin();
	spi_single_tx(0x06);
	spiEnd();

	spiBegin();
	spi_single_tx(write_cmd);
	spi_single_tx(addr >> 16);
	spi_single_tx(addr >> 8);
	spi_single_tx(addr >> 0);
	for (i = 0; (i < count) && (i < 256); i++)
		spi_single_tx(*data++);
	spiEnd();

	return 0;
}

uint8_t spiReset(void) {
	// Writing 0xff eight times is equivalent to exiting QPI mode,
	// or if CFM mode is enabled it will terminate CFM and return
	// to idle.
	unsigned int i;
	spiBegin();
	for (i = 0; i < 8; i++)
		spi_single_tx(0xff);
	spiEnd();

	// Some SPI parts require this to wake up
	spiBegin();
	spi_single_tx(0xab);                // Read electronic signature
	spiEnd();

	return 0;
}

int spiInit(void) {

	// Ensure CS is deasserted and the clock is high
	spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS));
#ifdef CSR_SPI_CS_BASE
    spi_cs_spi_cs_n_write(3);           // Both CS_n bits high
#endif
	
	// Reset the SPI flash, which will return it to SPI mode even
	// if it's in QPI mode, and ensure the chip is accepting commands.
	spiReset();
	spiId();                            // Gets the ID (global variable)
    spiUniqueID();                      // Get the unique ID from SPI flash (global variable)
    getSerial();                        // Get serial number recorded in SPI flash's security registers
	spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
	return 0;
}

void spiHold(void) {
	spiBegin();
	spi_single_tx(0xb9);
	spiEnd();

}
void spiUnhold(void) {
	spiBegin();
	spi_single_tx(0xab);
	spiEnd();
}

