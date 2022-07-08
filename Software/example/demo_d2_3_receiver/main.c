/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.3. (receiver code)

This file implements " D2.3 Testing communication : train-a-robot".

It shows how short a code can be thanks to the API design.

It exercises the following features: RGB LED API, low-level infrared
transmission API, stopwatch API.

Details:

- Two robots (A and B) are not moving and located next to one another (close
enough for IR to work)
- Robot B continuously listen in each direction.
    - when a message is received, change its RGB LED to match the content of the
message
    - RGB LED switches off (white/black) after 0.1 second.

Testing protocol: move Robot B by hand to test all combinations, including “no
message”.

*/

/* clang-format-ok */

#include <stdio.h>
#include <string.h>
#include "pogobot.h"


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


int main(void) {

    pogobot_init();

    printf("init ok\n");

    pogobot_led_setColor( 25, 25, 25 );

    while (1)
    {
        pogobot_infrared_update();

        /* read reception fifo buffer */
        if ( pogobot_infrared_message_available() )
        {
            while ( pogobot_infrared_message_available() )
            {
                printf( "You've got a new message ! \n" );
                message_t mr;
                pogobot_infrared_recover_next_message( &mr );

                if ( mr.header.payload_length != sizeof( my_message_t ) )
                {
                    printf( "ERROR : payload size not right ! \n" );
                }
                else
                {
                    my_message_t *recept = &( mr.payload );
                    printf( "message from : %s\n", recept->name );
                    printf( "color (%d,%d,%d)\n", recept->color.r, recept->color.g,
                            recept->color.b );
                    pogobot_led_setColor( recept->color.r, recept->color.g,
                                        recept->color.b );
                    msleep( 100 );
                    pogobot_led_setColor( 0, 0, 0 );
                }
            }
        }
        else
        {
            pogobot_led_setColor( 0, 0, 0 );
        }
    }

}
