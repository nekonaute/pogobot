/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#ifndef IMU_H
#define IMU_H

#define WHO_AM_I     0x75
#define USER_CTRL    0x6A
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48


#define ACCEL_OUT  0x3B
#define GYRO_OUT  0x43
#define TEMP_OUT  0x41
#define ACCEL_CONFIG   0x1C
#define ACCEL_FS_SEL_2G   0x00
#define ACCEL_FS_SEL_4G   0x08
#define ACCEL_FS_SEL_8G   0x10
#define ACCEL_FS_SEL_16G   0x18
#define GYRO_CONFIG   0x1B
#define GYRO_FS_SEL_250DPS   0x00
#define GYRO_FS_SEL_500DPS   0x08
#define GYRO_FS_SEL_1000DPS   0x10
#define GYRO_FS_SEL_2000DPS   0x18
#define GYRO_FCHOICE_B_8173HZ   0x01
#define GYRO_FCHOICE_B_3281HZ   0x10
#define ACCEL_CONFIG2   0x1D
#define ACCEL_DLPF_218HZ   0x01
#define ACCEL_DLPF_99HZ   0x02
#define ACCEL_DLPF_45HZ   0x03
#define ACCEL_DLPF_21HZ   0x04
#define ACCEL_DLPF_10HZ   0x05
#define ACCEL_DLPF_5HZ   0x06
#define ACCEL_DLPF_420HZ   0x07
#define ACCEL_DLPF_1046HZ   0x08
#define CONFIG   0x1A
#define GYRO_DLPF_250HZ   0x00
#define GYRO_DLPF_176HZ   0x01
#define GYRO_DLPF_92HZ   0x02
#define GYRO_DLPF_41HZ   0x03
#define GYRO_DLPF_20HZ   0x04
#define GYRO_DLPF_10HZ   0x05
#define GYRO_DLPF_5HZ   0x06
#define SMPLRT_DIV   0x19
#define INT_PIN_CFG   0x37
#define INT_ENABLE   0x38
#define INT_DISABLE   0x00
#define INT_HOLD_ANY   0x30
#define INT_PULSE_50US   0x00
#define INT_WOM_EN   0xE0
#define INT_RAW_RDY_EN   0x01
#define INT_STATUS   0x3A
#define PWR_MGMT_1   0x6B
#define PWR_CYCLE   0x20
#define PWR_RESET   0x80
#define CLOCK_SEL_PLL   0x01
#define PWR_MGMT_2   0x6C
#define SEN_ENABLE   0x00
#define DIS_GYRO   0x07
#define DIS_ACC   0x38
#define MOT_DETECT_CTRL   0x69
#define ACCEL_INTEL_EN   0x80
#define ACCEL_INTEL_MODE   0x40
#define ACCEL_WOM_THR   0x1F
#define WHO_AM_I   0x75
#define FIFO_EN   0x23
#define FIFO_TEMP   0x80
#define FIFO_GYRO   0x70
#define FIFO_ACCEL   0x08
#define FIFO_COUNT   0x72
#define FIFO_READ   0x74

void IMU_Init(void);
uint32_t IMU_ReadWord(uint8_t address);
uint8_t IMU_ReadByte(uint8_t address);
void IMU_WriteWord(uint8_t address, uint8_t data);
void IMU_WriteByte(uint8_t address, uint8_t data);

int readAcc(float* acc);
int readGyro(float * gyro);
float readTemp(void);

#endif //IMU_H
