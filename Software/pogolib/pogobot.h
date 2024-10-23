/*
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
 */


#ifndef __POGOBOT_H__
#define __POGOBOT_H__

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint8_t and friends */
#include <stdio.h> 
#include <stdlib.h> 
#include <console.h> 
#include <string.h> 
#include <uart.h> 
#include <system.h> 
#include <irq.h> 
#include <time.h> 
#include <stdbool.h>

#include <generated/csr.h> 
#include <generated/mem.h> 
#include <generated/git.h> 

#include "ts4231.h" 
#include "ir_uart.h" 
#include "imu.h" 
#include "spi.h" 
#include "pogobot_ir.h"
#include "rgb.h"
#include "FifoBuffer.h"
#include "sleep.h"
#include "adc.h"
#include "slip.h"
#include "release.h"


/* clang-format-ok */

/* API part */

/**
 * # POGODOCS 
 *
 * This document presents the simple API available to control the differents functions of the robot
 */

/**
 * ## Global API
 */

/** (pogobot_init)
 * Global API Initialisation
 * This function is mandatory inside your program
 * 
 * # Parameters
 * - none
 * 
 * # Return
 * - none
 * 
 */
void pogobot_init( void );


/** 
 * ## Infrared communication API Values
 */

/** 
 * ### Maximum size of a payload in a IR message 
 * MAX_PAYLOAD_SIZE_BYTES 384
 */

#ifndef MAX_PAYLOAD_SIZE_BYTES
#define MAX_PAYLOAD_SIZE_BYTES 382
#endif /* MAX_PAYLOAD_SIZE_BYTES */

/** 
 * ### Maximum number of word from IR recovers in one step 
 * MAX_NUMBER_OF_WORD 20
 */

#ifndef MAX_NUMBER_OF_WORD
#define MAX_NUMBER_OF_WORD 20
#endif /* MAX_NUMBER_OF_WORD */

/**
 * ### IR power level values:
 *
 * - pogobot_infrared_emitter_power_max      - 3
 * - pogobot_infrared_emitter_power_twoThird - 2
 * - pogobot_infrared_emitter_power_oneThird - 1
 * - pogobot_infrared_emitter_power_null     - 0
 */

#define pogobot_infrared_emitter_power_max 3
#define pogobot_infrared_emitter_power_twoThird 2
#define pogobot_infrared_emitter_power_oneThird 1
#define pogobot_infrared_emitter_power_null 0

#define pogobot_infrared_emitter_width_bits 2

#define pogobot_infrared_emitting_power_list( front, right, back, left )       \
    ( front ) | ( ( right ) << ( pogobot_infrared_emitter_width_bits ) ) |     \
        ( ( back ) << ( 2 * pogobot_infrared_emitter_width_bits ) ) |          \
        ( ( left ) << ( 3 * pogobot_infrared_emitter_width_bits ) )

enum
{
    pogobot_infrared_emitting_power_list_front_full =
        ( pogobot_infrared_emitter_power_max ),
    pogobot_infrared_emitting_power_list_right_full =
        ( ( pogobot_infrared_emitter_power_max )
          << ( pogobot_infrared_emitter_width_bits ) ),
    pogobot_infrared_emitting_power_list_back_full =
        ( ( pogobot_infrared_emitter_power_max )
          << ( 2 * pogobot_infrared_emitter_width_bits ) ),
    pogobot_infrared_emitting_power_list_left_full =
        ( ( pogobot_infrared_emitter_power_max )
          << ( 3 * pogobot_infrared_emitter_width_bits ) ),
    pogobot_infrared_emitting_power_list_all_full =
        ( pogobot_infrared_emitter_power_max *
          ( 1 | ( 1 << pogobot_infrared_emitter_width_bits ) |
            ( 1 << ( 2 * pogobot_infrared_emitter_width_bits ) ) |
            ( 1 << ( 3 * pogobot_infrared_emitter_width_bits ) ) ) ),
};

/*static inline uint8_t
pogobot_infrared_emitter_byindex(uint8_t index)
{
    return (1<<index)
};*/

/* Private, implementation part */

#define pogobot_infrared_emitter_front_mask ( 1 << 0 )
#define pogobot_infrared_emitter_right_mask ( 1 << 8 )
#define pogobot_infrared_emitter_back_mask  ( 1 << 16 )
#define pogobot_infrared_emitter_left_mask  ( 1 << 24 )

