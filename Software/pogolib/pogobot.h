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
 * ## Global API
 */

/**
 * @brief Global API initialization.
 * This function is mandatory inside your program.
 *
 * @return None
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
 * @brief Infrared direction identifiers.
 *
 * - ir_front: 0
 * - ir_right: 1
 * - ir_back: 2
 * - ir_left: 3
 * - ir_all: 4
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
 * @brief Infrared message header.
 *
 * The fields prefixed with '_' are filled by the infrared stack and should
 * not be modified by user code.
 *
 * @param _packet_type Packet type identifier.
 * @param _emitting_power_list Emission power list for each IR emitter.
 * @param _sender_id Sender robot identifier.
 * @param _sender_ir_index Sender IR direction index.
 * @param _receiver_ir_index Receiver IR direction index.
 * @param payload_length Payload size in bytes.
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
 * @brief Full infrared message.
 *
 * Contains a message header and an associated payload buffer.
 */
typedef struct message_t
{
    message_header_t header;
    uint8_t payload[MAX_PAYLOAD_SIZE_BYTES];
} message_t;

/**
 * @brief Short infrared message header.
 *
 * The fields prefixed with '_' are filled by the infrared stack.
 *
 * @param _packet_type Packet type identifier.
 * @param payload_length Payload size in bytes.
 */
typedef struct message_short_header_t
{
    uint8_t _packet_type;
    uint16_t payload_length;
} message_short_header_t;

/**
 * @brief Short infrared message.
 *
 * Contains a short message header and an associated payload buffer.
 */
typedef struct short_message_t
{
    message_short_header_t header;
    uint8_t payload[MAX_PAYLOAD_SIZE_BYTES];
} short_message_t;

/**
 * @brief Infrared message types.
 *
 * - ir_t_cmd: Command message.
 * - ir_t_flash: Program transfer message.
 * - ir_t_short: Short user-space message.
 * - ir_t_user: User-space message.
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

/**
 * @brief Initialize infrared hardware and software structures.
 * This is already done inside pogobot_init().
 *
 * @return None
 */
void pogobot_infrared_ll_init( void );

/**
 * @brief Check infrared reception and decode any received messages.
 * Decoded messages are placed in a FIFO.
 *
 * @return None
 */
void pogobot_infrared_update( void );

/**
 * @brief Check whether a new infrared message is available.
 *
 * @return Non-zero if a message is available, zero otherwise.
 */
int pogobot_infrared_message_available( void );

/**
 * @brief Recover the next message from the infrared message queue.
 *
 * @param mes Allocated structure of type message_t.
 * @return None
 */
void pogobot_infrared_recover_next_message( message_t *mes );

/**
 * @brief Clears the infrared message queue.
 *
 * @return None
 */
void pogobot_infrared_clear_message_queue( void );

/**
 * @brief Set the infrared emitter power for subsequent messages.
 *
 * @param power Use pogobot_infrared_emitter_power_* or values {0,1,2,3}.
 * @return None
 */
void pogobot_infrared_set_power( uint8_t power );

/**
 * @brief Prepare and send a raw long infrared message.
 *
 * @param message Fully filled message_t variable.
 * @return 0 on success, 1 if the payload is too long.
 */
uint32_t pogobot_infrared_sendRawLongMessage( message_t *const message );

/**
 * @brief Prepare and send a raw short infrared message.
 *
 * @param dir Direction to send the message.
 * @param message Fully filled short_message_t variable.
 * @return 0 on success, 1 if the payload is too long.
 */
uint32_t pogobot_infrared_sendRawShortMessage( ir_direction dir, short_message_t *const message );

/**
 * @brief Send a long infrared message in a single direction at the current power.
 * Uses pogobot_infrared_sendRawLongMessage().
 *
 * @param dir Direction to send the message.
 * @param message Payload buffer.
 * @param message_size Payload size.
 * @return 0 on success, 1 if the payload is too long.
 */
uint32_t pogobot_infrared_sendLongMessage_uniSpe( ir_direction dir, uint8_t *message, uint16_t message_size );

/**
 * @brief Send the same long infrared message on all directions at the current power.
 * Uses pogobot_infrared_sendRawLongMessage().
 * No sender ID is encoded.
 *
 * @param message Payload buffer.
 * @param message_size Payload size.
 * @return 0 on success, 1 if the payload is too long.
 */
uint32_t pogobot_infrared_sendLongMessage_omniGen( uint8_t *message, uint16_t message_size );

/**
 * @brief Send the same long infrared message successively with the sender infrared ID encoded per direction.
 * This is approximately four times slower than omniGen.
 * Uses pogobot_infrared_sendRawLongMessage().
 *
 * @param message Payload buffer.
 * @param message_size Payload size.
 * @return 0 on success, 1 if the payload is too long.
 */
