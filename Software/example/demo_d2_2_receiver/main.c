/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.2, receiver side.
The code for the sender is the same as the d2.1

This file implements the receiver side of demo "D2.2 Testing
communication: extroverted listener".

It shows how short a code can be thanks to the API design.

It exercises the following features: RGB LED, low-level infrared
transmission API.

Details:

Robot B continuously listen in each direction.
- for each message, change RGB LED according to which of its own IR received a
message:
    - Robot A’s front: red
    - Robot A’s left: green
    - Robot A’s back: blue
    - Robot A’s right: yellow
    - No message: purple/black

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

const rgb8_t colors[] = {
    { .r = 0x80, .g = 0x00, .b = 0x00 }, // front
    { .r = 0x00, .g = 0x80, .b = 0x00 }, // left
    { .r = 0x00, .g = 0x00, .b = 0x80 }, // back
    { .r = 0x80, .g = 0x80, .b = 0x00 }, // right
    { .r = 0x00, .g = 0x80, .b = 0x80 }, // none
};


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
                message_t mr;
                pogobot_infrared_recover_next_message( &mr );

                int color_index = mr.header._sender_ir_index;
                const rgb8_t *const color = &( colors[color_index] );

                pogobot_led_setColor( color->r, color->g, color->b );
                printf( "RECV: receiver %d, on ir %d, sender %d on ir %d ",
                        mr.header.receiver_id, mr.header._receiver_ir_index,
                        mr.header._sender_id, mr.header._sender_ir_index );
                printf( "RECV: len %d [%s]\n", mr.header.payload_length,
                        mr.payload );
            }
        }
        else
        {
            const rgb8_t *const color = &( colors[4] );
            pogobot_led_setColor( color->r, color->g, color->b );
        }
    }

}
