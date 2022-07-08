/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
 * Pogobot demo source code D2.7.
 * This file implements "D2.7 Testing photosensor : distance to lightDescription".
 * 
 * It shows how short a code can be thanks to the API design.
 * 
 * It exercises the following features: RGB LED, photosensor API.
 *
 * Details:
 *
 * Robot A is not moving and located next to a light source.
 * The light source (or the robot) will be moved by hand
 * Robot A continuously measures light intensity from its three light sensors :
 *      - the RGB LED is updated continuously according to light intensity and
 *          direction (winner-takes-all)
 *      - Robot A’s photosensor #1 is the most stimulated: LED is red
 *      - Robot A’s photosensor #2 is the most stimulated: LED is blue
 *      - Robot A’s photosensor #3 is the most stimulated: LED is green
 *      - => depending on the intensity perceived, the color is dimmed
 *          (continuous levels from #00 to #FF)
 *      - LED is black if no light
 *
 * Testing protocol: move Robot A by hand to test all combinations, including no
 *  light.
 *
 **/

/* clang-format-ok */

#include "pogobot.h"


int main(void) {

    pogobot_init();

    printf("init ok\n");

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

        // delay
        msleep( 250 );
    }
}