#define pogobot_infrared_emitter_mask_byindex( index ) ( 1 << ( 8 * index ) )

typedef union emitting_power_list_t
{
    uint32_t number;
    struct
    {
        uint8_t front;
        uint8_t left;
        uint8_t back;
        uint8_t right;
    } fields;
    uint8_t array[4];
} emitting_power_list_t;

/**
 * ### IR direction id list
 *
 * - ir_front - 0
 * - ir_right - 1
 * - ir_back  - 2 
 * - ir_left  - 3
 * - ir_all   - 4
 */

typedef enum
{
    ir_front = 0,
    ir_right = 1,
    ir_back = 2,
    ir_left = 3,
    ir_all = 4
} ir_direction;

/* ******************************** ******************************** */

/**
 * ### IR message header structure
 * 
 * message_header_t :
 *
 * The parameters that begin with "_" are not completed by the user
 *
 * - uint8_t _packet_type         - allows to give type of a packet (fixed for now)
 * - uint8_t _emitting_power_list - used to define the emission power on each IR
 * - uint16_t _sender_id          - id of the robot which send the message 
 * - uint8_t _sender_ir_index     - IR id direction of the sender robot  
 * - uint8_t _receiver_ir_index   - IR id direction that received the message
 * - uint16_t payload_length      - size of the payload
 *
 */

typedef struct message_header_t
{
    uint8_t _packet_type;
    uint8_t _emitting_power_list;
    uint16_t _sender_id;
    uint8_t _sender_ir_index;
    uint8_t _receiver_ir_index;
    uint16_t payload_length;
} message_header_t;

/**
 * ### IR message header structure
 *  
 * message_t :
 *
 * - message_header_t header                  - header of the message
 * - uint8_t payload[MAX_PAYLOAD_SIZE_BYTES]  - payload of the message
 */

typedef struct message_t
{
    message_header_t header;
    uint8_t payload[MAX_PAYLOAD_SIZE_BYTES];
} message_t;

/**
 * ### IR short message header structure
 * 
 * message_short_header_t :
 *
 * The parameters that begin with "_" are not completed by the user
 *
 * - uint8_t _packet_type         - allows to give type of a packet (fixed for now)
 * - uint16_t payload_length      - size of the payload
 *
 */

typedef struct message_short_header_t
{
    uint8_t _packet_type;
    uint16_t payload_length;
} message_short_header_t;

/**
 * ### IR short message header structure
 *  
 * short_message_t :
 *
 * - message_short_header_t header            - header of the message
 * - uint8_t payload[MAX_PAYLOAD_SIZE_BYTES]  - payload of the message
 */

typedef struct short_message_t
{
    message_short_header_t header;
    uint8_t payload[MAX_PAYLOAD_SIZE_BYTES];
} short_message_t;

/**
 * ### IR type message list
 *
 * - ir_t_cmd  : type use to send command to the robot
 * - ir_t_flash : type use to send part of a program
 * - ir_t_short : type use to send short message between robot in user space
 * - ir_t_user : type use to send message between robot in user space
 */

typedef enum
{
    ir_t_cmd = 1,   // only to send command to the robot
    ir_t_flash = 2, // only to send part of the prog
    ir_t_short = 3, // only to send short header message
    ir_t_user = 16  // userspace
} ir_type_message;


/* ******************************** ******************************** */

/* fifo */

#define NUMEL 20
FifoBuffer_typedef( message_t, Messagefifo );

extern Messagefifo my_mes_fifo;
extern Messagefifo *my_mes_fifo_p;

/* ******************************** ******************************** */

/** 
 * ## Infrared communication API Functions
 */

/** (pogobot_infrared_ll_init)
 * Initialise Infrared hardware and software struture
 * (already made inside pogobot_init)
 * 
 * # Parameters
 * - none
 * 
 * # Return
 * - none
 *
 */
void pogobot_infrared_ll_init( void );

/** (pogobot_infrared_update)
 * Infrared checks for received data and send to decode messages
 * Decoded messages are placed in a Fifo
 * 
 * # Parameters
 * - none
 *
 * # Return
 * - none
 */
void pogobot_infrared_update( void );

/** (pogobot_infrared_message_available)
 * Infrared new message checks fonction
 *
 * # Parameters
 * - none
 *
 * # Return
 * - none
 *
 */
int pogobot_infrared_message_available( void );

/** (pogobot_infrared_recover_next_message)
 * Recover the next message inside the message queue
 *
 * # Parameters
 * - 'mes' - Allocated structure of type 'message_t'
 *
 * # Return
 * - none
 *
 */
