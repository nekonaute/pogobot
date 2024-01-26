/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


// SPDX-License-Identifier: BSD-Source-Code

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <generated/csr.h>

#include <command.h>
#include <helpers.h>

#include <boot.h>
#include <ir_boot.h>

#ifdef CSR_IR_RX0_BASE
#include <pogobot.h>
#include <spi.h>
#endif

/*
 * Command "Go to standby mode"
 */
static void go_to_standby_handler(int nb_params, char **params) {

    printf("go to standby mode \n");
    extern uint8_t standby_status; 
    standby_status = 1; 
    IMU_GoToSleep();
}
define_command(goto_standby, go_to_standby_handler, "Go to standby mode", POGO_CMDS);

/*
 * Command "Go to normal mode"
 */
static void go_to_normal_handler(int nb_params, char **params) {

    printf("go to normal mode \n");
    extern uint8_t standby_status;
    standby_status = 0; 
    IMU_Init();
}
define_command(goto_normal, go_to_normal_handler, "Go to normal mode", POGO_CMDS);

/*
 * Command "Register a new robot"
 */
static void add_robot_handler(int nb_params, char **params) {
    //char *c;
    //char input[256];
    if ( nb_params != 1 ) {
        printf( "Usage: add_robot [serial_number]\n\
       [serial_number] must be in the form SN:YYYYMM-000001\n");
        return;
    }
//    printf("Enter batch production's date: ");
//    scanf();
    printf("Enter serial in the form SN:YYYYMM-000001 : %s\n", params[0]);
    spiEraseSecurityRegister(1);
    spiWriteSecurityRegister(1, 0, (uint8_t *)params[0], strlen(params[0]));
    printf("Done\n");
}
define_command(write_serial, add_robot_handler, "Write serial number to security register of the SPI flash", POGO_CMDS);

/*
 * Command "Display all robot's Ids"
 */
static void robot_id_handler(int nb_params, char **params) {

    extern uint64_t unique_id, serial_number;

    if(unique_id != 0) 
        printf(" Unique ID: 0x%08lx%08lx\n", (uint32_t)(unique_id>>32), (uint32_t)(unique_id & 0xffffffff));
    if(serial_number != 0) { 
        printf(" Serial #: 0x%08lx%08lx , ", (uint32_t)(serial_number>>32), (uint32_t)(serial_number & 0xffffffff));
        printSerial();
    }
    printf("\n Return of pogobot_helper_getid : 0x%x\n", pogobot_helper_getid());

}
define_command(robot_id, robot_id_handler, "Display all robot's Ids", POGO_CMDS);

/*
 * Erase SPI Security Registers
 */
static void spi_sr_erase_handler(int nb_params, char **params) {
	char *c;
    if ( nb_params != 1 ) {
        printf( "Usage: spi_sr_erase [reg_number]\n\
       [reg_number] must be 1, 2 or 3\n");
        return;
    }

	uint8_t reg  = (uint8_t)strtoul( params[0], &c, 0 );
    spiEraseSecurityRegister(reg);
}
define_command(spi_sr_erase, spi_sr_erase_handler, "Erase SPI security registers", POGO_CMDS);

/*
 * Read data from SPI security registers
 */
static void spi_sr_read_handler(int nb_params, char **params) {
	char *c;
    uint8_t reg=1, addr=0, result;
    uint8_t buf[256];
    uint32_t i, length=256;
    if( ( nb_params > 3  ) || ( nb_params == 0 ) ) {
        printf( "Usage: spi_sr_read [reg_number] [address] [length]\n\
       [reg_number] can be 1, 2 or 3\n\
       [address] can be zero to 255\n\
       [length] can be one to 256\n\
       If no argument is given, the first register is displayed\n");
    }
    if ( nb_params >= 1 ) {
        reg  = (uint8_t)strtoul( params[0], &c, 0 );
    }
    if ( nb_params >= 2 ) {
        
        addr = (uint8_t)strtoul( params[1], &c, 0 );
    }
    if ( nb_params >= 3 ) {
        length = (uint32_t)strtoul( params[2], &c, 0 );
    }
    if( ( reg < 1) || (reg > 3) ) {
        printf("  [reg_number] can be 1, 2 or 3\n");
        return;
    }
    if( length > 256 ) {
        printf("  [length] can be one to 256\n");
        return;
    }
    result = spiReadSecurityRegister(reg, addr, length, buf);
    if( result == 0 ) {
        printf("Displaying security register %d, starting at address %d\n", reg, addr);
        for(i=0; i<length; i++) {
            printf("0x%02x: 0x%02x\t",(uint8_t)i, buf[i]);
        }
        printf("\n");
    }
}
define_command(spi_sr_read, spi_sr_read_handler, "Read SPI security registers", POGO_CMDS);

