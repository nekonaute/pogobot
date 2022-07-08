<a name="line-45"></a>
# POGODOCS

This document presents the simple API available to control the differents functions of the robot

<a name="line-51"></a>
## Global API

<a name="line-55"></a><a name="pogobot_init"></a>
### :arrow_right: pogobot_init

```cpp
void pogobot_init( void ) /* line 66 */
```

Global API Initialisation
This function is mandatory inside your program

#### Parameters
- none

#### Return
- none


<a name="line-69"></a>
## Infrared communication API Values

<a name="line-73"></a>
### Maximum size of a payload in a IR message
MAX_PAYLOAD_SIZE_BYTES 384

<a name="line-82"></a>
### Maximum number of word from IR recovers in one step
MAX_NUMBER_OF_WORD 20

<a name="line-91"></a>
### IR power level values:

- pogobot_infrared_emitter_power_max      - 3
- pogobot_infrared_emitter_power_twoThird - 2
- pogobot_infrared_emitter_power_oneThird - 1
- pogobot_infrared_emitter_power_null     - 0

<a name="line-160"></a>
### IR direction id list

- ir_front - 0
- ir_right - 1
- ir_back  - 2
- ir_left  - 3
- ir_all   - 4

<a name="line-181"></a>
### IR message header structure

message_header_t :

The parameters that begin with "_" are not completed by the user

- uint8_t _packet_type         - allows to give type of a packet (fixed for now)
- uint8_t _emitting_power_list - used to define the emission power on each IR
- uint16_t _sender_id          - id of the robot which send the message
- uint8_t _sender_ir_index     - IR id direction of the sender robot
- uint16_t receiver_id         - id of the recipient robot
- uint8_t _receiver_ir_index   - IR id direction that received the message
- uint8_t _sequence_number     - (not used for now)
- uint8_t user_tag             - allows the user to give a tag to a message
- uint16_t payload_length      - size of the payload


<a name="line-213"></a>
### IR message header structure

message_t :

- message_header_t header                  - header of the message
- uint8_t payload[MAX_PAYLOAD_SIZE_BYTES]  - payload of the message

<a name="line-240"></a>
## Infrared communication API Functions

<a name="line-244"></a><a name="pogobot_infrared_ll_init"></a>
### :arrow_right: pogobot_infrared_ll_init

```cpp
void pogobot_infrared_ll_init( void ) /* line 255 */
```

Initialise Infrared hardware and software struture
(already made inside pogobot_init)

#### Parameters
- none

#### Return
- none


<a name="line-257"></a><a name="pogobot_infrared_update"></a>
### :arrow_right: pogobot_infrared_update

```cpp
void pogobot_infrared_update( void ) /* line 267 */
```

Infrared checks for received data and send to decode messages
Decoded messages are placed in a Fifo

#### Parameters
- none

#### Return
- none

<a name="line-269"></a><a name="pogobot_infrared_message_available"></a>
### :arrow_right: pogobot_infrared_message_available

```cpp
int pogobot_infrared_message_available( void ) /* line 279 */
```

Infrared new message checks fonction

#### Parameters
- none

#### Return
- none


<a name="line-281"></a><a name="pogobot_infrared_recover_next_message"></a>
### :arrow_right: pogobot_infrared_recover_next_message

```cpp
void pogobot_infrared_recover_next_message( message_t *mes ) /* line 291 */
```

Recover the next message inside the message queue

#### Parameters
- 'mes' - Allocated structure of type 'message_t'

#### Return
- none


<a name="line-293"></a><a name="pogobot_infrared_clear_message_queue"></a>
### :arrow_right: pogobot_infrared_clear_message_queue

```cpp
void pogobot_infrared_clear_message_queue( void ) /* line 303 */
```

Clears Infrared message queue

#### Parameters
- none

#### Return
- none


<a name="line-305"></a><a name="pogobot_infrared_set_power"></a>
### :arrow_right: pogobot_infrared_set_power

```cpp
void pogobot_infrared_set_power( uint8_t power ) /* line 314 */
```

set the power level used to send all the next messages

#### Parameters
- 'power' - use the pogobot_infrared_emitter_power_* or the values {0,1,2,3}

#### Return
- none

<a name="line-316"></a><a name="pogobot_infrared_sendMessageOnce"></a>
### :arrow_right: pogobot_infrared_sendMessageOnce

```cpp
uint32_t pogobot_infrared_sendMessageOnce( message_t *const message ) /* line 328 */
```

 Prepare and send one packet, with the specified emitters and
 powers, to the recipient, containing the specified message.

#### Parameters
- 'message' - fully filled message_t variable

#### Return
- '0' in case of success
- '1' in case of payload too long


<a name="line-330"></a><a name="pogobot_infrared_sendMessageOneDirection"></a>
### :arrow_right: pogobot_infrared_sendMessageOneDirection

```cpp
uint32_t pogobot_infrared_sendMessageOneDirection( ir_direction dir, uint16_t receiver_id, uint8_t *message, uint16_t message_size ) /* line 344 */
```

Send a message in only direction at defined power
Use pogobot_infrared_sendMessageOnce

#### Parameters
- 'dir' - indicates the direction to send the message
- 'receiver_id' - is the id of the robot we want to send the message
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long

<a name="line-346"></a><a name="pogobot_infrared_sendMessageAllDirection"></a>
### :arrow_right: pogobot_infrared_sendMessageAllDirection

```cpp
uint32_t pogobot_infrared_sendMessageAllDirection( uint16_t receiver_id, uint8_t *message, uint16_t message_size ) /* line 360 */
```

Send the same message in all direction at defined power
Use pogobot_infrared_sendMessageOnce
Their no infrared sender ID