void pogobot_infrared_recover_next_message( message_t *mes );

/** (pogobot_infrared_clear_message_queue)
 * Clears Infrared message queue
 *
 * # Parameters
 * - none
 *
 * # Return
 * - none
 *
 */
void pogobot_infrared_clear_message_queue( void );

/** (pogobot_infrared_set_power)
 * set the power level used to send all the next messages
 *
 * # Parameters
 * - 'power' - use the pogobot_infrared_emitter_power_* or the values {0,1,2,3}
 *
 * # Return
 * - none
 */
void pogobot_infrared_set_power( uint8_t power );

/** (pogobot_infrared_sendRawLongMessage)
 *  Prepare and send one packet, with the specified emitters and
 *  powers, to the recipient, containing the specified message.
 *
 * # Parameters
 * - 'message' - fully filled message_t variable
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 *
 */
uint32_t pogobot_infrared_sendRawLongMessage( message_t *const message );

/** (pogobot_infrared_sendRawShortMessage)
 *  Prepare and send one packet, with a short header
 *  containing the specified message.
 *
 * # Parameters
 * - 'dir' - indicates the direction to send the message
 * - 'message' - fully filled short_message_t variable
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 *
 */
uint32_t pogobot_infrared_sendRawShortMessage( ir_direction dir, short_message_t *const message );

/** (pogobot_infrared_sendLongMessage_uniSpe)
 * Send a message in only direction at defined power
 * Use pogobot_infrared_sendRawLongMessage
 *
 * # Parameters
 * - 'dir' - indicates the direction to send the message
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 */
uint32_t pogobot_infrared_sendLongMessage_uniSpe( ir_direction dir, uint8_t *message, uint16_t message_size );

/** (pogobot_infrared_sendLongMessage_omniGen)
 * Send the same message in all direction at defined power
 * Use pogobot_infrared_sendRawLongMessage
 * Their no infrared sender ID
 *
 * # Parameters
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 */
uint32_t pogobot_infrared_sendLongMessage_omniGen( uint8_t *message, uint16_t message_size );

/** (pogobot_infrared_sendLongMessage_omniSpe)
 * Send successively the same message with the origin infrared ID on each Infrared
 * It is 4 times slower that without ID
 * Use pogobot_infrared_sendRawLongMessage
 *
 * # Parameters
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 *
 */
uint32_t pogobot_infrared_sendLongMessage_omniSpe( uint8_t *message, uint16_t message_size );


/** (pogobot_infrared_sendShortMessage_uni)
 * Send a short header message in only direction at defined power
 * Use pogobot_infrared_sendRawShortMessage
 *
 * # Parameters
 * - 'dir' - indicates the direction to send the message
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 */
uint32_t pogobot_infrared_sendShortMessage_uni( ir_direction dir, uint8_t *message, uint16_t message_size );

/** (pogobot_infrared_sendShortMessage_omni)
 * Send a short header message in all direction at defined power
 * Use pogobot_infrared_sendRawShortMessage
 *
 * # Parameters
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 */
uint32_t pogobot_infrared_sendShortMessage_omni( uint8_t *message, uint16_t message_size );


/** (pogobot_infrared_get_receiver_error_counter)
 * Get the receiver error counter value
 *
 * # Parameters
 * - 'error_counter' - allocated structure 'slip_error_counter_s'
 * - 'ir_index' - index of the ir receiver (0 to 3)
 *
 * # Return
 * - none
 */
void pogobot_infrared_get_receiver_error_counter( slip_error_counter_s *error_counter, uint8_t ir_index );

/** (pogobot_infrared_reset_receiver_error_counter)
 * Reset all reveiver error counter
 *
 * # Parameters
 * - none
 *
 * # Return
 * - none
 *
 */
void pogobot_infrared_reset_receiver_error_counter( void );

/** 
 * ## RGB LED API
 */

/** (pogobot_led_setColor)
 * Set the value of red, green and blue of the head led in static mode
 * each value goes from 0 to 255 to determine the intensity.
 *
 * # Parameters
 * - 'r' - value of the RED part
 * - 'g' - value of the GREEN part
 * - 'b' - value of the BLUE part
 *
 * # Return
 * - none
 *
 */
void pogobot_led_setColor( const uint8_t r, const uint8_t g, const uint8_t b );