/*
 * Write data to SPI security registers
 */
static void spi_sr_write_handler(int nb_params, char **params) {
	char *c;
    if ( nb_params != 3 ) {
        printf( "Usage: spi_sr_write [reg_number] [address] [data]\n\
       [reg_number] must be 1, 2 or 3\n\
       [address] can be zero to 255\n\
       [data] can be 256 bytes long at maximum" );
        return;
    }

	uint8_t reg  = (uint8_t)strtoul( params[0], &c, 0 );
	uint8_t addr = (uint8_t)strtoul( params[1], &c, 0 );
    uint8_t value = (uint8_t)strtoul(params[2], &c, 0);
    if (*c != 0) {
        char *msg = params[2];
        size_t msglen = strlen( msg );
        printf( "Writing message : %s, length : %d\n", msg, msglen );
		spiWriteSecurityRegister(reg, addr, (uint8_t *)msg, msglen);
    }
    else {
        printf("Writing value : 0x%08x at address 0x%02x\n", value, addr);
		spiWriteSecurityRegister(reg, addr, &value, 1);
    }
}
define_command(spi_sr_write, spi_sr_write_handler, "Write to one of the three SPI security registers", POGO_CMDS);

/*
 * Toggle spiflash memory-mapped/bitbang mode
 */
static void spi_mm_handler(int nb_params, char **params) {
	char *c;
    if (( nb_params > 1  ) | (nb_params == 0) ) {
        printf( "Usage: spi_mm [0|1]\n\
       Enable or disable memory-mapped spiflash\n" );
        return;
    }
    if ( nb_params == 1 ) {
        uint8_t val = (uint8_t)strtoul( params[0], &c, 0 );
        if((val == 0) | (val == 1)) {
            spiflash_bitbang_en_write(!c);
        }
    }
}
define_command(spi_mm, spi_mm_handler, "Enable or disable memory-mapped spiflash", POGO_CMDS);

/*
 * Erase user program in flash (Flag only + start prog)
 */
static void erase_userprog_handler(int nb_params, char **params) {
    
    if ( check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET) || check_flash_state(FLASH_IS_PARTIAL, FLASH_OK_OFFSET)) {
        // Enable memory-mapped mode
        spiFree();
        //erase a part of the "flash is ok" token
        spiBeginErase4(FLASH_OK_OFFSET);
        //erase the begining of the user code
        spiBeginErase64(0x60000);

        printf("User program erased\n");
    }
}
define_command(erase_userprog, erase_userprog_handler, "Erase userprog in flash", POGO_CMDS);

#ifndef REMOCON
/*
 * Reboots to image 0 or 1
 */
static void run_handler(int nb_params, char **params)
{
	char *c;
    if ( nb_params > 1  ) {
        printf( "Usage: run [0|1]\n\
       Reboot to image 0 (bootloader) or 1 (user)\n\
       Default is to reboot to user image\n" );
        return;
    }

    if ( nb_params == 1 ) {
        uint8_t addr = (uint8_t)strtoul( params[0], &c, 0 );
        reboot_ctrl_write(0xac | (addr & 3));    // Reboot FPGA to image 0 (bootloader image)
    }
    else
        if (check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET))
        {
            reboot_ctrl_write(0xac | 1);    // Reboot FPGA to image 1 (user image)
        } else {
            printf( "run user code not possible\n");   
        }
        
}
define_command(run, run_handler, "Reboot to user image or bootloader", POGO_CMDS);
#endif

#if defined CSR_SPI_CS_BASE
/*
 * Start and configure IMU
 */
