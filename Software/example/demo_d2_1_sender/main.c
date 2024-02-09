/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.1, sender side.

This file implements the sender side of demo "D2.1 Testing
communication: introverted listener".

It shows how short a code can be thanks to the API design.

It exercises the following features: RGB LED, low-level infrared
transmission API.

Details:

Robot A continuously emits a specific message in each direction
- each message contains:
  - name of robot (“robot A”)
  - name of IR emitter (“front”, “back”, “left”, “right”)
  - 26 letters (alphabet in lower case)
- RBG LED is always white/black

 */

/* clang-format-ok */

#include <stdio.h>
#include <string.h>
#include "pogobot.h"


#define robot_name           "robot A"
#define alphabet             "abcdefghijklmnopqrstuvwyz"
#define message_length_bytes ( sizeof( robot_name "front" alphabet ) )

unsigned char *messages[4] = {
    (unsigned char *)robot_name "front" alphabet,
    (unsigned char *)robot_name "right" alphabet,
    (unsigned char *)robot_name "back_" alphabet,
    (unsigned char *)robot_name "left_" alphabet,
};


int main(void) {

    pogobot_init();

    printf("init ok\n");

    pogobot_led_setColor( 25, 25, 25 );

    // set to max
    pogobot_infrared_set_power(pogobot_infrared_emitter_power_twoThird);

    while (1)
    {
        pogobot_infrared_update();

        static int ir_emitter = 0;

        uint8_t l = ( ir_emitter == 0 ) ? 0 : 25;
        pogobot_led_setColor( l, l, l );

        printf( "TRANS: %d, %d ", ir_emitter, message_length_bytes );
        printf( "%s\n", messages[ir_emitter] );

        pogobot_infrared_sendLongMessage_uniSpe(
            ir_emitter, messages[ir_emitter], message_length_bytes );

        ir_emitter = ( ir_emitter + 1 ) % IR_RX_COUNT;
        msleep( 500 );
    }

}