#ifdef RGB_LEDS
/** (pogobot_led_setColors)
 * Set the value of red, green and blue of the led with the number (ID) in static mode
 * each value goes from 0 to 255 to determine the intensity.
 * (only available with a belly with multiple LEDs)
 *
 * # Parameters
 * - 'r' - value of the RED part
 * - 'g' - value of the GREEN part
 * - 'b' - value of the BLUE part
 * - 'id' - Led ID (0 = head, 1 = belly front, 2 = belly right, 3 = belly back, 4 = belly left)
 *
 * # Return
 * - none
 *
 */
void pogobot_led_setColors( const uint8_t r, const uint8_t g, const uint8_t b, uint8_t id );
#endif

/** 
 * ## Photosensors API Values
 */

/**
 * ### Photosensor id definition
 * - 0 is the back sensor
 * - 1 is the front-left sensor
 * - 2 is the front-right sensor
 */

typedef enum
{
    p_FL = 1,
    p_FR = 2,
    p_B = 0
} photosensor_id;

/** 
 * ## Photosensors API Functions
 */

/** (pogobot_photosensors_read)
 * Read one ambient light sensor.
 * Sensor number must be between 0 and 2.
 *
 * # Parameters
 * - 'sensor_number' - id of the sensor (Photosensor definition)
 *
 * # Return
 * Return a value proportional to the light
 */
int16_t pogobot_photosensors_read( uint8_t sensor_number );

/**
 * ## IMU API 
 */

/** (pogobot_imu_read)
 * Read the accelaration on the IMU. <br>
 * Returns in acc and gyro the IMU values. 
 *
 * usage : <br>
 * float acc[3], gyro[3];
 *
 * index : 
 *
 * - 0 - X axis
 * - 1 - Y axis
 * - 2 - Z axis
 * 
 * # Parameters
 * - 'acc' - allocated float table (size 3)
 * - 'gyro' - allocated float table (size 3)
 *
 * # Return
 * - none
 */
void pogobot_imu_read( float *acc, float *gyro );

/** (pogobot_imu_readTemp)
 * Read the temparature sensor on the IMU.
 * 
 * # Parameters
 * - none 
 *
 * # Return
 * Returns the temperature in degres celsius
 */
float pogobot_imu_readTemp( void );


/**
 * ## Battery API 
 */

/** (pogobot_battery_voltage_read)
 * Recovers the value of the battery in mV
 *
 * # Parameters
 * - none
 *
 * # Return
 * Returns a value in mV
 */
int16_t pogobot_battery_voltage_read( void );

/**
 * ## Motors API Values
 */

/**
 * ### Motor id definition :
 *
 * - motorR  - 0
 * - motorL  - 1
 * - motorB  - 2
 */

typedef enum
{
    motorR = 0, // Right
    motorL = 1, // Left
    motorB = 2  // Back
} motor_id;

/**
 * ### Motor range :
 *
 * - motorStop                   - 0
 * - motorQuarter                - 256
 * - motorHalfmotorThreeQuarter  - 716
 * - motorFull                   - 1023
 */

typedef enum
{
    motorStop = 0,
    motorQuarter = 256,
    motorHalf = 512,
    motorThreeQuarter = 716,
    motorFull = 1023
} motor_range;

/**
 * ## Motors API Functions
 */

/** (pogobot_motor_power_set)
 * set the value of pwm that commands the motor (active)
 * 
 * # Parameters
 * - 'motor' - is the id of the motor you want to command
 * - 'value' - is PWM ratio apply to the motor (between 0 and 1023)
 *
 * # Return
 * - none
 *
 */
void pogobot_motor_power_set( motor_id motor, uint16_t value );
void pogobot_motor_set ( motor_id motor, uint16_t value );

/** (pogobot_motor_dir_current_status)
 * recover the value of the motor direction bit field.
 * 
 * # Parameters
 * - none
 *
 * # Return
 * - bit field ( XXXX XMLR )
 *
 */
uint32_t pogobot_motor_dir_current_status( void );

/** (pogobot_motor_dir_mem_get)
 * get the value of pwm that commands the motor.
 * 
 * # Parameters
 * - 'p_directions' - is an array of size 3 [R, L, B].
 *            Each value is the chosen direction (0 or 1)
 *
 * # Return
 * - the success or not of the read in memory (0: Ok, -1: NOk)
 *
 */
int8_t pogobot_motor_dir_mem_get( uint8_t *p_directions );