static void imu_start_handler(int nb_params, char **params) {
    IMU_Init();
}
define_command(imu_start, imu_start_handler, "Start IMU", POGO_CMDS);

/*
 * Put IMU in sleep mode
 */
static void imu_sleep_handler(int nb_params, char **params) {
    IMU_GoToSleep();
}
define_command(imu_sleep, imu_sleep_handler, "Put IMU in sleep mode", POGO_CMDS);


/*
 * Read SPI registers containing core temperature and display it
 * TODO: Fix the ratio to display the real temperature in °C
 */
static void imu_read_temp_handler(int nb_params, char **params) {
    float temp = readTemp();
    printf("Temperature of IMU core : %d°C\n", (int)temp); 
}
define_command(imu_read_temp, imu_read_temp_handler, "Read IMU temperature", POGO_CMDS);

/*
 * Read IMU register containing ID
 */
static void imu_read_id_handler(int nb_params, char **params) {
   printf("ID read : 0x%02x, should be 0x98.\n", IMU_ReadByte(WHO_AM_I)); 
}
define_command(imu_read_id, imu_read_id_handler, "Read IMU ID, should be 0x98", POGO_CMDS);

/*
 * Read a IMU register by address
 */
static void imu_read_handler(int nb_params, char **params) {
	char *c;
    if (( nb_params == 0 ) || ( nb_params > 1 ) ) {
        printf( "Usage: imu_read [address]\n" );
        return;
    }

	uint8_t addr = (uint8_t)strtoul( params[0], &c, 0 );
	printf("Register at address 0x%02x: 0x%02x\n", addr, IMU_ReadByte(addr)); 
}
define_command(imu_read, imu_read_handler, "Read IMU register by address", POGO_CMDS);

/*
* Command "read ACC / Gyro from IMU"
*/
static void imu_read_acc_handler(int nb_params, char **params)
{
    float acc[3] = {0, 0, 0};
    float gyro[3] = {0, 0, 0};
    char *c;
    unsigned int delay_ms;

    if ( ( nb_params == 0 ) || ( nb_params > 1 ) )
    {
        printf( "imu_read_sensor <time>\n" );
        printf("<time> : count delay in ms \n");
        printf(" E.g. : imu_read_sensor 100 \n");

            return;
    }

    delay_ms = (unsigned int)strtoul(params[0], &c, 0);

    printf( "type something to quit\n");
    printf("Acc *1000 m.s-2\n");
    printf("Gyro *100 rads\n");
    printf("delay betwwen each display %d ms\n", delay_ms);
    printf("# Acc X, Acc Y, Acc Z, Gyro X, Gyro Y Gyro Z\n");

    while(uart_read_nonblock() == 0) {
        readAcc(acc);
        readGyro((float*)(&gyro));

	    printf("%d, %d, %d, ", (int)(acc[0]*1000), (int)(acc[1]*1000), (int)(acc[2]*1000)); 
	    printf("%d, %d, %d \n", (int)(gyro[0]*100), (int)(gyro[1]*100), (int)(gyro[2]*100));

        msleep(delay_ms);
    }    
}
define_command(imu_read_sensor, imu_read_acc_handler, "Read IMU sensor", POGO_CMDS);

/*
 * write to an IMU register by address
 */
static void imu_write_handler(int nb_params, char **params) {
	char *c;
    if (( nb_params == 0 ) || ( nb_params > 2 ) ) {
        printf( "Usage: imu_read [address] [data]\n" );
        return;
    }
	uint8_t addr = (uint8_t)strtoul( params[0], &c, 0 );
	uint8_t data = (uint8_t)strtoul( params[1], &c, 0 );
	printf("Write to register 0x%02x, data: 0x%08x\n", addr, data); 
    IMU_WriteByte(addr, data);
}
define_command(imu_write, imu_write_handler, "Write to IMU register", POGO_CMDS);

/*
 * Read ADC 
 */
