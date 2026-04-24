/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#ifndef MAGNETOMETER_H
#define MAGNETOMETER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pogobot.h"
#include "sleep.h" 
#include <math.h>

// ---------------------------------------------------------
// LIS2MDL Register Map
// ---------------------------------------------------------
#define LIS2MDL_OFFSET_X_REG_L      0x45
#define LIS2MDL_OFFSET_X_REG_H      0x46
#define LIS2MDL_OFFSET_Y_REG_L      0x47
#define LIS2MDL_OFFSET_Y_REG_H      0x48
#define LIS2MDL_OFFSET_Z_REG_L      0x49
#define LIS2MDL_OFFSET_Z_REG_H      0x4A

#define LIS2MDL_WHO_AM_I            0x4F

#define LIS2MDL_CFG_REG_A           0x60
#define LIS2MDL_CFG_REG_B           0x61
#define LIS2MDL_CFG_REG_C           0x62
#define LIS2MDL_INT_CRTL_REG        0x63
#define LIS2MDL_INT_SOURCE_REG      0x64
#define LIS2MDL_INT_THS_L_REG       0x65
#define LIS2MDL_INT_THS_H_REG       0x66
#define LIS2MDL_STATUS_REG          0x67

#define LIS2MDL_OUTX_L_REG          0x68
#define LIS2MDL_OUTX_H_REG          0x69
#define LIS2MDL_OUTY_L_REG          0x6A
#define LIS2MDL_OUTY_H_REG          0x6B
#define LIS2MDL_OUTZ_L_REG          0x6C
#define LIS2MDL_OUTZ_H_REG          0x6D

#define LIS2MDL_TEMP_OUT_L_REG      0x6E
#define LIS2MDL_TEMP_OUT_H_REG      0x6F

// ---------------------------------------------------------
// LIS2MDL Special values
// ---------------------------------------------------------
#define LIS2MDL_SPI_READ_MASK       0x80
#define LIS2MDL_WHO_AM_I_VALUE      0x40
#define LIS2MDL_STATUS_XDA          0x01 // X-axis new data available
#define LIS2MDL_STATUS_YDA          0x02 // Y-axis new data available
#define LIS2MDL_STATUS_ZDA          0x04 // Z-axis new data available
#define LIS2MDL_STATUS_ZYXDA        0x08 // X, Y, and Z-axis new data available

// ---------------------------------------------------------
// Function Prototypes
// ---------------------------------------------------------
void magn_begin(void);
void magn_end(void);
uint8_t magn_check(void);
int magn_read_XYZ(int16_t* x, int16_t* y, int16_t* z, uint16_t timeout_ms);

#endif