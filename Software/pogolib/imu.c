/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


// Read/Write ICM-20689 IMU chip
// inspired by https://github.com/finani/ICM20689

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <generated/mem.h>
#include <generated/csr.h>

#include "spi.h"
#include "imu.h"
#include "sleep.h"

float _accelScale = 0.0;
float _gyroScale = 0.0;
const float G = 9.807f;
const float _d2r = 3.14159265359f/180.0f;
const float _r2d = 180.0f/3.14159265359f;
const float _tempScale = 333.87f;
const float _tempOffset = 21.0f;


void imu_single_tx(uint8_t);
uint8_t imu_single_rx(void);
void imu_write(uint8_t);
void imu_read(uint8_t);

void imu_write(uint8_t out)     // Write to a register
{
    out = out & 0x7F;           // Write bit set (0)
    imu_single_tx(out);
}
void imu_read(uint8_t out)      // Read a register
{
    out = out | 0x80;           // Read bit set (1)
    imu_single_tx(out);
}

void IMU_Init(void)
{
    uint8_t result;
    result = IMU_ReadByte(WHO_AM_I);
    if(result != 0x98)
    {
        printf("Error communicating with IMU ICM-20689\n");
        return;
    }
    IMU_WriteByte(PWR_MGMT_1, CLOCK_SEL_PLL);    // Get out of sleep mode
    IMU_WriteByte(USER_CTRL, 0x10);     // Disable I2C interface
    IMU_WriteByte(PWR_MGMT_2, SEN_ENABLE);    // Enable accelerometer and gyro
    IMU_WriteByte(ACCEL_CONFIG,ACCEL_FS_SEL_16G); // setting accel range to 16G as default
    _accelScale = G * 16.0f/32767.5f; // setting the accel scale to 16G
    IMU_WriteByte(ACCEL_CONFIG2,ACCEL_DLPF_218HZ); // setting bandwidth to 218Hz as default
    IMU_WriteByte(SMPLRT_DIV,0x00); // setting the sample rate divider to 0 as default
    IMU_WriteByte(GYRO_CONFIG,GYRO_FS_SEL_2000DPS); // setting the gyro range to 2000DPS as default
    _gyroScale = 2000.0f/32767.5f * _d2r; // setting the gyro scale to 2000DPS


}

void IMU_GoToSleep(void)
{
    uint8_t result;
    result = IMU_ReadByte(WHO_AM_I);
    if(result != 0x98)
    {
        printf("Error communicating with IMU ICM-20689\n");
        return;
    }
    IMU_WriteByte(PWR_MGMT_1, PWR_RESET);    // Go to sleep mode
    msleep(10);
    result = IMU_ReadByte(PWR_MGMT_1);
    if(result == PWR_SLEEP)
    {
        printf("sleep mode ok\n");
    } else
    {
        printf("something went wrong %d\n", result);
    }

}

uint8_t IMU_ReadByte(uint8_t address)
{
    uint8_t temp = 0;
    IMUBegin();
    imu_read(address);          // Give the device the address to read
    temp = imu_single_rx();
    IMUADCEnd();
    return temp;
}

uint32_t IMU_ReadWord(uint8_t address)
{
    uint32_t temp = 0;
    IMUBegin();
    imu_read(address);          // Give the device the address to read
    temp = imu_single_rx();
    temp = (temp << 8) | imu_single_rx();
    IMUADCEnd();
    return temp;
}

void IMU_WriteByte(uint8_t address, uint8_t data)
{
    IMUBegin();
    imu_write(address);         // Give the device the address to write to
    imu_single_tx(data);        // And data
    IMUADCEnd();
}

