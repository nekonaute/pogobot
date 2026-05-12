<a name="line-46"></a>
# POGODOCS

This document presents the simple API available to control the differents functions of the robot

<a name="line-52"></a>
## Global API

<a name="line-56"></a><a name="pogobot_init"></a>
### :arrow_right: pogobot_init

```cpp
void pogobot_init( void ) /* line 67 */
```

Global API Initialisation
This function is mandatory inside your program

#### Parameters
- none

#### Return
- none


<a name="line-70"></a>
## Infrared communication API Values

<a name="line-74"></a>
### Maximum size of a payload in a IR message
MAX_PAYLOAD_SIZE_BYTES 384

<a name="line-83"></a>
### Maximum number of word from IR recovers in one step
MAX_NUMBER_OF_WORD 20

<a name="line-92"></a>
### IR power level values:

- pogobot_infrared_emitter_power_max      - 3
- pogobot_infrared_emitter_power_twoThird - 2
- pogobot_infrared_emitter_power_oneThird - 1
- pogobot_infrared_emitter_power_null     - 0

<a name="line-161"></a>
### IR direction id list

- ir_front - 0
- ir_right - 1
- ir_back  - 2
- ir_left  - 3
- ir_all   - 4

<a name="line-182"></a>
### IR message header structure

message_header_t :

The parameters that begin with "_" are not completed by the user

- uint8_t _packet_type         - allows to give type of a packet (fixed for now)
- uint8_t _emitting_power_list - used to define the emission power on each IR
- uint16_t _sender_id          - id of the robot which send the message
- uint8_t _sender_ir_index     - IR id direction of the sender robot
- uint8_t _receiver_ir_index   - IR id direction that received the message
- uint16_t payload_length      - size of the payload


<a name="line-208"></a>
### IR message header structure

message_t :

- message_header_t header                  - header of the message
- uint8_t payload[MAX_PAYLOAD_SIZE_BYTES]  - payload of the message

<a name="line-223"></a>
### IR short message header structure

message_short_header_t :

The parameters that begin with "_" are not completed by the user

- uint8_t _packet_type         - allows to give type of a packet (fixed for now)
- uint16_t payload_length      - size of the payload


<a name="line-241"></a>
### IR short message header structure

short_message_t :

- message_short_header_t header            - header of the message
- uint8_t payload[MAX_PAYLOAD_SIZE_BYTES]  - payload of the message

<a name="line-256"></a>
### IR type message list

- ir_t_cmd  : type use to send command to the robot
- ir_t_flash : type use to send part of a program
- ir_t_short : type use to send short message between robot in user space
- ir_t_user : type use to send message between robot in user space

<a name="line-286"></a>
## Infrared communication API Functions

<a name="line-290"></a><a name="pogobot_infrared_ll_init"></a>
### :arrow_right: pogobot_infrared_ll_init

```cpp
void pogobot_infrared_ll_init( void ) /* line 301 */
```

Initialise Infrared hardware and software struture
(already made inside pogobot_init)

#### Parameters
- none

#### Return
- none


<a name="line-303"></a><a name="pogobot_infrared_update"></a>
### :arrow_right: pogobot_infrared_update

```cpp
void pogobot_infrared_update( void ) /* line 313 */
```

Infrared checks for received data and send to decode messages
Decoded messages are placed in a Fifo

#### Parameters
- none

#### Return
- none

<a name="line-315"></a><a name="pogobot_infrared_message_available"></a>
### :arrow_right: pogobot_infrared_message_available

```cpp
int pogobot_infrared_message_available( void ) /* line 325 */
```

Infrared new message checks fonction

#### Parameters
- none

#### Return
- none


<a name="line-327"></a><a name="pogobot_infrared_recover_next_message"></a>
### :arrow_right: pogobot_infrared_recover_next_message

```cpp
void pogobot_infrared_recover_next_message( message_t *mes ) /* line 337 */
```

Recover the next message inside the message queue

#### Parameters
- 'mes' - Allocated structure of type 'message_t'

