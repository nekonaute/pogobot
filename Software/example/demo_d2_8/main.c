/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
 * Pogobot demo source code D2.8.
 * This file implements "D2.8 Testing photosensor : phototaxis".
 * 
 * It shows how short a code can be thanks to the API design.
 * 
 * It exercises the following features: RGB LED, photosensor API.
 *
 * Details:
 * - Assumes: three photosensors, front-right (FR), front-left (FL), back (B)
 * - Robot A continuously measures light intensity from its three light sensors
 *   and follow the light
 *     - Tentative implementation (hysteresis for turning might
 *       be useful):
 *          - if max(FR,FL,B) == max( FR, FL )  && abs(FR-FL ) < threshold:
 *              motorR = motorL = 1.0
 *          - elif max(FR,FL,B) == FR: motorL = 0.7, motorR = 0.3
 *          - elif max(FR,FL,B) == FL: motorR = 0.7, motorL = 0.3
 *          - else: motorR = 1.0, motorL = 0.0
 *   -Robot A’s LED is described in D2.7
 *
 * Testing protocol: try from different starting location (facing or not the
 * light, close to afar)
 *
 **/

/* clang-format-ok */

#include <pogobot.h>


#define THRESHOLD 50

uint16_t max( uint16_t a, uint16_t b );
uint16_t max( uint16_t a, uint16_t b )
{
    return ( a < b ) ? b : a;
}

uint16_t max3( uint16_t a, uint16_t b, uint16_t c );
uint16_t max3( uint16_t a, uint16_t b, uint16_t c )
{
    uint16_t d = ( a < b ) ? b : a;
    return ( d < c ) ? c : d;
}

int 
main(void) {

    pogobot_init();

    printf("init ok\n");

    while (1)
    {    
        int i;
        int16_t max_val = 0;
        int8_t winner = 0;

        int16_t val[3];

        // mesurements
        for ( i = 0; i < 3; i++ )
        {
            int16_t tmp_val = pogobot_photosensors_read( i );
            val[i] = tmp_val;
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

        if ( max3( val[p_FR], val[p_FL], val[p_B] ) ==
                max( val[p_FR], val[p_FL] ) &&
                abs( val[p_FR] - val[p_FL] ) < THRESHOLD )
        {
            // motorR = motorL = 1.0
            printf( "light front\n" );
            pogobot_motor_set( motorR, motorFull );
            pogobot_motor_set( motorL, motorFull );
        }
        else if ( max3( val[p_FR], val[p_FL], val[p_B] ) == val[p_FR] )
        {
            // motorR = 0.3, motorL = 0.7
            printf( "light right\n" );
            pogobot_motor_set( motorR, motorQuarter );
            pogobot_motor_set( motorL, motorThreeQuarter );
        }
        else if ( max3( val[p_FR], val[p_FL], val[p_B] ) == val[p_FL] )
        {
            // motorR = 0.7, motorL = 0.3
            printf( "light left\n" );
            pogobot_motor_set( motorR, motorThreeQuarter );
            pogobot_motor_set( motorL, motorQuarter );
        }
        else
        {
            // motorR = 1.0, motorL = 0.0
            printf( "light back\n" );
            pogobot_motor_set( motorR, motorFull );
            pogobot_motor_set( motorL, motorStop );
        }

        // delay
        msleep( 250 );

    }

}