#### Parameters
- 'receiver_id' - is the id of the robot we want to send the message
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long

<a name="line-362"></a><a name="pogobot_infrared_sendMessageAllDirectionWithId"></a>
### :arrow_right: pogobot_infrared_sendMessageAllDirectionWithId

```cpp
uint32_t pogobot_infrared_sendMessageAllDirectionWithId( uint16_t receiver_id, uint8_t *message, uint16_t message_size ) /* line 377 */
```

Send successively the same message with the origin infrared ID on each Infrared
It is 4 times slower that without ID
Use pogobot_infrared_sendMessageOnce

#### Parameters
- 'receiver_id' - is the id of the robot we want to send the message
- 'message' - the current payload to send
- 'message_size' - the size of the payload

#### Return
- '0' in case of success
- '1' in case of payload too long


<a name="line-379"></a><a name="pogobot_infrared_get_receiver_error_counter"></a>
### :arrow_right: pogobot_infrared_get_receiver_error_counter

```cpp
void pogobot_infrared_get_receiver_error_counter( slip_error_counter_s *error_counter, uint8_t ir_index ) /* line 389 */
```

Get the receiver error counter value

#### Parameters
- 'error_counter' - allocated structure 'slip_error_counter_s'
- 'ir_index' - index of the ir receiver (0 to 3)

#### Return
- none

<a name="line-391"></a><a name="pogobot_infrared_reset_receiver_error_counter"></a>
### :arrow_right: pogobot_infrared_reset_receiver_error_counter

```cpp
void pogobot_infrared_reset_receiver_error_counter( void ) /* line 401 */
```

Reset all reveiver error counter

#### Parameters
- none

#### Return
- none


<a name="line-403"></a>
## RGB LED API

<a name="line-407"></a><a name="pogobot_led_setColor"></a>
### :arrow_right: pogobot_led_setColor

```cpp
void pogobot_led_setColor( const uint8_t r, const uint8_t g, const uint8_t b ) /* line 420 */
```

Set the value of red, green and blue of the head led in static mode
each value goes from 0 to 255 to determine the intensity.

#### Parameters
- 'r' - value of the RED part
- 'g' - value of the GREEN part
- 'b' - value of the BLUE part

#### Return
- none


<a name="line-423"></a><a name="pogobot_led_setColors"></a>
### :arrow_right: pogobot_led_setColors

```cpp
void pogobot_led_setColors( const uint8_t r, const uint8_t g, const uint8_t b, uint8_t id ) /* line 438 */
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


<a name="line-441"></a>
## Photosensors API Values

<a name="line-445"></a>
### Photosensor id definition
- 0 is the rear sensor
- 1 is the right sensor
- 2 is the left sensor

<a name="line-459"></a>
## Photosensors API Functions

<a name="line-463"></a><a name="pogobot_photosensors_read"></a>
### :arrow_right: pogobot_photosensors_read

```cpp
int16_t pogobot_photosensors_read( uint8_t sensor_number ) /* line 473 */
```

Read one ambient light sensor.
Sensor number must be between 0 and 2.

#### Parameters
- 'sensor_number' - id of the sensor (Photosensor definition)

#### Return
Return a value proportional to the light

<a name="line-475"></a>
## IMU API

<a name="line-479"></a><a name="pogobot_imu_read"></a>
### :arrow_right: pogobot_imu_read

```cpp
void pogobot_imu_read( float *acc, float *gyro ) /* line 499 */
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

<a name="line-501"></a><a name="pogobot_imu_readTemp"></a>
### :arrow_right: pogobot_imu_readTemp

```cpp
float pogobot_imu_readTemp( void ) /* line 510 */
```

Read the temparature sensor on the IMU.

#### Parameters
- none

#### Return
Returns the temperature in degres celsius

<a name="line-513"></a>
## Battery API

<a name="line-517"></a><a name="pogobot_battery_voltage_read"></a>
### :arrow_right: pogobot_battery_voltage_read

```cpp
int16_t pogobot_battery_voltage_read( void ) /* line 526 */
```

Recovers the value of the battery in mV

#### Parameters
- none

#### Return
Returns a value in mV

<a name="line-528"></a>
## Motors API Values

<a name="line-532"></a>
### Motor id definition :

- motorR  - 0
- motorL  - 1
- motorB  - 2

<a name="line-547"></a>
### Motor range :

- motorStop                   - 0
- motorQuarter                - 256
- motorHalfmotorThreeQuarter  - 716
- motorFull                   - 1023

<a name="line-565"></a>
## Motors API Functions

<a name="line-569"></a><a name="pogobot_motor_set"></a>
### :arrow_right: pogobot_motor_set

```cpp
void pogobot_motor_set( motor_id motor, uint16_t value ) /* line 580 */
```

set the value of pwm that commands the motor

#### Parameters
- 'motor' - is the id of the motor you want to command
- 'value' - is PWM ratio apply to the motor (between 0 and 1023)

#### Return
- none


<a name="line-582"></a>
## Helper API

<a name="line-586"></a><a name="pogobot_helper_getid"></a>
### :arrow_right: pogobot_helper_getid

```cpp
uint16_t pogobot_helper_getid( void ) /* line 596 */
```

gives an unique identifier or an random number if no id is register inside the robot

#### Parameters
- none

#### Return
Returns an unique id on 16bits


<a name="line-598"></a><a name="pogobot_helper_getRandSeed"></a>
### :arrow_right: pogobot_helper_getRandSeed

```cpp
int16_t pogobot_helper_getRandSeed( void ) /* line 608 */
```

gives an seed base on the ADC read of the battery

#### Parameters
- none

#### Return
Returns an random id on 16bits