/** (pogobot_motor_dir_mem_set)
 * set the value of pwm that commands the motor (persistent).
 * 
 * # Parameters
  * - 'p_directions' - is an array of size 3 [R, L, B].
 *            Each value is the chosen direction (0 or 1)
 *
 * # Return
 * - the success or not of the read in memory (0: Ok, -1: NOk)
 *
 */
int8_t pogobot_motor_dir_mem_set( uint8_t *p_directions);

/** (pogobot_motor_dir_set)
 * set the value of pwm that commands the motor (active).
 * 
 * # Parameters
 * - 'motor' - is the id of the motor you want to command (motorR, motorL, motorB)
 * - 'value' - is the choosen direction (0 or 1)
 *
 * # Return
 * - none
 *
 */
void pogobot_motor_dir_set( motor_id motor, uint8_t value );

/** (pogobot_motor_power_mem_get)
 * recover the value of the motor power memorized.
 * 
 * # Parameters
 * - 'p_powers' - is an array of size 3 [R, L, B].
 *            Each value is the chosen power [0, 1023]
 *
 * # Return
 * - the success or not of the read in memory (0: Ok, -1: NOK)
 *
 */
uint8_t pogobot_motor_power_mem_get( uint16_t *p_powers );

/** (pogobot_motor_power_mem_set)
 * write the value of the motor power in memory (persistent).
 * 
 * # Parameters
 * - 'p_powers' - is an array of size 3 [R, L, B].
 *            Each value is the chosen power [0, 1023]
 *
 * # Return
 * - the success or not of the read in memory (0: Ok, -1: NOK)
 *
 */
uint8_t pogobot_motor_power_mem_set( uint16_t *p_powers );

/**
 * ## Helper API
 */

/** (pogobot_helper_getid)
 * gives an unique identifier or an random number if no id is register inside the robot
 *
 * # Parameters
 * - none
 *
 * # Return
 * Returns an unique id on 16bits
 *
 */
uint16_t pogobot_helper_getid( void );

/** (pogobot_helper_getRandSeed)
 * gives an seed base on the ADC read of the battery 
 *
 * # Parameters
 * - none
 *
 * # Return
 * Returns an random id on 16bits
 *
 */
int16_t pogobot_helper_getRandSeed( void );

/** (pogobot_helper_print_version)
 * print the version inside the prompt
 * It is also possible to use the define RELEASE_VERSION in release.h
 *
 * # Parameters
 * - none
 *
 * # Return
 * - none
 *
 */
void pogobot_helper_print_version( void );

/**
 * ## Time API
 */


/** This is a simple timer implementation based on the available
 * hardware timer.
 *
 * The benefit is that the only hardware timer we have is configured
 * only once, and all software that depend on it just works without
 * conflict.
 *
 * The drawback is that we can't use the interrupt of the hardware
 * timer, if available.
 *
 * To keep the implementation simple, we only rely on the 32bit timer0
 * counter.  This limit the timespan to 2^31/CLOCKRATE, which is 107
 * seconds with a 20MHz system clock.  We can extent the
 * implementation by maintaining a rollover count in some function
 * that we promise to call often enough, add a rollover count in
 * time_reference_t, and extend computations accordingly.
 *
 * Sample code:
 *
 *
 * One time interval, measuring duration, think "get elapsed microseconds".
 *
 * time_reference_t mystopwatch;
 * pogobot_stopwatch_reset( &mystopwatch );
 * my long_computation( arguments );
 * uint32_t microseconds =
 *     pogobot_stopwatch_get_elapsed_microseconds( &mystopwatch );
 * printf( "Duration: %u microseconds", milliseconds );
 *
 *
 * Series of events, measuring duration of each, think "stopwatch lap".
 *
 * time_reference_t mystopwatch;
 * pogobot_stopwatch_reset( &mystopwatch );
 * while ( 1 )
 * {
 *     wait_for_some_external_event();
 *     uint32_t microseconds = pogobot_stopwatch_lap( &mystopwatch );
 *     printf( "Time since previous event: %u microseconds", milliseconds );
 * }
 *
 *
 * You want your code to periodically do something.
 *
 * time_reference_t mytimer;
 * uint32_t period_microseconds = 250000;
 * pogobot_timer_init( &mytimer, period_microseconds );
 * while (1)
 * {
 *    wait_for_some_external_event();
 *    if (pogobot_timer_has_expired())
 *    {
 *        pogobot_timer_offset_origin_microseconds( &mytimer,
 *                                                  period_microseconds );
 *        react_do_something();
 *    }
 * }
 *
 *
 */