#### Return
- none


<a name="line-339"></a><a name="pogobot_infrared_clear_message_queue"></a>
### :arrow_right: pogobot_infrared_clear_message_queue

```cpp
void pogobot_infrared_clear_message_queue( void ) /* line 349 */
```

Clears Infrared message queue

#### Parameters
- none

#### Return
- none


<a name="line-351"></a><a name="pogobot_infrared_set_power"></a>
### :arrow_right: pogobot_infrared_set_power

```cpp
void pogobot_infrared_set_power( uint8_t power ) /* line 360 */
```

set the power level used to send all the next messages

#### Parameters
- 'power' - use the pogobot_infrared_emitter_power_* or the values {0,1,2,3}

#### Return
- none

<a name="line-362"></a><a name="pogobot_infrared_sendRawLongMessage"></a>
### :arrow_right: pogobot_infrared_sendRawLongMessage

```cpp
uint32_t pogobot_infrared_sendRawLongMessage( message_t *const message ) /* line 374 */
```

 Prepare and send one packet, with the specified emitters and
 powers, to the recipient, containing the specified message.

#### Parameters
- 'message' - fully filled message_t variable

#### Return
- '0' in case of success
- '1' in case of payload too long


<a name="line-376"></a><a name="pogobot_infrared_sendRawShortMessage"></a>
### :arrow_right: pogobot_infrared_sendRawShortMessage

```cpp
uint32_t pogobot_infrared_sendRawShortMessage( ir_direction dir, short_message_t *const message ) /* line 389 */
```

 Prepare and send one packet, with a short header
 containing the specified message.

#### Parameters
- 'dir' - indicates the direction to send the message
- 'message' - fully filled short_message_t variable

#### Return
- '0' in case of success
- '1' in case of payload too long


<a name="line-391"></a><a name="pogobot_infrared_sendLongMessage_uniSpe"></a>
### :arrow_right: pogobot_infrared_sendLongMessage_uniSpe

```cpp
uint32_t pogobot_infrared_sendLongMessage_uniSpe( ir_direction dir, uint8_t *message, uint16_t message_size ) /* line 404 */
```

Send a message in only direction at defined power
Use pogobot_infrared_sendRawLongMessage

#### Parameters
- 'dir' - indicates the direction to send the message
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long

<a name="line-406"></a><a name="pogobot_infrared_sendLongMessage_omniGen"></a>
### :arrow_right: pogobot_infrared_sendLongMessage_omniGen

```cpp
uint32_t pogobot_infrared_sendLongMessage_omniGen( uint8_t *message, uint16_t message_size ) /* line 419 */
```

Send the same message in all direction at defined power
Use pogobot_infrared_sendRawLongMessage
Their no infrared sender ID

#### Parameters
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long

<a name="line-421"></a><a name="pogobot_infrared_sendLongMessage_omniSpe"></a>
### :arrow_right: pogobot_infrared_sendLongMessage_omniSpe

```cpp
uint32_t pogobot_infrared_sendLongMessage_omniSpe( uint8_t *message, uint16_t message_size ) /* line 435 */
```

Send successively the same message with the origin infrared ID on each Infrared
It is 4 times slower that without ID
Use pogobot_infrared_sendRawLongMessage

#### Parameters
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long


<a name="line-438"></a><a name="pogobot_infrared_sendShortMessage_uni"></a>
### :arrow_right: pogobot_infrared_sendShortMessage_uni

```cpp
uint32_t pogobot_infrared_sendShortMessage_uni( ir_direction dir, uint8_t *message, uint16_t message_size ) /* line 451 */
```

Send a short header message in only direction at defined power
Use pogobot_infrared_sendRawShortMessage

#### Parameters
- 'dir' - indicates the direction to send the message
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long

<a name="line-453"></a><a name="pogobot_infrared_sendShortMessage_omni"></a>
### :arrow_right: pogobot_infrared_sendShortMessage_omni

