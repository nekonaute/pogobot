/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.6.

This file implements "D2.6 Testing communication : dialogue".

It shows how short a code can be thanks to the API design.

It exercises the following features: RGB LED, low-level infrared
transmission API.

Details:

- Two robots (A and B) are not moving and located next to one another (close
    enough for IR to work)
- Robot A and robot B continuously emits a specific message in each direction
    [similar as D2.1 (except LED)]
- each message contains:
    - name of robot (“robot A” or “robot B”)
    - name of IR emitter (“front”, “back”, “left”, “right”)
    - 26 letters (alphabet in lower case)
-Robot A and robot B continuously listen in each direction.
    - for each message, change RGB LED according to which of the other’s robot
IR sent the message:
        - From other robot’s front IR: red
        - From other robot’s left IR: green
        - From other robot’s back IR: blue
        - From other robot’s right IR: yellow
        - No message: white/black

Testing protocol: move one robot by hand to test all combinations. Then, same
with the other robot.

*/

/* clang-format-ok */

#include <stdio.h>
#include <string.h>
#include "pogobot.h"

#define robot_name           "robot A"
#define alphabet             "abcdefghijklmnopqrstuvwyz"
#define message_length_bytes ( sizeof( robot_name "front" alphabet ) )

unsigned char *messages[message_length_bytes] = {
    (unsigned char *)robot_name "front" alphabet,
    (unsigned char *)robot_name "right" alphabet,
    (unsigned char *)robot_name "back_" alphabet,
    (unsigned char *)robot_name "left_" alphabet,
};

typedef struct rgb8_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb8_t;

const rgb8_t colors[] = {
    { .r = 0x80, .g = 0x00, .b = 0x00 }, // front
    { .r = 0x00, .g = 0x80, .b = 0x00 }, // right
    { .r = 0x00, .g = 0x00, .b = 0x80 }, // back
    { .r = 0x80, .g = 0x80, .b = 0x00 }, // left
    { .r = 0x80, .g = 0x80, .b = 0x80 }, // none
};


int main(void) {

    pogobot_init();

    printf("init ok\n");

    pogobot_led_setColor( 25, 25, 25 );

    while (1)
    {
        pogobot_infrared_update();

        static int ir_emitter = 0;

        printf( "TRANS: %d, %d\n", ir_emitter, message_length_bytes );
        printf( "TRANS: %s\n", messages[ir_emitter] );

        pogobot_infrared_sendMessageOneDirection(
            ir_emitter, 0x1234, messages[ir_emitter], message_length_bytes );

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

        ir_emitter = ( ir_emitter + 1 ) % IR_RX_COUNT;

        msleep( 200 );
    }

}