/** (pli_timer_sleep_stopwatch_init)
 * Initialise the timer structure
 * (already made inside pogobot_init)
 * 
 * # Parameters
 * - none
 * 
 * # Return
 * - none
 *
 */
void pli_timer_sleep_stopwatch_init( void );

/**
 * ### Timer structure
 * 
 * time_reference_t :
 *
 * - uint32_t hardware_value_at_time_origin - timer reference
 * 
 *
 */
typedef struct time_reference_t
{
    uint32_t hardware_value_at_time_origin;
} time_reference_t;


/** (pogobot_stopwatch_reset)
 * reset a time_reference structure. 
 * To use a time_reference_t as a stopwatch you must reset it using pogobot_stopwatch_reset()
 *
 * # Parameters
 * - 'stopwatch' - pointer to a time_reference_t structure
 *
 * # Return
 * - none
 *
 */
void pogobot_stopwatch_reset( time_reference_t *stopwatch );

/** (pogobot_stopwatch_lap)
 * measures time elapsed from origin and offsets so that origin is zero at this point in time.  
 *
 * # Parameters
 * - 'stopwatch' - pointer to a time_reference_t structure
 *
 * # Return
 * Returns the number of microseconds elapsed on 32 bits (which may be negative if you offset the origin to the future)
 *
 */
int32_t pogobot_stopwatch_lap( time_reference_t *stopwatch );

/** (pogobot_stopwatch_get_elapsed_microseconds)
 * provides the current number of elapsed microseconds without otherwise interfering with the stopwatch state.
 *
 * # Parameters
 * - 'stopwatch' - pointer to a time_reference_t structure
 *
 * # Return
 * Returns the number of microseconds elapsed on 32 bits (which may be negative if you offset the origin to the future)
 *
 */
int32_t pogobot_stopwatch_get_elapsed_microseconds( time_reference_t *stopwatch );

/** (pogobot_stopwatch_offset_origin_microseconds)
 * offsets the origin of the stopwatch by the specified number of microseconds.
 *
 * # Parameters
 * - 'stopwatch' - pointer to a time_reference_t structure
 * - 'microseconds_offset' - number of microsenconds to offset the origin
 *
 * # Return
 * - none
 *
 */
void pogobot_stopwatch_offset_origin_microseconds( time_reference_t *stopwatch, int32_t microseconds_offset );

/** (pogobot_timer_init)
 * set a timer that will expire in the defined number of microseconds in the future.
 *
 * # Parameters
 * - 'timer' - pointer to a time_reference_t structure
 * - 'microseconds_to_go' - number of microsenconds to go
 *
 * # Return
 * - none
 *
 */
void pogobot_timer_init( time_reference_t *timer, int32_t microseconds_to_go );

/** (pogobot_timer_get_remaining_microseconds)
 * provides the current of microseconds until the timer has expired, without otherwise interfering with the timer state.
 *
 * # Parameters
 * - 'timer' - pointer to a time_reference_t structure
 *
 * # Return
 * Returns the number of microseconds elapsed on 32 bits. The result is a signed number, positive when the timer has not expired yet, negative when the timer has expired.
 *
 */
int32_t pogobot_timer_get_remaining_microseconds( time_reference_t *timer );

/** (pogobot_timer_has_expired)
 * Returns true when the timer has expired, false when the timer has not expired yet.
 *
 * # Parameters
 * - 'timer' - pointer to a time_reference_t structure
 *
 * # Return
 * Returns a bool depending on the status (True: if expired, False: if not expired)
 * 
 */
bool pogobot_timer_has_expired( time_reference_t *timer );

/** (pogobot_timer_wait_for_expiry)
 * waits until the timer has expired.
 *
 * # Parameters
 * - 'timer' - pointer to a time_reference_t structure
 *
 * # Return
 * - none
 *
 */
void pogobot_timer_wait_for_expiry( time_reference_t *timer );

/** (pogobot_timer_offset_origin_microseconds)
 * offsets the origin of the timer by the specified number of microseconds.
 *
 * # Parameters
 * - 'timer' - pointer to a time_reference_t structure
 * - 'microseconds_offset' - number of microsenconds to offset the origin
 *
 * # Return
 * - none
 *
 */
void pogobot_timer_offset_origin_microseconds( time_reference_t *timer, int32_t microseconds_offset );


#endif /* __POGOBOT_H__ */