```cpp
uint32_t pogobot_infrared_sendShortMessage_omni( uint8_t *message, uint16_t message_size ) /* line 465 */
```

Send a short header message in all direction at defined power
Use pogobot_infrared_sendRawShortMessage

#### Parameters
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long

<a name="line-468"></a><a name="pogobot_infrared_get_receiver_error_counter"></a>
### :arrow_right: pogobot_infrared_get_receiver_error_counter

```cpp
void pogobot_infrared_get_receiver_error_counter( slip_error_counter_s *error_counter, uint8_t ir_index ) /* line 478 */
```

Get the receiver error counter value

#### Parameters
- 'error_counter' - allocated structure 'slip_error_counter_s'
- 'ir_index' - index of the ir receiver (0 to 3)

#### Return
- none

<a name="line-480"></a><a name="pogobot_infrared_reset_receiver_error_counter"></a>
### :arrow_right: pogobot_infrared_reset_receiver_error_counter

```cpp
void pogobot_infrared_reset_receiver_error_counter( void ) /* line 490 */
```

Reset all reveiver error counter

#### Parameters
- none

#### Return
- none


<a name="line-492"></a>
## RGB LED API

<a name="line-496"></a><a name="pogobot_led_setColor"></a>
### :arrow_right: pogobot_led_setColor

```cpp
void pogobot_led_setColor( const uint8_t r, const uint8_t g, const uint8_t b ) /* line 509 */
```

Set the value of red, green and blue of the head led in static mode
each value goes from 0 to 255 to determine the intensity.

#### Parameters
- 'r' - value of the RED part
- 'g' - value of the GREEN part
- 'b' - value of the BLUE part

#### Return
- none


<a name="line-512"></a><a name="pogobot_led_setColors"></a>
### :arrow_right: pogobot_led_setColors

```cpp
void pogobot_led_setColors( const uint8_t r, const uint8_t g, const uint8_t b, uint8_t id ) /* line 527 */
```

Set the value of red, green and blue of the led with the number (ID) in static mode
each value goes from 0 to 255 to determine the intensity.
(only available with a belly with multiple LEDs)

#### Parameters
- 'r' - value of the RED part
- 'g' - value of the GREEN part
- 'b' - value of the BLUE part
- 'id' - Led ID (0 = head, 1 = belly front, 2 = belly right, 3 = belly back, 4 = belly left)

#### Return
- none


<a name="line-530"></a>
## Photosensors API Values

<a name="line-534"></a>
### Photosensor id definition
- 0 is the back sensor
- 1 is the front-left sensor
- 2 is the front-right sensor

<a name="line-548"></a>
## Photosensors API Functions

<a name="line-552"></a><a name="pogobot_photosensors_read"></a>
### :arrow_right: pogobot_photosensors_read

```cpp
int16_t pogobot_photosensors_read( uint8_t sensor_number ) /* line 562 */
```

Read one ambient light sensor.
Sensor number must be between 0 and 2.

#### Parameters
- 'sensor_number' - id of the sensor (Photosensor definition)

#### Return
Return a value proportional to the light

<a name="line-564"></a>
## IMU API

<a name="line-568"></a><a name="pogobot_imu_read"></a>
### :arrow_right: pogobot_imu_read

```cpp
void pogobot_imu_read( float *acc, float *gyro ) /* line 588 */
```

Read the accelaration on the IMU. <br>
Returns in acc and gyro the IMU values.

usage : <br>
float acc[3], gyro[3];

index :

- 0 - X axis
- 1 - Y axis
- 2 - Z axis

#### Parameters
- 'acc' - allocated float table (size 3)
- 'gyro' - allocated float table (size 3)

#### Return
- none

<a name="line-590"></a><a name="pogobot_imu_readTemp"></a>
### :arrow_right: pogobot_imu_readTemp

```cpp
float pogobot_imu_readTemp( void ) /* line 599 */
```

Read the temparature sensor on the IMU.

#### Parameters
- none

#### Return
Returns the temperature in degres celsius