void imu_single_tx(uint8_t out) {
    int bit;
    for (bit = 7; bit >= 0; bit--) { // 7 to 0 because MSB first
        if (out & (1 << bit)) {
            spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI) | (1 << PIN_CS));
            spiflash_bitbang_write((1 << PIN_CLK) | (1 << PIN_MOSI) | (1 << PIN_CS));
            spiflash_bitbang_write((0 << PIN_CLK) | (1 << PIN_MOSI) | (1 << PIN_CS));
        } else {
            spiflash_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI) | (1 << PIN_CS));
            spiflash_bitbang_write((1 << PIN_CLK) | (0 << PIN_MOSI) | (1 << PIN_CS));
            spiflash_bitbang_write((0 << PIN_CLK) | (0 << PIN_MOSI) | (1 << PIN_CS));
        }
    }
}

uint8_t imu_single_rx(void) {
    int bit = 0;
    uint8_t in = 0;

    spiflash_bitbang_write((1 << PIN_MISO_EN) | (0 << PIN_CLK) | (1 << PIN_CS));

    while (bit++ < 8) {
        spiflash_bitbang_write((1 << PIN_MISO_EN) | (1 << PIN_CLK) | (1 << PIN_CS));
        in = (in << 1) | spiflash_miso_read();
        spiflash_bitbang_write((1 << PIN_MISO_EN) | (0 << PIN_CLK) | (1 << PIN_CS));
    }

    return in;
}


int readAcc(float * acc) {

    uint8_t _buffer[6] = {};
    int16_t _accCounts[3] = {};

    // grab the data from the ICM20689
    _buffer[0] = IMU_ReadByte(ACCEL_XOUT_H);
    _buffer[1] = IMU_ReadByte(ACCEL_XOUT_L);
    _buffer[2] = IMU_ReadByte(ACCEL_YOUT_H);
    _buffer[3] = IMU_ReadByte(ACCEL_YOUT_L);
    _buffer[4] = IMU_ReadByte(ACCEL_ZOUT_H);
    _buffer[5] = IMU_ReadByte(ACCEL_ZOUT_L);

    // combine into 16 bit values
    _accCounts[0] = (((int16_t)_buffer[0]) << 8) | _buffer[1];
    _accCounts[1] = (((int16_t)_buffer[2]) << 8) | _buffer[3];
    _accCounts[2] = (((int16_t)_buffer[4]) << 8) | _buffer[5];
    acc[0] = _accCounts[1] * _accelScale;
    acc[1] = _accCounts[0] * _accelScale;
    acc[2] = -1 *(_accCounts[2] * _accelScale);

    return 1;
}

int readGyro(float * gyro) {

    uint8_t _buffer[6] = {};
    int16_t _gyroCounts[3] = {};


    // grab the data from the ICM20689
    _buffer[0] = IMU_ReadByte(GYRO_XOUT_H);
    _buffer[1] = IMU_ReadByte(GYRO_XOUT_L);
    _buffer[2] = IMU_ReadByte(GYRO_YOUT_H);
    _buffer[3] = IMU_ReadByte(GYRO_YOUT_L);
    _buffer[4] = IMU_ReadByte(GYRO_ZOUT_H);
    _buffer[5] = IMU_ReadByte(GYRO_ZOUT_L);

    // combine into 16 bit values
    _gyroCounts[0] = (((int16_t)_buffer[0]) << 8) | _buffer[1];
    _gyroCounts[1] = (((int16_t)_buffer[2]) << 8) | _buffer[3];
    _gyroCounts[2] = (((int16_t)_buffer[4]) << 8) | _buffer[5];

    gyro[0] = _gyroCounts[1] * _gyroScale;
    gyro[1] = _gyroCounts[0] * _gyroScale;
    gyro[2] = -1 *_gyroCounts[2] * _gyroScale;


  return 1;
}

float readTemp(void) {
    uint8_t _buffer[2] = {};
    int16_t _tCounts = 0;

    _buffer[0] = IMU_ReadByte(TEMP_OUT_H);
    _buffer[1] = IMU_ReadByte(TEMP_OUT_L);

    _tCounts = (((int16_t)_buffer[0]) << 8) | _buffer[1];

    return ((((float) _tCounts) - _tempOffset)/_tempScale) + _tempOffset;
}