static void adc_read_handler(int nb_params, char **params) {
    char *c;
    uint32_t result;
    uint32_t channel;
    if (( nb_params == 0 ) || ( nb_params > 1 ) ) {
        printf( "Usage: adc_read [channel]\n\
Channel 0: Back, Channel 1: Front-Left, Channel 2: Front-Right, Channel 3: Battery level\n\
Channel 4 to 7 : not connected on head PCB but available on connector to belly\n");
        for(channel=0; channel<ADC_INPUTS; channel++) {
            result = ADC_Read(channel);
            printf("Value read on channel %ld : 0x%02lx\n", channel, result);
            if(channel == 3) printf("Battery voltage : %ld mV\n", (result*6445)/1000);
        }
        return;
    }
    channel = (uint32_t)strtoul( params[0], &c, 0 );
    if( channel >= ADC_INPUTS  ) {
        printf("Incorrect argument entered\n");
        return;
    }
    result = ADC_Read(channel);
    printf("Value read on channel %ld : 0x%02lx\n", channel, result);
    if(channel == 3) {
        printf("Battery voltage : %ld mV\n", (result*6445)/1000);
        // result * ( VRef/(1024) ) * 2 (VBatt divided by 2) * 1000 (for mV), *1000 to stay in int range
        // VRef = 3.3V
    }
}
define_command(adc_read, adc_read_handler, "Read ADC registers", POGO_CMDS);

/*
 * Read ADC in loop
 */
static void adc_read_loop_handler(int nb_params, char **params) {
    uint32_t result;
    uint32_t channel;

        printf("type something to quit\n");
        printf("# photosensor Back (0), photosensor Front-Left (1), photosensor Front-Right (2), batery level, CH4, CH5, CH6, CH7\n");
       
    while(uart_read_nonblock() == 0) {
        for(channel=0; channel<8; channel++) {
            result = ADC_Read(channel);
            if (channel != 7) {
                printf("0x%02lx,", result);
            } else {
                printf("0x%02lx\n", result);
            }
        }
        msleep(200);
    } 


}
define_command(adc_read_loop, adc_read_loop_handler, "Read ADC registers in loop", POGO_CMDS);



/*
 * display the battery voltage 
 * ( put any parameters to display the debug )
*/
static void battery_reading_handler(int nb_params, char **params) {
    uint32_t result = 0;
    uint32_t bLevel = 0;
    uint8_t iter = 16;

    if(nb_params) {
        printf(" green : bat >= 3.3V \n\
                 yellow : 3.2V <= bat < 3.3V \n\
                 orange : 3.1V <= bar < 3.2V \n\
                 red : bat < 3.1V \n");
    }
    
    // read battery level
    for (int i = 0; i < iter; i++)
    {
        result += ADC_Read(3);
    }
    //divide by 16
    result = result >> 4; 
    
    if (nb_params) printf("Value read for the battery : 0x%02lx\n", result);
    // result * ( VRef/(1024) ) * 2 (VBatt divided by 2) * 1000 (for mV), *1000 to stay in int range
    // VRef = 3.3V
    bLevel = (result*6445)/1000;
    if (nb_params) printf("Battery voltage : %ld mV\n", bLevel);
    
    // LED color
    if ( bLevel >= 3300)
    {
        //green
        rgb_set(0x00, 0x22, 0x00);

    } else if ( bLevel < 3300 || bLevel >= 3200 )
    {
        //yellow
        rgb_set(0x22, 0x22, 0x00);

    } else if ( bLevel < 3200 || bLevel >= 3100 )
    {
        //orange
        rgb_set(0x22, 0x05, 0x00);

    } else {
        //red
        rgb_set(0x22, 0x00, 0x00);
    }

}
define_command(bat_life, battery_reading_handler, "give a color corresponding of the voltage (paramater to activate debug)", POGO_CMDS);

static void voltage_mode_handler(int nb_params, char **params) {
    extern uint8_t voltage_status;
    char *c;

    if ( nb_params != 1) {
        printf( "volt_mode 0|1. (0: OFF, 1: ON) \n");
        return;
    }
    voltage_status = (uint8_t)strtoul(params[0], &c, 0);

    printf(" new voltage mode status(%d)\n", voltage_status);
}
define_command(volt_mode, voltage_mode_handler, "Voltage mode (0: OFF, 1: ON)", POGO_CMDS);

