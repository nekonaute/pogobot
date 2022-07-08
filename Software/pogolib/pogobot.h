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

#include <generated/csr.h> 
#include <generated/mem.h> 
#include <generated/git.h> 

#include "ts4231.h" 
#include "ir_uart.h" 
#include "imu.h" 
#include "spi.h" 
#include "pogobot_timer_sleep_stopwatch.h"
#include "pogobot_ir.h"
#include "rgb.h"
#include "FifoBuffer.h"
#include "sleep.h"
#include "adc.h"
#include "slip.h"


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
#define MAX_PAYLOAD_SIZE_BYTES 384
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
 * - uint16_t receiver_id         - id of the recipient robot
 * - uint8_t _receiver_ir_index   - IR id direction that received the message
 * - uint8_t _sequence_number     - (not used for now)
 * - uint8_t user_tag             - allows the user to give a tag to a message
 * - uint16_t payload_length      - size of the payload
 *
 */

typedef struct message_header_t
{
    uint8_t _packet_type;
    uint8_t _emitting_power_list;
    uint16_t _sender_id;
    uint8_t _sender_ir_index;
    uint16_t receiver_id;
    uint8_t _receiver_ir_index;
    uint8_t _sequence_number;
    uint8_t user_tag;
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

/** (pogobot_infrared_sendMessageOnce)
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
uint32_t pogobot_infrared_sendMessageOnce( message_t *const message );

/** (pogobot_infrared_sendMessageOneDirection)
 * Send a message in only direction at defined power
 * Use pogobot_infrared_sendMessageOnce
 *
 * # Parameters
 * - 'dir' - indicates the direction to send the message
 * - 'receiver_id' - is the id of the robot we want to send the message
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 */
uint32_t pogobot_infrared_sendMessageOneDirection( ir_direction dir, uint16_t receiver_id, uint8_t *message, uint16_t message_size );

/** (pogobot_infrared_sendMessageAllDirection)
 * Send the same message in all direction at defined power
 * Use pogobot_infrared_sendMessageOnce
 * Their no infrared sender ID
 *
 * # Parameters
 * - 'receiver_id' - is the id of the robot we want to send the message
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 */
uint32_t pogobot_infrared_sendMessageAllDirection( uint16_t receiver_id, uint8_t *message, uint16_t message_size );

/** (pogobot_infrared_sendMessageAllDirectionWithId)
 * Send successively the same message with the origin infrared ID on each Infrared
 * It is 4 times slower that without ID
 * Use pogobot_infrared_sendMessageOnce
 *
 * # Parameters
 * - 'receiver_id' - is the id of the robot we want to send the message
 * - 'message' - the current payload to send
 * - 'message_size' - the size of the payload
 *
 * # Return
 * - '0' in case of success
 * - '1' in case of payload too long
 *
 */
uint32_t pogobot_infrared_sendMessageAllDirectionWithId( uint16_t receiver_id, uint8_t *message, uint16_t message_size );

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
 * - 0 is the rear sensor
 * - 1 is the right sensor
 * - 2 is the left sensor
 */

typedef enum
{
    p_FR = 1,
    p_FL = 2,
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

/** (pogobot_motor_set)
 * set the value of pwm that commands the motor
 * 
 * # Parameters
 * - 'motor' - is the id of the motor you want to command
 * - 'value' - is PWM ratio apply to the motor (between 0 and 1023)
 *
 * # Return
 * - none
 *
 */
void pogobot_motor_set( motor_id motor, uint16_t value );

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

#endif /* __POGOBOT_H__ */
