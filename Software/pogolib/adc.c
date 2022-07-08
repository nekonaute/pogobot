/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


// Code to read 10-bit ADC : Microchip MCP3004/8

#include <stdio.h>
#include <stdint.h>

#include <generated/mem.h>
#include <generated/csr.h>

#include <spi.h>
#include <adc.h>

uint32_t ADC_Read(uint8_t channel)
{
    uint8_t dataout[3], datain[3];
    int bit, i;

    dataout[0]=1;
    dataout[1]=0x80 | (channel << 4); // Single bit + 1st and 2nd bit of channel + 3rd channel bit unused + 2 zeros (total 8bits)

    ADCBegin(); // ADC_CSn low
    for( i=0; i< 3; i++)
    {
        datain[i]=0;
        for (bit = 7; bit >= 0; bit--) {
            if (dataout[i] & (1 << bit)) {
                spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI) | (1 << PIN_CS));
                spiflash_bitbang_write((1 << PIN_CLK) | (1 << PIN_MOSI) | (1 << PIN_CS));
                datain[i] = (datain[i] << 1) | spiflash_miso_read();
                spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI) | (1 << PIN_CS));
            } else {
                spiflash_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI) | (1 << PIN_CS));
                spiflash_bitbang_write((1 << PIN_CLK) | (0 << PIN_MOSI) | (1 << PIN_CS));
                datain[i] = (datain[i] << 1) | spiflash_miso_read();
                spiflash_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI) | (1 << PIN_CS));
            }
        }
    }
    IMUADCEnd();    // ADC_CSn high
    return( ((((uint32_t)datain[1])<<8) | datain[2] ) & 0x3FF ); // Two bits of datain[1] + 8 bits of datain[2]
}