static void autotest_mode_handler(int nb_params, char **params) {
    extern uint8_t autotest_status;
    extern uint8_t autotest_done;

    autotest_status = !autotest_status;
    autotest_done = 0;

    printf(" new autotest mode status(%d)\n", autotest_status);
}
define_command(autotest_mode, autotest_mode_handler, "toggle the autotest mode", POGO_CMDS);

#endif //CSR_SPI_CS_BASE
#ifdef CSR_MOTOR_RIGHT_BASE
/*
 * Set motor's PWM
 */
#define INCREMENT 16
static void motor_handler(int nb_params, char **params) {
    char *c;
    char key=0;
    int32_t pwm_values[3]={0, 0, 0};
    int8_t m_number=0, incr=0;

    printf( "Usage: motor [R|L|B] [value]\n\
             R for right, L for left and B for middle motor\n\
            value is the PWM level between 0 (off) and 1023\n");
    printf(" or motor [R_value] [L_value] [B_value]\n\
                value is the PWM level between 0 (off) and 1023\n");
    printf("if no argument given: interactive mode\n");
    
    if ( nb_params == 0 ) {
        printf("Interactive mode :\n\
                press R or r, L or l, B or b to increment or decrement the PWM value\n\
                press z to zero the values\n\
                press q to quit\n");
        while( (key != 'q') ) {
            if( uart_read_nonblock() != 0) {
                key = uart_read();
                if (key == 'r'){
                    m_number = 0;
                    incr = -INCREMENT;
                }
                if (key == 'R'){
                    m_number = 0;
                    incr = INCREMENT;
                }
                if (key == 'l'){
                    m_number = 1;
                    incr = -INCREMENT;
                }
                if (key == 'L'){
                    m_number = 1;
                    incr = INCREMENT;
                }
                if (key == 'b'){
                    m_number = 2;
                    incr = -INCREMENT;
                }
                if (key == 'B'){
                    m_number = 2;
                    incr = INCREMENT;
                }
                pwm_values[m_number] += incr;
                if(pwm_values[m_number] >= 1024) pwm_values[m_number] = 1023;
                if(pwm_values[m_number] < 0) pwm_values[m_number] = 0;

                if ((key == 'z') || (key == 'Z')) {
                    incr = 0;
                    for (uint8_t i=0; i<3; i++) {
                        pwm_values[i]=0;
                        pogobot_motor_set(i, 0);
                    }
                    printf("Resetting the three PWM values to zero\n");
                }
                else {
                    printf("\
                                     \r\
Setting PWM for motor %d to value %ld\tPress 'q' to quit\r", m_number, pwm_values[m_number]);
                    pogobot_motor_set(m_number, pwm_values[m_number]);
                }
            }
        }
        return;
    } else if (nb_params == 2 ) {
        switch ((int)params[0][0]) {
            case 'R':
            case 'r':
                m_number = 0;
                break;
            case 'L':
            case 'l':
                m_number = 1;
                break;
            case 'B':
            case 'b':
                m_number = 2;
                break;
        }
        pwm_values[0] = (uint32_t)strtoul( params[1], &c, 0 );
        printf("Setting PWM for motor %d to value %ld\n", m_number, pwm_values[0]);
        pogobot_motor_set(m_number, pwm_values[0]);
    } else if ( nb_params == 3) {
        for (size_t i = 0; i < 3; i++)
        {
            pwm_values[i] = (uint32_t)strtoul( params[i], &c, 0 );
            printf("Setting PWM for motor %d to value %ld\n", i, pwm_values[i]);
	        pogobot_motor_set(i, pwm_values[i]);
        }
        
    } 
}

define_command(motor, motor_handler, "Set PWM for motors", POGO_CMDS);

#ifdef CSR_GPIO_BASE
static void motor_dir_status_handler(int nb_params, char **params) {
    uint32_t status = pogobot_motor_dir_current_status();
    //field ( XXXX XBLR )
    int8_t R = status & 0x1;
    int8_t L = (status>>1) & 0x1;
    int8_t B = (status>>2) & 0x1; 
    printf(" motors direction current status <%lx> : R:%d L:%d B:%d \n", status, R, L, B);
}
define_command(motor_dir_current_status, motor_dir_status_handler, "motors direction current status", POGO_CMDS);