<a name="line-602"></a>
## Battery API

<a name="line-606"></a><a name="pogobot_battery_voltage_read"></a>
### :arrow_right: pogobot_battery_voltage_read

```cpp
int16_t pogobot_battery_voltage_read( void ) /* line 615 */
```

Recovers the value of the battery in mV

#### Parameters
- none

#### Return
Returns a value in mV

<a name="line-617"></a>
## Motors API Values

<a name="line-621"></a>
### Motor id definition :

- motorR  - 0
- motorL  - 1
- motorB  - 2

<a name="line-636"></a>
### Motor range :

- motorStop                   - 0
- motorQuarter                - 256
- motorHalfmotorThreeQuarter  - 716
- motorFull                   - 1023

<a name="line-654"></a>
## Motors API Functions

<a name="line-658"></a><a name="pogobot_motor_power_set"></a>
### :arrow_right: pogobot_motor_power_set

```cpp
void pogobot_motor_power_set( motor_id motor, uint16_t value ) /* line 669 */
```

set the value of pwm that commands the motor (active)

#### Parameters
- 'motor' - is the id of the motor you want to command
- 'value' - is PWM ratio apply to the motor (between 0 and 1023)

#### Return
- none


<a name="line-672"></a><a name="pogobot_motor_dir_current_status"></a>
### :arrow_right: pogobot_motor_dir_current_status

```cpp
uint32_t pogobot_motor_dir_current_status( void ) /* line 682 */
```

recover the value of the motor direction bit field.

#### Parameters
- none

#### Return
- bit field ( XXXX XMLR )


<a name="line-684"></a><a name="pogobot_motor_dir_mem_get"></a>
### :arrow_right: pogobot_motor_dir_mem_get

```cpp
int8_t pogobot_motor_dir_mem_get( uint8_t *p_directions ) /* line 695 */
```

get the value of pwm that commands the motor.

#### Parameters
- 'p_directions' - is an array of size 3 [R, L, B].
           Each value is the chosen direction (0 or 1)

#### Return
- the success or not of the read in memory (0: Ok, -1: NOk)


<a name="line-697"></a><a name="pogobot_motor_dir_mem_set"></a>
### :arrow_right: pogobot_motor_dir_mem_set

```cpp
int8_t pogobot_motor_dir_mem_set( uint8_t *p_directions) /* line 708 */
```

set the value of pwm that commands the motor (persistent).

#### Parameters
- 'p_directions' - is an array of size 3 [R, L, B].
           Each value is the chosen direction (0 or 1)

#### Return
- the success or not of the read in memory (0: Ok, -1: NOk)


<a name="line-710"></a><a name="pogobot_motor_dir_set"></a>
### :arrow_right: pogobot_motor_dir_set

```cpp
void pogobot_motor_dir_set( motor_id motor, uint8_t value ) /* line 721 */
```

set the value of pwm that commands the motor (active).

#### Parameters
- 'motor' - is the id of the motor you want to command (motorR, motorL, motorB)
- 'value' - is the choosen direction (0 or 1)

#### Return
- none


<a name="line-723"></a><a name="pogobot_motor_power_mem_get"></a>
### :arrow_right: pogobot_motor_power_mem_get

```cpp
uint8_t pogobot_motor_power_mem_get( uint16_t *p_powers ) /* line 734 */
```

recover the value of the motor power memorized.

#### Parameters
- 'p_powers' - is an array of size 3 [R, L, B].
           Each value is the chosen power [0, 1023]

#### Return
- the success or not of the read in memory (0: Ok, -1: NOK)


<a name="line-736"></a><a name="pogobot_motor_power_mem_set"></a>
### :arrow_right: pogobot_motor_power_mem_set

```cpp
uint8_t pogobot_motor_power_mem_set( uint16_t *p_powers ) /* line 747 */
```

write the value of the motor power in memory (persistent).

#### Parameters
- 'p_powers' - is an array of size 3 [R, L, B].
           Each value is the chosen power [0, 1023]

