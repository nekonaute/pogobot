/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

// Make measurments with the LIS2MDL magnetometer (located on an add-on PCB on the SRAM footprint)

#include "magnetometer.h"

void magn_begin(void)
{
    spiflash_bitbang_en_write(1);       // Enable bit-bang mode

    spi_cs_spi_cs_n_write(3);           // ADC_CS_n & IMU_CS_n high
    spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS)); // Ensure CLK is low
}

void magn_end(void)
{
    spi_cs_spi_cs_n_write(7);           // Both CS_n bit high
    spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_CS)); 

    spiflash_bitbang_en_write(0);       // Enable memory-mapped mode
}

// *****
// Measurement of the magnetic field on the x,y,z axis. 
// Args : 
// x, y - x, y and z axis coordinates measured by the magnetometer (needs calibration)
// timeout_ms - timeout in ms.
// Returns :
// - 1 if successfull else 0
// *****
int magn_read_XYZ(int16_t* x, int16_t* y, int16_t* z, uint16_t timeout_ms)
{
    uint8_t check = 0x0;
    uint16_t temps_ecoule = 0;
    
    while (temps_ecoule < timeout_ms) {
        
        magn_begin();
        spi_single_tx(LIS2MDL_STATUS_REG | LIS2MDL_SPI_READ_MASK);
        check = spi_single_rx();
        magn_end();
        
        if (check & LIS2MDL_STATUS_ZYXDA) {
            
            magn_begin();
            spi_single_tx(LIS2MDL_OUTX_L_REG | LIS2MDL_SPI_READ_MASK);
            
            *x = spi_single_rx();
            *x = (spi_single_rx() << 8) | *x;
            *y = spi_single_rx();
            *y = (spi_single_rx() << 8) | *y;
            *z = spi_single_rx();
            *z = (spi_single_rx() << 8) | *z;
            magn_end();
            
            return(0);
        }
        
        msleep(1); 
        temps_ecoule++;
    }
    
    return(1); 
}

// *******
// Configuration :
// - Temp Comp ON
// - ODR = 50 Hz, MD = continuous mode
// - digital LPF ON
// - Offset cancellation ON
// - 4WSPI ON
// - BDU ON
// Arg :
// - None
// Returns :
// - 1 if successfully detected the magnetometer, 0 else
// ******
uint8_t magn_check(void)
{
    magn_begin();
    spi_single_tx(LIS2MDL_CFG_REG_A);
    spi_single_tx(0x40); 
    magn_end();

    msleep(100);

    magn_begin();
    spi_single_tx(LIS2MDL_CFG_REG_A);
    spi_single_tx(0x88); 
    magn_end();

    msleep(60);

    magn_begin();
    spi_single_tx(LIS2MDL_CFG_REG_B);
    spi_single_tx(0x03); 
    magn_end();

    msleep(60);

    magn_begin();
    spi_single_tx(LIS2MDL_CFG_REG_C);
    spi_single_tx(0x14); 
    magn_end();

    msleep(60);

    uint8_t buf = 0x00;
    magn_begin();
    spi_single_tx(LIS2MDL_WHO_AM_I | LIS2MDL_SPI_READ_MASK);
    buf = spi_single_rx(); 
    magn_end();

    if(buf == LIS2MDL_WHO_AM_I_VALUE){ 
        return 0; 
    }

    return 1; 
}