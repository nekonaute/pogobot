/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include "pogobot.h"


int main(void) {

    pogobot_init();

    printf("init ok\n");

    while (1)
    {
      pogobot_led_setColor(0,0,255);
      pogobot_motor_set(motorL, motorFull);
      pogobot_motor_set(motorR, motorStop);
      msleep(500);

      printf(" HELLO WORLD !!! \n");

      pogobot_led_setColor(255,0,0);
      pogobot_motor_set(motorL, motorStop);
      pogobot_motor_set(motorR, motorFull);
      msleep(500);
    }

}