uint32_t pogobot_infrared_sendLongMessage_omniSpe( uint8_t *message, uint16_t message_size );


/**
 * @brief Send a short header infrared message in a single direction.
 * Uses pogobot_infrared_sendRawShortMessage().
 *
 * @param dir Direction to send the message.
 * @param message Payload buffer.
 * @param message_size Payload size.
 * @return 0 on success, 1 if the payload is too long.
 */
uint32_t pogobot_infrared_sendShortMessage_uni( ir_direction dir, uint8_t *message, uint16_t message_size );

/**
 * @brief Send a short header infrared message to all directions.
 * Uses pogobot_infrared_sendRawShortMessage().
 *
 * @param message Payload buffer.
 * @param message_size Payload size.
 * @return 0 on success, 1 if the payload is too long.
 */
uint32_t pogobot_infrared_sendShortMessage_omni( uint8_t *message, uint16_t message_size );


/**
 * @brief Get the receiver error counter value for a specific IR receiver.
 *
 * @param error_counter Allocated slip_error_counter_s structure.
 * @param ir_index Index of the IR receiver (0 to 3).
 * @return None
 */
void pogobot_infrared_get_receiver_error_counter( slip_error_counter_s *error_counter, uint8_t ir_index );

/**
 * @brief Reset all receiver error counters.
 *
 * @return None
 */
void pogobot_infrared_reset_receiver_error_counter( void );

/** 
 * ## RGB LED API
 */

/**
 * @brief Set the head RGB LED color in static mode.
 * Each component is in the range 0..255.
 *
 * @param r Red component.
 * @param g Green component.
 * @param b Blue component.
 * @return None
 */
void pogobot_led_setColor( const uint8_t r, const uint8_t g, const uint8_t b );

#ifdef RGB_LEDS
/**
 * @brief Set the RGB color of a specific LED by ID in static mode.
 * Only available when the belly has multiple LEDs.
 *
 * @param r Red component.
 * @param g Green component.
 * @param b Blue component.
 * @param id LED ID (0=head, 1=belly front, 2=belly right, 3=belly back, 4=belly left).
 * @return None
 */
void pogobot_led_setColors( const uint8_t r, const uint8_t g, const uint8_t b, uint8_t id );
#endif

/**
 * @brief Photosensor identifiers.
 *
 * - p_B: Back sensor (0)
 * - p_FL: Front-left sensor (1)
 * - p_FR: Front-right sensor (2)
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

/**
 * @brief Read one ambient light sensor.
 * Sensor number must be between 0 and 2.
 *
 * @param sensor_number ID of the sensor (Photosensor definition).
 * @return A value proportional to the light.
 */
int16_t pogobot_photosensors_read( uint8_t sensor_number );

/**
 * ## IMU API 
 */

/**
 * @brief Read acceleration and gyro values from the IMU.
 * Results are returned in acc and gyro.
 *
 * Usage:
 * float acc[3], gyro[3];
 *
 * Index:
 * - 0: X axis
 * - 1: Y axis
 * - 2: Z axis
 *
 * @param acc Allocated float array of size 3.
 * @param gyro Allocated float array of size 3.
 * @return None
 */
void pogobot_imu_read( float *acc, float *gyro );

/**
 * @brief Read the IMU temperature sensor.
 *
 * @return Temperature in degrees Celsius.
 */
float pogobot_imu_readTemp( void );


/**
 * ## Battery API 
 */

/**
 * @brief Read the battery voltage.
 *
 * @return Battery voltage in millivolts.
 */
int16_t pogobot_battery_voltage_read( void );

/**
 * ## Motors API Values
 */

/**
 * @brief Motor identifiers.
 *
 * - motorR: Right motor (0)
 * - motorL: Left motor (1)
 * - motorB: Back motor (2)
 */
typedef enum
{
    motorR = 0, // Right
    motorL = 1, // Left
    motorB = 2  // Back
} motor_id;

/**
 * @brief Standard motor power range values.
 *
 * - motorStop: 0
 * - motorQuarter: 256
 * - motorHalf: 512
 * - motorThreeQuarter: 716
 * - motorFull: 1023
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

/**
 * @brief Set the PWM power for the given motor.
 *
 * @param motor Motor ID to command.
 * @param value PWM ratio applied to the motor (0..1023).
 * @return None
 */
void pogobot_motor_power_set( motor_id motor, uint16_t value );
void pogobot_motor_set ( motor_id motor, uint16_t value );

/**
 * @brief Read the current motor direction bit field.
 *
 * @return Bit field representing motor direction status.
 */
uint32_t pogobot_motor_dir_current_status( void );

/**
 * @brief Read the stored motor direction values from memory.
 *
 * @param p_directions Array of size 3 [R, L, B]. Each value is 0 or 1.
 * @return 0 on success, -1 on failure.
 */
