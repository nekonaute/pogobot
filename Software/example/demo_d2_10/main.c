/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
 * Pogobot demo source code D2.10.
 * This file implements "D2.10 Testing locomotion: go-forwardDescription".
 * 
 * It shows how short a code can be thanks to the API design.
 * 
 * It exercises the following features: RGB LED, photosensor API.
 *
 * Details:
 * Robot A goes forward, sinuisoidal translational velocity of period 4 sec
 * (MotorR = MotorL = from 0 to 1 and back)
 * Robot A’s RBG LED is always white/black
 *
 * Testing protocol: Robot A is placed in the arena, and observed for 30 seconds
 *
 **/


#include "pogobot.h"
#include "math.h"

#define PI    3.141592
#define DELAY 250
#define FREQ  0.25 // periods of 4 seconds

// float val = 0.5 * sinf( 2 * PI * FREQ * time ) + 0.5;
#define NB_VALUE 16
float sin_value[NB_VALUE] = { 1.000000, 0.961940, 0.853554, 0.691343, 
                              0.500001, 0.308659, 0.146448, 0.038061, 
                              0.000000, 0.038060, 0.146446, 0.308657, 
                              0.499999, 0.691341, 0.853552, 0.961939 };


int main(void) {

    pogobot_init();
    pogobot_led_setColor( 22, 22, 22 );

    printf("init ok\n");

    uint8_t tab_index = 0;

    while (1)
    {
        float val = sin_value[tab_index]*motorFull;
        pogobot_motor_set( motorR, val );
        pogobot_motor_set( motorL, val );

        // index 
        tab_index = (tab_index +1)%NB_VALUE;
        printf("tab_index %d, val moteur %d\n", tab_index, (int)val);

        // delay
        msleep( DELAY );
    }
}
