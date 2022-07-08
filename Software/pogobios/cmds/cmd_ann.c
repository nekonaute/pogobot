/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/* clang-format-ok */

#include <crc.h>
#include <generated/csr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system.h>
#include <uart.h>

#include <command.h>
#include <helpers.h>
#include <superbot_neural_network.h>
//#include <sim_debug.h>

#define xstr( a ) str( a )
#define str( a )  #a

/**
 * Command "speed_test_ann>
 *
 * Benchmark Artificial Neural Network
 *
 */
static void
ann_handler( int nb_params, char **params )
{

    printf( "\nSample random number=%d.\n", rand() );

    printf( "\nInit neural net step matrix.\n" );
    neural_network_matrix_populate();

    /* init timer */
    timer0_en_write( 0 );
    timer0_reload_write( 0 );
    timer0_load_write( 0xffffffff );
    timer0_en_write( 1 );

    timer0_update_value_write( 1 );

    uint32_t start = timer0_value_read();
	uint32_t it=12;
    printf( "Starting %ld iterations...\n", it );
    for ( int i = 0; i < it; i++ )
    {
        neural_network_input_populate();
        compute_one_neural_network_step();
    }
    timer0_update_value_write( 1 );
    uint32_t end = timer0_value_read();
    printf( "...the end\n" );

    printf( "start: %lu, end: %lu\n", start, end );
    uint32_t time_microseconds =
        ( start - end ) / ( CONFIG_CLOCK_FREQUENCY / 1000000L );
    printf( "Elapsed time for i iterations : %lu, per iteration: %lu\n",
            time_microseconds, time_microseconds / it );
}
define_command( ann, ann_handler, "Benchmark ANN.", POGO_CMDS );