#### Return
- the success or not of the read in memory (0: Ok, -1: NOK)


<a name="line-749"></a>
## Helper API

<a name="line-753"></a><a name="pogobot_helper_getid"></a>
### :arrow_right: pogobot_helper_getid

```cpp
uint16_t pogobot_helper_getid( void ) /* line 763 */
```

gives an unique identifier or an random number if no id is register inside the robot

#### Parameters
- none

#### Return
Returns an unique id on 16bits


<a name="line-765"></a><a name="pogobot_helper_getRandSeed"></a>
### :arrow_right: pogobot_helper_getRandSeed

```cpp
int16_t pogobot_helper_getRandSeed( void ) /* line 775 */
```

gives an seed base on the ADC read of the battery

#### Parameters
- none

#### Return
Returns an random id on 16bits


<a name="line-777"></a><a name="pogobot_helper_print_version"></a>
### :arrow_right: pogobot_helper_print_version

```cpp
void pogobot_helper_print_version( void ) /* line 788 */
```

print the version inside the prompt
It is also possible to use the define RELEASE_VERSION in release.h

#### Parameters
- none

#### Return
- none


<a name="line-790"></a>
## Time API

<a name="line-795"></a>
This is a simple timer implementation based on the available hardware timer.

The benefit is that the only hardware timer we have is configured only once, and all software that depend on it just works without conflict.

The drawback is that we can't use the interrupt of the hardware timer, if available.

To keep the implementation simple, we only rely on the 32bit timer0 counter.
This limit the timespan to 2^31/CLOCKRATE, which is 107 seconds with a 20MHz system clock.
We can extent the implementation by maintaining a rollover count in some function that we promise to call often enough, add a rollover count in time_reference_t, and extend computations accordingly.

Sample code:


One time interval, measuring duration, think "get elapsed microseconds".

time_reference_t mystopwatch;
pogobot_stopwatch_reset( &mystopwatch );
my long_computation( arguments );
uint32_t microseconds =
    pogobot_stopwatch_get_elapsed_microseconds( &mystopwatch );
printf( "Duration: %u microseconds", milliseconds );


Series of events, measuring duration of each, think "stopwatch lap".

time_reference_t mystopwatch;
pogobot_stopwatch_reset( &mystopwatch );
while ( 1 )
{
    wait_for_some_external_event();
    uint32_t microseconds = pogobot_stopwatch_lap( &mystopwatch );
    printf( "Time since previous event: %u microseconds", milliseconds );
}


You want your code to periodically do something.

time_reference_t mytimer;
uint32_t period_microseconds = 250000;
pogobot_timer_init( &mytimer, period_microseconds );
while (1)
{
   wait_for_some_external_event();
   if (pogobot_timer_has_expired())
   {
       pogobot_timer_offset_origin_microseconds( &mytimer,
                                                 period_microseconds );
       react_do_something();
   }
}



<a name="line-850"></a><a name="pli_timer_sleep_stopwatch_init"></a>
### :arrow_right: pli_timer_sleep_stopwatch_init

```cpp
void pli_timer_sleep_stopwatch_init( void ) /* line 861 */
```

Initialise the timer structure
(already made inside pogobot_init)

#### Parameters
- none

#### Return
- none


<a name="line-863"></a>
### typedef struct time_reference_t

```cpp
typedef struct time_reference_t /* line 872 */
```

###### Timer structure

time_reference_t :

- uint32_t hardware_value_at_time_origin - timer reference



<a name="line-878"></a><a name="pogobot_stopwatch_reset"></a>
### :arrow_right: pogobot_stopwatch_reset

```cpp
void pogobot_stopwatch_reset( time_reference_t *stopwatch ) /* line 889 */
```

reset a time_reference structure.
To use a time_reference_t as a stopwatch you must reset it using pogobot_stopwatch_reset()

#### Parameters
- 'stopwatch' - pointer to a time_reference_t structure

#### Return
- none