int8_t pogobot_motor_dir_mem_get( uint8_t *p_directions );

/**
 * @brief Store motor direction values persistently in memory.
 *
 * @param p_directions Array of size 3 [R, L, B]. Each value is 0 or 1.
 * @return 0 on success, -1 on failure.
 */
int8_t pogobot_motor_dir_mem_set( uint8_t *p_directions);

/**
 * @brief Set the active direction for the given motor.
 *
 * @param motor Motor ID to command (motorR, motorL, motorB).
 * @param value Direction value, 0 or 1.
 * @return None
 */
void pogobot_motor_dir_set( motor_id motor, uint8_t value );

/**
 * @brief Read stored motor power values from memory.
 *
 * @param p_powers Array of size 3 [R, L, B]. Each value is 0..1023.
 * @return 0 on success, -1 on failure.
 */
uint8_t pogobot_motor_power_mem_get( uint16_t *p_powers );

/**
 * @brief Store motor power values persistently in memory.
 *
 * @param p_powers Array of size 3 [R, L, B]. Each value is 0..1023.
 * @return 0 on success, -1 on failure.
 */
uint8_t pogobot_motor_power_mem_set( uint16_t *p_powers );

/**
 * ## Helper API
 */

/**
 * @brief Return a unique identifier, or a random number if none is registered.
 *
 * @return Unique 16-bit ID.
 */
uint16_t pogobot_helper_getid( void );

/**
 * @brief Return a seed based on the ADC battery reading.
 *
 * @return Random 16-bit seed.
 */
int16_t pogobot_helper_getRandSeed( void );

/**
 * @brief Print the library version to the console.
 * It is also possible to use RELEASE_VERSION from release.h.
 *
 * @return None
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

/**
 * @brief Initialise timer and stopwatch support.
 * This is already done inside pogobot_init().
 *
 * @return None
 */
void pli_timer_sleep_stopwatch_init( void );

/**
 * @brief Timer reference structure.
 *
 * Contains the hardware timer counter value at the reference origin.
 */
typedef struct time_reference_t
{
    uint32_t hardware_value_at_time_origin;
} time_reference_t;


/**
 * @brief Reset a stopwatch origin reference.
 * To use a time_reference_t as a stopwatch, call this first.
 *
 * @param stopwatch Pointer to a time_reference_t structure.
 * @return None
 */
void pogobot_stopwatch_reset( time_reference_t *stopwatch );

/**
 * @brief Measure elapsed time and reset the stopwatch origin to now.
 *
 * @param stopwatch Pointer to a time_reference_t structure.
 * @return Elapsed microseconds as a signed 32-bit value.
 */
int32_t pogobot_stopwatch_lap( time_reference_t *stopwatch );

/**
 * @brief Get elapsed microseconds without modifying stopwatch state.
 *
 * @param stopwatch Pointer to a time_reference_t structure.
 * @return Elapsed microseconds as a signed 32-bit value.
 */
int32_t pogobot_stopwatch_get_elapsed_microseconds( time_reference_t *stopwatch );

/**
 * @brief Offset the stopwatch origin by a number of microseconds.
 *
 * @param stopwatch Pointer to a time_reference_t structure.
 * @param microseconds_offset Number of microseconds to offset the origin.
 * @return None
 */
void pogobot_stopwatch_offset_origin_microseconds( time_reference_t *stopwatch, int32_t microseconds_offset );

/**
 * @brief Set a timer to expire after the specified number of microseconds.
 *
 * @param timer Pointer to a time_reference_t structure.
 * @param microseconds_to_go Number of microseconds until expiry.
 * @return None
 */
void pogobot_timer_init( time_reference_t *timer, int32_t microseconds_to_go );

/**
 * @brief Get the remaining microseconds until timer expiry.
 *
 * @param timer Pointer to a time_reference_t structure.
 * @return Signed microseconds remaining; positive before expiry, negative after expiry.
 */
int32_t pogobot_timer_get_remaining_microseconds( time_reference_t *timer );

/**
 * @brief Test whether the timer has expired.
 *
 * @param timer Pointer to a time_reference_t structure.
 * @return true if expired, false otherwise.
 */
bool pogobot_timer_has_expired( time_reference_t *timer );

/**
 * @brief Wait until the timer expires.
 *
 * @param timer Pointer to a time_reference_t structure.
 * @return None
 */
void pogobot_timer_wait_for_expiry( time_reference_t *timer );

/**
 * @brief Offset the timer origin by a specified number of microseconds.
 *
 * @param timer Pointer to a time_reference_t structure.
 * @param microseconds_offset Number of microseconds to offset the origin.
 * @return None
 */
void pogobot_timer_offset_origin_microseconds( time_reference_t *timer, int32_t microseconds_offset );


#endif /* __POGOBOT_H__ */