static void motor_dir_set_handler(int nb_params, char **params) {

    int32_t dir_value[3]={0, 0, 0};
    int8_t m_number=0;
    char *c;

    if (( nb_params == 0 ) || ( nb_params > 2 ) ) {
        printf( "Usage: motor_dir_set [R|L|B] [value]\n\
                 R for right, L for left and B for middle motor\n\
                 Value is 0 or 1 \n");
        return;
    }

    switch ((int)params[0][0]) {
        case 'R':
        case 'r':
            m_number = 0;
            break;
        case 'L':
        case 'l':
            m_number = 1;
            break;
        case 'B':
        case 'b':
            m_number = 2;
            break;
        default:
            printf("motor unknown\n");
            return;
    }
    dir_value[0] = (uint32_t)strtoul( params[1], &c, 0 );
    printf("Setting direction for motor %d to value %ld\n", m_number, dir_value[0]);
	pogobot_motor_dir_set(m_number, dir_value[0]);

}
define_command(motor_dir_set, motor_dir_set_handler, "motors direction set", POGO_CMDS);

#ifdef CSR_SPI_CS_BASE
static void motor_dir_mem_handler(int nb_params, char **params) {

    uint8_t dir_value[3]={0, 0, 0};
    char *c;

    if ( nb_params != 3 ) {
        printf( "Usage: motor_dir_mem_set [valueR] [valueL] [valueB]\n\
                 R for right, L for left and B for middle motor\n\
                 Value is 0 or 1 \n\
                 e.g. motor_dir_mem 1 0 0 \n");
        return;
    }

    dir_value[0] = (uint8_t)strtoul( params[0], &c, 0 );
    dir_value[1] = (uint8_t)strtoul( params[1], &c, 0 );
    dir_value[2] = (uint8_t)strtoul( params[2], &c, 0 );
	setMotorDirMem(dir_value);

}
define_command(motor_dir_mem_set, motor_dir_mem_handler, "set memorized motors direction", POGO_CMDS);

static void motor_dir_mem_get_handler(int nb_params, char **params) {
    
    uint8_t data[3] = {0};
    getMotorDirMem(data);
    printf(" memorized motors direction: R:%d L:%d B:%d \n", data[0], data[1], data[2]);
}
define_command(motor_dir_mem_get, motor_dir_mem_get_handler, "get memorized motors direction", POGO_CMDS);

static void motor_power_mem_set_handler(int nb_params, char **params) {

    uint16_t p_value[3]={0, 0, 0};
    char *c;

    if ( nb_params != 3 ) {
        printf( "Usage: motor_power_mem_set [valueR] [valueL] [valueB]\n\
                 R for right, L for left and B for back motor\n\
                 Value [0;1023] \n\
                 e.g. motor_power_mem_set 512 650 0\n");
        return;
    }

    p_value[0] = (uint16_t)strtoul( params[0], &c, 0 );
    p_value[1] = (uint16_t)strtoul( params[1], &c, 0 );
    p_value[2] = (uint16_t)strtoul( params[2], &c, 0 );
	setMotorPowerMem(p_value);

}
define_command(motor_power_mem_set, motor_power_mem_set_handler, "memorize motors power", POGO_CMDS);

static void motor_power_mem_get_handler(int nb_params, char **params) {
    uint16_t m_power[3]={0};
    
    getMotorPowerMem(m_power);

    printf(" motors power status {R %d, L %d, B %d} \n", m_power[0],m_power[1],m_power[2]);
}
define_command(motor_power_mem_get, motor_power_mem_get_handler, "motors power status", POGO_CMDS);

#endif


#endif

#endif //CSR_MOTOR_RIGHT_BASE

/*
 * Change color of LED(s)
 */
