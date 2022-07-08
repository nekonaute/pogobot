/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.3. (sender code)

This file implements " D2.3 Testing communication : train-a-robot".

It shows how short a code can be thanks to the API design.

It exercises the following features: RGB LED API, low-level infrared
transmission API, stopwatch API.

Details:

- Two robots (A and B) are not moving and located next to one another (close
enough for IR to work)
- Robot A continuously emits one specific message in all directions (~broadcast)
    - the content of the message changes every two seconds
    - a message contains:
        -name of robot (“robot A”)
        -a RGB value out of #FF0000, #00FF00, #0000FF, #FF00FF, #FFFF00, #00FFFF
    -RBG LED matches the message’s RGB values (i.e. also changes every two
seconds)


Testing protocol: move Robot B by hand to test all combinations, including “no
message”.

*/

/* clang-format-ok */

#include <stdio.h>
#include <string.h>
#include "pogobot.h"

#define robot_name "robot A"
#define COLOR_TIME 2000000 // us == 2s

typedef struct rgb8_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb8_t;

typedef struct my_message_t
{
    char name[8];
    rgb8_t color;
} my_message_t;

// #FF0000, #00FF00, #0000FF, #FF00FF, #FFFF00, #00FFFF
#define NB_COLOR 6
rgb8_t my_color[NB_COLOR] = {
    { .r = 0xFF, .g = 0x00, .b = 0x00 }, { .r = 0x00, .g = 0xFF, .b = 0x00 },
    { .r = 0x00, .g = 0x00, .b = 0xFF }, { .r = 0xFF, .g = 0x00, .b = 0xFF },
    { .r = 0xFF, .g = 0xFF, .b = 0x00 }, { .r = 0x00, .g = 0xFF, .b = 0xFF } };

time_reference_t my_timer;


int main(void) {

    pogobot_init();

    pogobot_stopwatch_reset( &my_timer );

    printf("init ok\n");

    pogobot_led_setColor( 25, 25, 25 );

    while (1)
    {
        pogobot_infrared_update();

        static int counter = 0;

        if ( pogobot_stopwatch_get_elapsed_microseconds( &my_timer ) >= COLOR_TIME )
        {
            pogobot_stopwatch_offset_origin_microseconds( &my_timer, COLOR_TIME );
            counter = ( counter + 1 ) % NB_COLOR;
            printf( " counter : %d \n", counter );
        }

        uint8_t l = ( counter == 0 ) ? 0 : 255;
        pogobot_led_setColor( l, l, l );

        my_message_t my_mes = { .name = robot_name };
        my_mes.color = my_color[counter];

        pogobot_infrared_sendMessageAllDirection( 0xBEEF, (uint8_t *)( &my_mes ),
                                                sizeof( my_message_t ) );

        msleep( 200 );
    }

}
