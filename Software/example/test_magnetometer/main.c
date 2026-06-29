/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include <stdio.h>
#include <stdint.h>
#include "pogobot.h"
#include "magnetometer.h"

int main(void) 
{

    /* Read LIS2MDL magnetometer X,Y,Z axis*/

    int16_t mag_x = 0;
    int16_t mag_y = 0;
    int16_t mag_z = 0;  

    pogobot_init();

    if (magn_check() != 0) {
        printf("Erreur : Magnetometre non detecte ou erreur SPI.\n");
        return 1;
    }
    
    while (1) {
        
        pogobot_led_setColor(0, 0, 255);
        
        if (magn_read_XYZ(&mag_x, &mag_y, &mag_z, 10) == 0) {
            
            printf("%d %d %d\n", mag_x, mag_y, mag_z);
            mag_x = 0; 
            mag_y = 0; 
            mag_z = 0;
        }

        pogobot_led_setColor(0, 255, 0);

        msleep(20);
    }   

    return 0;
}