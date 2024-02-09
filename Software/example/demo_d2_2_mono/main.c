/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.2, one-robot version.

This file implements the one-robot version of demo "D2.2 Testing
communication: extroverted listener".

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

Robot B continuously listen in each direction.
- for each message, change RGB LED according to which of its own IR received a
message:
    - Robot A’s front: red
    - Robot A’s left: green
    - Robot A’s back: blue
    - Robot A’s right: yellow
    - No message: purple/black

*/

/* clang-format-ok */

#include <stdio.h>
#include <string.h>
#include "pogobot.h"


#define SIZE_TEST 10

#define robot_name           "robot A"
#define alphabet             "abcdefghijklmnopqrstuvwyz"
#define message_length_bytes ( sizeof( robot_name "front" alphabet ) )

unsigned char *messages[4] = {
    (unsigned char *)robot_name "front" alphabet,
    (unsigned char *)robot_name "right" alphabet,
    (unsigned char *)robot_name "back_" alphabet,
    (unsigned char *)robot_name "left_" alphabet,
};

typedef struct rgb8_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb8_t;

const rgb8_t colors[] = {
    { .r = 0x80, .g = 0x00, .b = 0x00 }, // front
    { .r = 0x00, .g = 0x80, .b = 0x00 }, // left
    { .r = 0x00, .g = 0x00, .b = 0x80 }, // back
    { .r = 0x80, .g = 0x80, .b = 0x00 }, // right
    { .r = 0x00, .g = 0x80, .b = 0x80 }, // none
};

time_reference_t mytimer;

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

    printf("init ok\n");

    pogobot_led_setColor( 25, 25, 25 );

    while (1)
    {
        pogobot_infrared_update();

        static int counter = 0;
        static int ir_emitter = 0;

        if ( counter < SIZE_TEST )
        {
          printf( "TRANS: %d, %d ", ir_emitter, message_length_bytes );
          printf( "%s\n", messages[ir_emitter] );
          pogobot_infrared_sendLongMessage_uniSpe(
              ir_emitter, messages[ir_emitter], message_length_bytes );
          counter = counter + 1;
        }

        /* read reception fifo buffer */
        if ( pogobot_infrared_message_available() )
        {
            while ( pogobot_infrared_message_available() )
            {
                message_t mr;
                pogobot_infrared_recover_next_message( &mr );

                int color_index = mr.header._sender_ir_index;
                const rgb8_t *const color = &( colors[color_index] );

                pogobot_led_setColor( color->r, color->g, color->b );
                printf( "RECV: receiver on ir %d, sender %d on ir %d ",
                        mr.header._receiver_ir_index,
                        mr.header._sender_id, mr.header._sender_ir_index );
                printf( "RECV: len %d [%s]\n", mr.header.payload_length,
                        mr.payload );
            }
        }
        else
        {
            const rgb8_t *const color = &( colors[4] );
            pogobot_led_setColor( color->r, color->g, color->b );
        }

        /*if ( counter == SIZE_TEST )
        {
            uint32_t time_microseconds =
                pogobot_stopwatch_get_elapsed_microseconds( &mytimer );
            printf( "\nSent %u %u bytes in %lu microseconds.\n", SIZE_TEST,
                    message_length_bytes, time_microseconds );
            uint64_t numerator =
                ( (uint64_t)SIZE_TEST * message_length_bytes * 1000000L );
            uint64_t denominator = ( time_microseconds );
            uint32_t ir_speed = numerator / denominator;
            printf( "IR comm speed:  %luB/s\n", ir_speed );

            // end of the test
            while ( 1 )
            {
            }
        }
        else
        {
            counter++;
        }*/

        ir_emitter = ( ir_emitter + 1 ) % IR_RX_COUNT;
    }

}
