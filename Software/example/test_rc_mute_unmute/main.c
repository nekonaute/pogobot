/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

/* clang-format-ok */

#include <stdio.h>
#include <string.h>
#include "pogobot.h"

uint8_t data[] = "hello";

int main(void) {

    pogobot_init();

    while(1){
    
        pogobot_infrared_sendLongMessage_omniGen(data, 6);

        pogobot_infrared_update();

        if(is_muted())
        {
            pogobot_led_setColor(255,0,0);
            printf("Mute !\n");
        }
        else
        {
            pogobot_led_setColor(0,255,0);
            printf("Unmute !\n");
        }

        msleep(500);

    }	

}