static void rgb_set_handler(int nb_params, char **params) {
	char *c;
    uint32_t color;
    int8_t id;
    if (( nb_params == 0 ) || ( nb_params > 2 ) ) {
        printf( "Usage: set_led [color in hex] [LED_id]\n\
       no [LED_id] means 0, -1 means all, {0,4}\n\
       color is a 24bit value in Red Green Blue MSB to LSB.\n\
       Example : Set RED color : rgb_set 0xFF0000 0\n\
       You can specify which LED to control as last argument\n\
       LED 0 is the LED on head board\n\
       LED 1 is the LED in front of belly board\n\
       LED 2 is on the right, 3 back, 4 left\n" );
        return;
    }
    if ( nb_params == 2 ) {
        id = (int8_t)strtoul( params[1], &c, 0 );
        if ( id < -1 && id > 4 ) {
            printf("Error : There are only 5 LEDs, index 0 to 4\n");
            return;
        }
    } else {
        id = 0;
    }
    color = (uint32_t)strtoul( params[0], &c, 0 );
    if( color > 0xFFFFFF ) {
        printf("Incorrect argument entered\n");
        return;
    }
#ifdef RGB_LEDS
    if ( id >=0 ) {
        rgb_set_led((color&0xFF0000) >> 16, (color&0xFF00) >> 8, color&0xFF, id);
    }
    else {
        for (size_t i = 0; i < 5; i++)
        {
            rgb_set_led((color&0xFF0000) >> 16, (color&0xFF00) >> 8, color&0xFF, i);
        }
        
    }
#else
    rgb_set((color&0xFF0000) >> 16, (color&0xFF00) >> 8, color&0xFF);
#endif
}
define_command(set_led, rgb_set_handler, "Set RGB LED color", POGO_CMDS);


/*
 * Launch an autotest of the system
 */
#define GREEN_T "\e[92;1m"
#define STOP_T "\e[m"
#define RED_T "\e[91;1m"
#define BOLD_T "\e[1m"

void autotest_leds(void);
void autotest_leds(void) {
#ifdef RGB_LEDS
    printf(BOLD_T"RGB LEDs test\n LED : "STOP_T);
    for (int i = 0; i < RGB_LEDS; i++) {
        printf("%d ", i);
        rgb_set_led(0, 0, 255, i);
        msleep(300);
        rgb_set_led(0, 255, 0, i);
        msleep(300);
        rgb_set_led(255, 0, 0, i);
        msleep(300);
        rgb_set_led(0, 0, 0, i);
    }
    printf("\n");
#else
    printf(BOLD_T"RGB LED test"STOP_T"\n");
    rgb_set(0, 0, 255);
    msleep(300);
    rgb_set(0, 255, 0);
    msleep(300);
    rgb_set(255, 0, 0);
    msleep(300);
    rgb_set(0, 0, 0);
#endif
}

void autotest_imu(void);
void autotest_imu(void) {
#ifdef CSR_SPI_CS_BASE
    printf(BOLD_T"IMU test... "STOP_T);
    float acc[3] = {0, 0, 0};
    readAcc(acc);
    //Acc X -177 Y -38  Z -9558 
    //Gyro X 4 Y -2  Z 0
    if (acc[2] < -8.0 && acc[2] > -10.0) {
        printf(GREEN_T"OK\n"STOP_T);
        rgb_set(50, 0, 50);
    } else {
        printf(RED_T"NOK"STOP_T", Z axis accel : %d\n", (int)(acc[2]*1000));
        rgb_set(50, 0, 0);
    }
    //printf("Value : x:%f, y:%f, z:%f\n", acc[0], acc[1], acc[2]);
#endif
}

void autotest_motors(void);
void autotest_motors(void) {
#ifdef CSR_MOTOR_RIGHT_BASE
    printf(BOLD_T "Motor test\n"STOP_T);
    printf(" Test right motor...");
    motor_right_width_write(1023);
    rgb_set_led(0, 0, 255, 2);
    msleep(1000);
    rgb_set_led(0, 0, 0, 2);
    motor_right_width_write(0);
    printf("\n Test left motor...");
    motor_left_width_write(1023);
    rgb_set_led(0, 0, 255, 5);
    msleep(1000);
    rgb_set_led(0, 0, 0, 5);
    motor_left_width_write(0);
    printf("\n Test middle motor...");
    motor_middle_width_write(1023);
    rgb_set_led(0, 0, 255, 3);
    msleep(1000);
    rgb_set_led(0, 0, 0, 3);
    motor_middle_width_write(0);
    printf("\n");
#endif
}

