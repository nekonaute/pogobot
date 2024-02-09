/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.3.

This file implements " D2.3 Testing communication : train-a-robot".

It shows how short a code can be thanks to the API design.

It exercises the following features: RGB LED API, low-level infrared
transmission API, stopwatch API.

Details:

- Two robots (A and B) are not moving and located next to one another (close
enough for IR to work)
- Robot A continuously emits one specific message in all directions (~broadcast)
    - the content of the message changes every two seconds
    - a message contains:
        -name of robot (“robot A”)
        -a RGB value out of #FF0000, #00FF00, #0000FF, #FF00FF, #FFFF00, #00FFFF
    -RBG LED matches the message’s RGB values (i.e. also changes every two
seconds) -Robot B continuously listen in each direction. -when a message is
received, change its RGB LED to match the content of the message -RGB LED
switches off (white/black) after 0.1 second.

Testing protocol: move Robot B by hand to test all combinations, including “no
message”.

*/

/* clang-format-ok */


#include <stdio.h>
#include <string.h>
#include "pogobot.h"


#define robot_name "robot A"
#define COLOR_TIME 2000000 // us == 2s

typedef struct rgb8_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb8_t;

typedef struct my_message_t
{
    char name[8];
    rgb8_t color;
} my_message_t;

// #FF0000, #00FF00, #0000FF, #FF00FF, #FFFF00, #00FFFF
#define NB_COLOR 6
rgb8_t my_color[NB_COLOR] = {
    { .r = 0xFF, .g = 0x00, .b = 0x00 }, { .r = 0x00, .g = 0xFF, .b = 0x00 },
    { .r = 0x00, .g = 0x00, .b = 0xFF }, { .r = 0xFF, .g = 0x00, .b = 0xFF },
    { .r = 0xFF, .g = 0xFF, .b = 0x00 }, { .r = 0x00, .g = 0xFF, .b = 0xFF } };

time_reference_t my_timer;

void echo_cancel_disable( void );
void echo_cancel_disable( void )
{
  for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ )
  {
      uint32_t value = IRn_rx_conf_read( ir_i );
      value = ir_rx0_conf_echo_cancel_replace( value, 0 );
      IRn_rx_conf_write( ir_i, value );
  }
}

int main(void) {

    pogobot_init();

    // echo cancel
    echo_cancel_disable();

    pogobot_stopwatch_reset( &my_timer );
    printf("init ok\n");


    while (1)
    {
        pogobot_infrared_update();

        static int counter = 0;

        if ( pogobot_stopwatch_get_elapsed_microseconds( &my_timer ) >= COLOR_TIME )
        {
            pogobot_stopwatch_offset_origin_microseconds( &my_timer, COLOR_TIME );
            counter = ( counter + 1 ) % NB_COLOR;
            printf( " counter : %d \n", counter );
        }

        my_message_t my_mes = { .name = robot_name };
        my_mes.color = my_color[counter];

        pogobot_infrared_sendLongMessage_omniGen( (uint8_t *)( &my_mes ),
                                                  sizeof( my_message_t ) );

        /* read reception fifo buffer */
        if ( pogobot_infrared_message_available() )
        {
            while ( pogobot_infrared_message_available() )
            {
                message_t mr;
                pogobot_infrared_recover_next_message( &mr );

                if ( mr.header.payload_length != sizeof( my_message_t ) )
                {
                    printf( "ERROR : payload size not right ! \n" );
                }
                else
                {
                    my_message_t *recept = (my_message_t*)(&( mr.payload ));
                    // printf(" message from : %s\n", recept->name);
                    pogobot_led_setColor( recept->color.r, recept->color.g,
                                        recept->color.b );
                    msleep( 100 );
                    pogobot_led_setColor( 0, 0, 0 );
                }
            }
        }
        else
        {
            pogobot_led_setColor( 0, 0, 0 );
        }
    }
}
