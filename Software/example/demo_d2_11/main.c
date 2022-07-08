/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
 *
 * Pogobot demo source code D2.11.
 * This file implements "D2.11 Testing locomotion: circling around".
 * 
 * It shows how short a code can be thanks to the API design.
 * 
 * It exercises the following features: RGB LED, photosensor API.
 *
 * Details:
 * Robot A makes circles, leftward (motorR, motorL = 0.7,0.3) then rightward
 *(reverse) switches every 20 seconds. Robot A’s RBG LED is red when circling
 *leftward, blue when righward
 *
 * Testing protocol: Robot A is placed in the arena, and observed for 100
 *seconds
 *
 **/

/* clang-format-ok */

#include "pogobot.h"

time_reference_t mytimer;
uint32_t period_microseconds = 20000000;

int main(void) {

    pogobot_init();
    pogobot_led_setColor( 22, 22, 22 );

    pogobot_timer_init( &mytimer, period_microseconds );

    printf("init ok\n");

    while (1)
    {
        //printf("+ turn (%d)\n",  pogobot_timer_has_expired( &mytimer ) );

        static uint8_t way = 0;

        if ( pogobot_timer_has_expired( &mytimer ) )
        {
            pogobot_timer_offset_origin_microseconds( &mytimer,
                                                      period_microseconds );
            printf( " change way \n" );
            way = ~way;
        }

        if ( way )
        {
            pogobot_led_setColor( 255, 0, 0 );
            pogobot_motor_set( motorL, motorThreeQuarter );
            pogobot_motor_set( motorR, motorQuarter );
        }
        else
        {
            pogobot_led_setColor( 0, 0, 255 );
            pogobot_motor_set( motorR, motorQuarter );
            pogobot_motor_set( motorL, motorThreeQuarter );
        }

        // delay
        msleep( 250 );

    }
}