<a name="line-891"></a><a name="pogobot_stopwatch_lap"></a>
### :arrow_right: pogobot_stopwatch_lap

```cpp
int32_t pogobot_stopwatch_lap( time_reference_t *stopwatch ) /* line 901 */
```

measures time elapsed from origin and offsets so that origin is zero at this point in time.

#### Parameters
- 'stopwatch' - pointer to a time_reference_t structure

#### Return
Returns the number of microseconds elapsed on 32 bits (which may be negative if you offset the origin to the future)


<a name="line-903"></a><a name="pogobot_stopwatch_get_elapsed_microseconds"></a>
### :arrow_right: pogobot_stopwatch_get_elapsed_microseconds

```cpp
int32_t pogobot_stopwatch_get_elapsed_microseconds( time_reference_t *stopwatch ) /* line 913 */
```

provides the current number of elapsed microseconds without otherwise interfering with the stopwatch state.

#### Parameters
- 'stopwatch' - pointer to a time_reference_t structure

#### Return
Returns the number of microseconds elapsed on 32 bits (which may be negative if you offset the origin to the future)


<a name="line-915"></a><a name="pogobot_stopwatch_offset_origin_microseconds"></a>
### :arrow_right: pogobot_stopwatch_offset_origin_microseconds

```cpp
void pogobot_stopwatch_offset_origin_microseconds( time_reference_t *stopwatch, int32_t microseconds_offset ) /* line 926 */
```

offsets the origin of the stopwatch by the specified number of microseconds.

#### Parameters
- 'stopwatch' - pointer to a time_reference_t structure
- 'microseconds_offset' - number of microsenconds to offset the origin

#### Return
- none


<a name="line-928"></a><a name="pogobot_timer_init"></a>
### :arrow_right: pogobot_timer_init

```cpp
void pogobot_timer_init( time_reference_t *timer, int32_t microseconds_to_go ) /* line 939 */
```

set a timer that will expire in the defined number of microseconds in the future.

#### Parameters
- 'timer' - pointer to a time_reference_t structure
- 'microseconds_to_go' - number of microsenconds to go

#### Return
- none


<a name="line-941"></a><a name="pogobot_timer_get_remaining_microseconds"></a>
### :arrow_right: pogobot_timer_get_remaining_microseconds

```cpp
int32_t pogobot_timer_get_remaining_microseconds( time_reference_t *timer ) /* line 951 */
```

provides the current of microseconds until the timer has expired, without otherwise interfering with the timer state.

#### Parameters
- 'timer' - pointer to a time_reference_t structure

#### Return
Returns the number of microseconds elapsed on 32 bits. The result is a signed number, positive when the timer has not expired yet, negative when the timer has expired.


<a name="line-953"></a><a name="pogobot_timer_has_expired"></a>
### :arrow_right: pogobot_timer_has_expired

```cpp
bool pogobot_timer_has_expired( time_reference_t *timer ) /* line 963 */
```

Returns true when the timer has expired, false when the timer has not expired yet.

#### Parameters
- 'timer' - pointer to a time_reference_t structure

#### Return
Returns a bool depending on the status (True: if expired, False: if not expired)


<a name="line-965"></a><a name="pogobot_timer_wait_for_expiry"></a>
### :arrow_right: pogobot_timer_wait_for_expiry

```cpp
void pogobot_timer_wait_for_expiry( time_reference_t *timer ) /* line 975 */
```

waits until the timer has expired.

#### Parameters
- 'timer' - pointer to a time_reference_t structure

#### Return
- none


<a name="line-977"></a><a name="pogobot_timer_offset_origin_microseconds"></a>
### :arrow_right: pogobot_timer_offset_origin_microseconds

```cpp
void pogobot_timer_offset_origin_microseconds( time_reference_t *timer, int32_t microseconds_offset ) /* line 988 */
```

offsets the origin of the timer by the specified number of microseconds.

#### Parameters
- 'timer' - pointer to a time_reference_t structure
- 'microseconds_offset' - number of microsenconds to offset the origin

#### Return
- none