void autotest_adc(void);
void autotest_adc(void) {
#ifdef CSR_SPI_CS_BASE
    printf(BOLD_T"ADC test... "STOP_T);
    uint32_t result;
    result = ADC_Read(3);
    result = (result*6445)/1000;
    printf(" Battery voltage : %ld mV\n ADC test ", result);
    if( (result < 2000) || ( result > 3700 )) {
        printf(RED_T"NOK\n"STOP_T);
    }
    else {
        printf(GREEN_T"OK\n"STOP_T);
        printf(" Visible light sensor, right : %ld, left : %ld, rear : %ld\n", ADC_Read(1), ADC_Read(2), ADC_Read(0));
    }
#endif
}

void autotest_ir(void);
void autotest_ir(void) {
#ifdef CSR_IR_RX0_BASE
    printf(BOLD_T"Infrared test\n"STOP_T);
    ir_init(); //TODO remove if not needed
    // remove echo canceling
    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ ) {
        uint32_t value = IRn_rx_conf_read( ir_i );
        value = ir_rx0_conf_echo_cancel_replace( value, 0 );
        IRn_rx_conf_write( ir_i, value );
    }
    uint8_t result_ir = 0;
    // send and receive on each IR
    for (int i = 0; i < IR_RX_COUNT; i++) {
        // Send at minimal power
        IRn_conf_tx_power_write(i, 1);

        // Purge the buffer first
        while( ir_uart_read_nonblock(i) != 0 ) {
            ir_uart_read(i); 
        }

        char *msg = "DEADBEEF";
		size_t msglen = strlen( msg );
		printf( " Sending message : %s, length : %d on IR %d\n Received : ", msg, msglen, i );
        uint8_t target_ir_bit_mask = 1<<i;
		IRn_tx_write_msg(target_ir_bit_mask, (ir_uart_word_t *)msg, msglen);
        msleep(5);

        // Receive
        uint8_t count = 0, ok = 1;
        if(ir_uart_read_nonblock(i) != 0) {
            count = 0;
            while(ir_uart_read_nonblock(i) != 0) {
                char c = ir_uart_read(i);
                printf("%c ; ", isprint(c)?c:'_');
                if( msg[count++] != c ) {
                    ok = 0;
                }
            }
        }
        printf("\n IR %d : ", i);
        if( (ok == 1) && (count == msglen) ) {
            result_ir++;
            printf(GREEN_T"OK\n"STOP_T);
        }
        else {
            printf(RED_T"NOK\n"STOP_T);
        }
    }

    if (result_ir == IR_RX_COUNT) {
        rgb_set(0,55,0);
        printf (" Infrared test "GREEN_T"OK\n"STOP_T);
    }
    else {
        printf (" Infrared test "RED_T"NOK\n"STOP_T);
    }
    
    // put echo_canceling back
    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ ) {
        uint32_t value = IRn_rx_conf_read( ir_i );
        value = ir_rx0_conf_echo_cancel_replace( value, 1 );
        IRn_rx_conf_write( ir_i, value );
    }
#endif
}

static void autotest_handler(int nb_params, char **params) {
    if ( nb_params == 0 ) {
        printf("Usage : autotest [autotest_name]\n  autotest_name can be : led, imu, motors, ir\n");
        autotest_leds();
        autotest_imu();
        autotest_adc();
        autotest_motors();
        autotest_ir();
    }
    if ( nb_params == 1 ) {
        if (strcmp(params[0], "led") == 0) {
            autotest_leds();
        } else if (strcmp(params[0], "imu") == 0) {
            autotest_imu();
        } else if (strcmp(params[0], "adc") == 0) {
            autotest_adc();
        } else if (strcmp(params[0], "motors") == 0) {
            autotest_motors();
        } else if (strcmp(params[0], "ir") == 0) {
            autotest_ir();
        } else {
            printf("Usage : autotest [autotest_name]\n  autotest_name can be : led, imu, motors, ir\n");
        }
    }
}
define_command(autotest, autotest_handler, "Launch Auto Test", POGO_CMDS);

