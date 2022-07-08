/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/** \file
Pogobot demo source code D2.9.

This file implements "D2.9 Testing communication and computation: a distributed
genetic algorithm".

It shows how short a code can be thanks to the API design.

It exercises the following features: RGB LED, low-level infrared
transmission API.

Details:

- (At least) three robots (A, B, C) are not moving and located next to one
    another (close enough for IR to work, each robot has at least one neighbour
in its vicinity).
- Each robot executes the following pseudo-code
    - my.genome = {1|0}X # i.e. random sequence of 0 and 1, length is X, with
        X=12 by default
    - my.fitness = count the number of 1 in my.genome
    - setLED (my.genome) # X binary values => mapped to a white/black LED
        sequence ({1|0}X => {white|black}X) # LED duration: 1/X sec per bit #
breathing when X bits at 1
    - while (True):
        - send_though_IR( my.genome, my.fitness ) # broadcast
        - first_message = listen_IR() # get first message in list
        - if first_message.fitness > my.fitness:
            - my.genome = first_message.genome
        - elif random() < Pmutation:         # Pmutation = 0.1 by default
            - my.genome = my.genome with exactly one bit flipped
        - my.fitness = count the number of 1 in genome
        - setLED (my.genome) # X binary values
        - wait for 0.1 sec

Testing protocol: robots are placed next to each other, we expect they all
    converge to displaying a white LED

*/

/* clang-format-ok */

#include "pogobot.h"


#define GENOME_SIZE 12
#define LED_TIME    ( 1000 / GENOME_SIZE ) // ms
#define PMUTATION   10

typedef struct my_data_t
{
    uint16_t genome;
    uint16_t fitness;
} my_data_t;

my_data_t my_data = { .genome = 0, .fitness = 0 };

void set_led( uint16_t c );
void set_led( uint16_t c )
{
    unsigned int i1 = 1 << ( GENOME_SIZE - 1 );
    for ( ; i1; i1 >>= 1 )
    {
        if ( ( c & i1 ) != 0 )
        {
            pogobot_led_setColor( 25, 25, 25 );
        }
        else
        {
            pogobot_led_setColor( 0, 0, 0 );
        }
        msleep( LED_TIME );
    }
}

uint16_t compute_fitness( uint16_t c );
uint16_t compute_fitness( uint16_t c )
{
    uint16_t count = 0;
    unsigned int i1 = 1 << ( GENOME_SIZE - 1 );
    for ( ; i1; i1 >>= 1 )
    {
        if ( ( c & i1 ) != 0 )
        {
            count++;
        }
    }
    return count;
}

void print_binary( uint16_t c );
void print_binary( uint16_t c )
{
    uint16_t i1 = ( 1 << ( sizeof( c ) * 8 - 1 ) );
    for ( ; i1; i1 >>= 1 )
    {
        printf( "%d", ( c & i1 ) != 0 );
    }
    printf( "\n" );
}

void drawANewGenome( my_data_t *my_data, int size );
void drawANewGenome( my_data_t *my_data, int size )
{
    int i = 0;
    my_data->genome = 0x0;
    for ( i = 0; i < size; i++ )
    {
        uint8_t bit = rand() % 2; // 2 states {0|1}
        my_data->genome |= bit << i;
    }

    printf( " new genome : " );
    print_binary( my_data->genome );
    printf( "\n" );
}

void flip_one_single_bit( my_data_t *my_data, int size );
void flip_one_single_bit( my_data_t *my_data, int size )
{
    int n = rand() % size;
    my_data->genome ^= 1UL << n;
    printf( "flip %d nth bit\n", n );
}

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

    //echo_cancel_disable();

    /* initialize random */
    srand( pogobot_helper_getRandSeed() );
    drawANewGenome( &my_data, GENOME_SIZE );
    my_data.fitness = compute_fitness( my_data.genome );
    set_led( my_data.genome );

    printf("init ok\n");

    while (1)
    {
        pogobot_infrared_update();

        pogobot_infrared_sendMessageAllDirection( 0x1234, (uint8_t *)( &my_data ),
                                                  sizeof( my_data ) );

        // take the first message
        if ( pogobot_infrared_message_available() )
        {
            printf( "You have a new message\n" );
            message_t mr;
            pogobot_infrared_recover_next_message( &mr );
            my_data_t *recept = (my_data_t *)(&( mr.payload ));
            if ( recept->fitness > my_data.fitness )
            {
                printf( "copy genome\n" );
                my_data.genome = recept->genome;
                print_binary( recept->genome );
            }
            else if ( rand() % 100 < PMUTATION )
            {
                printf( "mutation\n" );
                flip_one_single_bit( &my_data, GENOME_SIZE );
                print_binary( my_data.genome );
            }
            else
            {
                printf( "do nothing\n" );
            }
        }
        // clean queue
        pogobot_infrared_clear_message_queue();

        my_data.fitness = compute_fitness( my_data.genome );
        set_led( my_data.genome );

        printf( "my new fitness [%d]\n", my_data.fitness );

        msleep( 100 ); // 0.1 sec
    }

}
