/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
 * Pogobot demo source code D2.8 BIS.
 * This file implements "D2.8 BIS Testing photosensor : phototaxis".
 * 
 * It shows how short a code can be thanks to the API design.
 * 
 * It exercises the following features: RGB LED, photosensor API.
 *
 * Details:
 * - Assumes: three photosensors, front-right (FR), front-left (FL), back (B)
 * - Robot A continuously measures light intensity from its three light sensors
 *   and stop when the light level is other a threashold
 * - Robot A’s LED is described in D2.7
 *
 * Testing protocol: try from different starting location (facing or not the
 * light, close to afar)
 *
 **/

/* clang-format-ok */

#include <pogobot.h>


#define THRESHOLD 150


int 
main(void) {

    pogobot_init();

    printf("init ok\n");

    printf("Chosen threshold %u \n", THRESHOLD);

    while (1)
    {    
        int i;
        int16_t max_val = 0;
        int8_t winner = 0;

        // mesurements
        for ( i = 0; i < 3; i++ )
        {
            int16_t tmp_val = pogobot_photosensors_read( i );
            if ( tmp_val > max_val )
            {
                max_val = tmp_val;
                winner = i;
            }
            printf( " %d : %d; ", i, tmp_val );
        }
        printf( "\n" );

        // *2 and saturation for a better visualisation
        max_val <<= 1;
        if ( max_val >= 255 )
        {
            max_val = 255;
        }

        // Led commands
        if ( winner == 0 && max_val > 0 )
        {
            pogobot_led_setColor( max_val, 0, 0 );
        }
        else if ( winner == 1 && max_val > 0 )
        {
            pogobot_led_setColor( 0, 0, max_val );
        }
        else if ( winner == 2 && max_val > 0 )
        {
            pogobot_led_setColor( 0, max_val, 0 );
        }
        else
        {
            pogobot_led_setColor( 0, 0, 0 );
        }

        if ( max_val < THRESHOLD )
        {
            printf( "moving!\n" );
            pogobot_motor_set( motorR, motorFull );
            pogobot_motor_set( motorL, motorFull );
        }
        else
        {
            printf( "stop\n" );
            pogobot_motor_set( motorR, motorStop );
            pogobot_motor_set( motorL, motorStop );
        }

        // delay
        msleep( 250 );

    }

}
