/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include <stdint.h>
#include <stdlib.h>

#include "spi.h"
#include "pogobot.h"
#include "ir_uart.h"

/* clang-format-ok */

/* Global Init */

void isr( void );
#ifdef CONFIG_CPU_HAS_INTERRUPT
void isr( void )
{
    __attribute__((unused)) unsigned int irqs;

    irqs = irq_pending() & irq_getmask();

    if(irqs & (1 << UART_INTERRUPT))
    {
        uart_isr();
    }
#ifdef CSR_IR_RX0_BASE
    if(irqs & ((1 << IR_RX0_INTERRUPT) | (1 << IR_RX1_INTERRUPT) | (1 << IR_RX2_INTERRUPT) | (1 << IR_RX3_INTERRUPT) ))
        ir_uart_rx_isr();
#endif //CSR_IR_RX0_BASE
#ifdef CSR_IR_TX_BASE
//    if(irqs & (1 << IR_TX_INTERRUPT))
//        ir_uart_tx_isr();
#endif //CSR_IR_TX_BASE
}
#else
void isr( void ){};
#endif //CONFIG_CPU_HAS_INTERRUPT



void
pogobot_init( void )
{
#ifdef CONFIG_CPU_HAS_INTERRUPT
    irq_setmask(0);
    irq_setie(1);
#endif
    // Necessary for sleep()/msleep()/usleep() to work.
    pli_timer_sleep_stopwatch_init();
#ifdef CSR_UART_BASE
    uart_init();
#endif
    spiInit();
    //recover motor direction 
    uint8_t data[3] = {0};
    getMotorDirMem(data);
    //set the motor direction
    for (size_t i = 0; i < 3; i++)
    {
        pogobot_motor_dir_set(i, data[i]);
    }
     
#ifdef CSR_IR_TX_BASE
    ir_uart_init();
    ir_init(); // Uses usleep()
#endif
#ifdef CSR_SPI_CS_BASE
    IMU_Init();
#endif

    rgb_init(); // Clear the LEDs

    pogobot_infrared_ll_init();
}

void
pogobot_led_setColor( const uint8_t r, const uint8_t g, const uint8_t b )
{
    rgb_set( r, g, b);
}

#ifdef RGB_LEDS
void
pogobot_led_setColors( const uint8_t r, const uint8_t g, const uint8_t b, uint8_t id)
{
    rgb_set_led( r, g, b, id);
}
#endif

/* sensors */

/* photosensors */
int16_t
pogobot_photosensors_read( uint8_t sensor_number )
{
    if( ( sensor_number >= 0 ) && ( sensor_number <=2 ) ) {
        return ADC_Read( sensor_number );
    }
    else
        return -1;
}

/* imu */
void 
pogobot_imu_read( float *acc, float *gyro )
{
    readAcc(acc);
    readGyro(gyro);
}

float 
pogobot_imu_readTemp( void )
{
    return readTemp();
}

/* battery */
int16_t
pogobot_battery_voltage_read( void )
{
    uint32_t result;
    result = ADC_Read(3);
    return (result*6445)/1000;
}

/* motors */

void
pogobot_motor_set( motor_id motor, uint16_t value )
{
    if ( value > motorFull)
        value = motorFull;

    switch(motor)
    {
		case motorR:
#ifdef CSR_MOTOR_RIGHT_BASE
            motor_right_width_write(value);
#endif
        break;
        case motorL:
#ifdef CSR_MOTOR_LEFT_BASE
            motor_left_width_write(value);
#endif
        break;

        case motorB:
#ifdef CSR_MOTOR_MIDDLE_BASE
            motor_middle_width_write(value);
#endif
		break;
	}
}

uint32_t 
pogobot_motor_dir_status( void )
{
    uint32_t value = 0xFFFFFFFF;
#ifdef CSR_GPIO_BASE
    value = gpio_gpo_read();
#endif
    return value;
}

void 
pogobot_motor_dir_set( motor_id motor, uint16_t value )
{
    switch(motor)
    {
#ifdef CSR_GPIO_BASE
		case motorR:
        gpio_gpo_right_motor_dir_write(value);
        break;

        case motorL:
        gpio_gpo_left_motor_dir_write(value);
        break;

        case motorB:
        gpio_gpo_middle_motor_dir_write(value);
		break;
#endif
        default:
        break;
    }

}

/* helper */

int16_t
pogobot_helper_getRandSeed( void )
{
    return (int16_t)ADC_Read( 3 );
}

uint16_t
pogobot_helper_getid( void )
{
    extern uint64_t unique_id, serial_number;
    if( unique_id != 0 ) return unique_id;
    if( serial_number != 0 ) return serial_number;

    uint16_t seed = (int16_t)ADC_Read( 3 );
    //printf( "FIXME: seed -> %u\n", seed );
    srand( seed );
    return rand();
}
