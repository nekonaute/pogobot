/**
 * POGOBOT
 *
 * Copyright ¬© 2022 Sorbonne Universit√© ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/* clang-format-ok */

#include <crc.h>
#include <generated/csr.h>
#include <generated/soc.h>
#include <generated/mem.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <system.h>
#include <uart.h>

#include <command.h>
#include <helpers.h>

#ifdef CSR_IR_RX0_BASE

#include <pogobot.h>

#ifdef REMOCON
#include <ir_boot.h>
#include <boot.h>
#include <sfl.h>
#endif

#define xstr( a ) str( a )
#define str( a )  #a
//#define CLOCK_NS 1000000000/CONFIG_CLOCK_FREQUENCY
#define CLOCK_NS 48

/**
 *
 * Get config of TS4231
 *
 */
static void
ir_readConfig_handler( int nb_params, char **params )
{
    uint8_t i;
    uint16_t result;
    for( i=0; i<IR_RX_COUNT; i++) {
        result = ts_readConfig(i);
        printf("Config of IR %d is : 0x%04x\n", i, result);
    }
}
define_command( ir_readConfig, ir_readConfig_handler,
                "Get config of TS4231.", POGO_CMDS );
/**
 *
 * Get bus status of TS4231
 *
 */
static void
ir_checkBus_handler( int nb_params, char **params )
{
    uint8_t result, i;
    char result_in_text[][8] = {"S0", "SLEEP", "WATCH", "S3", "UNKNOWN"};
    for( i=0; i<IR_RX_COUNT; i++) {
        result = ts_checkBus(i);
        if(result <=5) {
            printf("IR %d guessed state : %s\n", i, result_in_text[result]);
        }
        else{
            printf("Error, result should never be > 5");
        }
    }
}
define_command( ir_checkBus, ir_checkBus_handler,
                "Get bus status of TS4231.", POGO_CMDS );
/**
 *
 * Set TS4231 to watch mode
 *
 */
static void
ir_watch_handler( int nb_params, char **params )
{
    uint8_t i;
    bool result;
    for( i=0; i<IR_RX_COUNT; i++) {
        result = ts_goToWatch(i);
        if(result) {
            printf("IR %d in watch state\n", i);
        }
        else{
            printf("Error switching IR %d to watch state\n", i);
        }
    }
}
define_command( ir_watch, ir_watch_handler,
                "Set TS4231 to watch mode.", POGO_CMDS );

/**
 * Command "ir_target_bit_mask [bit mask]"
 *
 * Set which IR are used
 *
 */
static void
ir_target_bit_mask_handler( int nb_params, char **params )
{
    char *c;

    printf( "Compiled for hardware with %d IR.\n", IR_RX_COUNT );

    if ( ( nb_params == 0 ) || ( nb_params > 1 ) )
    {
        printf( "ir_target_bit_mask [bit mask]\n" );
        if ( nb_params > 1 )
        {
            return;
        }
    }

    if ( nb_params >= 1 )
    {
        printf( "before: ir_target_bit_mask=0x%x\n", target_ir_bit_mask );
        target_ir_bit_mask =
            (uint32_t)strtoul( params[0], &c, 0 ) & IR_RX_BITMASK_ALL;
        if ( *c != 0 )
        {
            printf( "Incorrect target_ir_bit_mask value : %s\n", params[0] );
            return;
        }
        printf( "after: " );
    }

    printf( "target_ir_bit_mask=0x%x\n", target_ir_bit_mask );
}
define_command( ir_target_bit_mask, ir_target_bit_mask_handler,
                "Show/select which IRs are targeted by a command.", POGO_CMDS );


/**
 * Command "ir_init"
 *
 * Init the TS4231
 *
 */
static void
ir_init_handler( int nb_params, char **params )
{
    printf("ir_init [target_ir_bit_mask]\n");
    printf("Compiled for hardware with %d IR.\n", IR_RX_COUNT);
    char *c;
    uint8_t result;
    if ( nb_params >= 1 ) {
        target_ir_bit_mask = (uint32_t)strtoul(params[0], &c, 0);
        if ( *c != 0 ) {
            printf( "Incorrect target_ir_bit_mask value : %s\n",
                    params[1] );
            return;
        }
    }
    printf("Using target_ir_bit_mask=0x%x\n", target_ir_bit_mask);
    result = ir_init();
    for( uint8_t i=0; i<IR_RX_COUNT; i++) {
        printf("IR %d: ", i);
        if( (result & (1<<i)) == (1<<i) ) {
            printf("success\n");
        }
        else {
            printf("failure\n");
        }

    }
}

define_command( ir_init, ir_init_handler,
                "Infrared received (TS4231) initialization", POGO_CMDS);

/**
 * Command "ir_send"
 *
 * Send message through the infrared LEDs
 *
 */
static void
ir_send_handler( int nb_params, char **params )
{
    char *c;
    unsigned int *value;

    if ( ( nb_params == 0 ) || ( nb_params > 2 ) )
    {
        printf( "ir_send <message> [target_ir_bit_mask]\n" );
        if ( nb_params > 2 )
        {
            return;
        }
    }

    if ( nb_params >= 2 )
    {
        target_ir_bit_mask = (uint32_t)strtoul( params[1], &c, 0 );
        if ( *c != 0 )
        {
            printf( "Incorrect target_ir_bit_mask value : %s\n", params[1] );
            return;
        }
    }

    printf( "Using target_ir_bit_mask=0x%x\n", target_ir_bit_mask );

	value = (unsigned int *)strtoul(params[0], &c, 0);
    if (*c != 0) {
		char *msg = params[0];
		size_t msglen = strlen( msg );
		printf( "Sending message : %s, length : %d\n", msg, msglen );
		if( IRn_tx_write_msg(target_ir_bit_mask, (ir_uart_word_t *)msg, msglen) != 0 ) {
            printf("Error sending message");
        }
    }
	else {
		printf("Sending value : 0x%08x\n",(unsigned int)value);
		if( IRn_tx_write_msg(target_ir_bit_mask, (ir_uart_word_t *)(&value), sizeof(value)) ) {
            printf("Error sending message");
        }
	}

}

define_command( ir_send, ir_send_handler, "Send message through IR",
                POGO_CMDS );

/**
 * Command "ir_power <powerl_level>"
 *
 * Set power lever for the infrared LEDs
 *
 */
static void
ir_power_handler( int nb_params, char **params )
{
    char *c;
    unsigned int level=0;

    if ( ( nb_params == 0 ) || ( nb_params > 2 ) )
    {
        printf( "ir_power_handler <power_level> [target_ir_bit_mask]\n" );
        //if ( nb_params > 2 )
        {
            return;
        }
    }

    level = (unsigned int)strtoul( params[0], &c, 0 );
    if ( ( *c != 0 ) || ( level > 3 ) )
    {
        printf( "Incorrect value : %d\n", level );
        return;
    }

    if ( nb_params >= 2 )
    {
        target_ir_bit_mask = (uint32_t)strtoul( params[1], &c, 0 );
        if ( *c != 0 )
        {
            printf( "Incorrect target_ir_bit_mask value : %s\n", params[1] );
            return;
        }
    }

    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ )
    {
        if ( ( ( target_ir_bit_mask >> ir_i ) & 1 ) == 0 )
        {
            printf( "\nSkipping IR %d\n", ir_i );
            continue;
        }

        printf( "Setting power %d on IR %d\n", level, ir_i );
        IRn_conf_tx_power_write( ir_i, level );
    }
}
define_command( ir_power, ir_power_handler, "Set IR power level", POGO_CMDS );

#ifdef CSR_IR_RX0_CONF_RX_ZERO_OFFSET
/**
 * Command "ir_rx_zeroone [short_thres] [long_thresh]"
 *
 * Gets and/or set duration thresholds to distinguish zeroes and ones.
 *
 */
static void
ir_rx_zeroone_handler( int nb_params, char **params )
{
    char *c;

    if ( ( nb_params == 0 ) || ( nb_params > 2 ) )
    {
        printf( "ir_rx_zeroone [zero_length] [one_length]\n" );
        if ( nb_params > 2 )
        {
            return;
        }
    }
    printf( "Using target_ir_bit_mask=0x%x\n", target_ir_bit_mask );

    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ )
    {
        if ( ( ( target_ir_bit_mask >> ir_i ) & 1 ) == 0 )
        {
            printf( "\nSkipping IR %d\n", ir_i );
            continue;
        }
        printf( "\nProceeding on IR %d\n", ir_i );
        uint32_t value = IRn_rx_conf_read( ir_i );
        uint32_t zero = ir_rx0_conf_rx_zero_extract( value );
        printf( "Current zero threshold: %ld\n", zero );
        uint32_t one = ir_rx0_conf_rx_one_extract( value );
        printf( "Current one threshold: %ld\n", one );
        if ( nb_params > 0 )
        {
            zero = (uint32_t)strtoul( params[0], &c, 0 );
            if ( ( *c != 0 ) || ( zero > 31 ) ) // 5 bits value
            {
                printf( "Incorrect zero value : %ld\n", zero );
                return;
            }

            one = (uint32_t)strtoul( params[1], &c, 0 );
            if ( ( *c != 0 ) || ( one > 31 ) ) // 5 bits value
            {
                printf( "Incorrect one value : %ld\n", one );
                return;
            }
            printf( "New zero threshold: %ld\n", zero );
            printf( "New one threshold: %ld\n", one );
            value = ir_rx0_conf_rx_zero_replace( value, zero );
            value = ir_rx0_conf_rx_one_replace( value, one );
            // printf( "Writing:   0x%08x\n", value );
            IRn_rx_conf_write( ir_i, value );
        }
    }
}
define_command( ir_rx_zeroone, ir_rx_zeroone_handler,
                "Set IR duration threshold(s)", POGO_CMDS );
#endif // CSR_IR_RX0_CONF_RX_ZERO_OFFSET

#ifdef CSR_IR_TX_CONF_TX_ZERO_OFFSET
/**
 * Command "ir_tx_zeroone [zero_length] [one_length]"
 *
 * Gets and/or set duration of zero and ones during transmission.
 *
 */
static void
ir_tx_zeroone_handler( int nb_params, char **params )
{
    char *c;

    if ( nb_params > 2 )
    {
        printf( "ir_tx_zeroone [zero_length] [one_length]\n" );
        return;
    }
    uint32_t value = ir_tx_conf_read();
    uint32_t zero = ir_tx_conf_tx_zero_extract( value );
    printf( "Current zero length: %ld\n", zero );
    uint32_t one = ir_tx_conf_tx_one_extract( value );
    printf( "Current one length: %ld\n", one );
    if ( nb_params > 0 )
    {
       zero = (uint32_t)strtoul( params[0], &c, 0 );
       if ( ( *c != 0 ) || ( zero > 15 ) ) // 4 bits value
       {
           printf( "Incorrect zero value : %ld\n", zero );
           return;
       }

       one = (uint32_t)strtoul( params[1], &c, 0 );
       if ( ( *c != 0 ) || ( one > 15 ) ) // 4 bits value
       {
           printf( "Incorrect one value : %ld\n", one );
           return;
       }

       printf( "New zero length: %ld\n", zero );
       printf( "New one length: %ld\n", one );
       value = ir_tx_conf_tx_zero_replace( value, zero );
       value = ir_tx_conf_tx_one_replace( value, one );
       printf( "Writing:   0x%08lx\n", value );
       ir_tx_conf_write( value );
    }
}
define_command( ir_tx_zeroone, ir_tx_zeroone_handler,
                "Set TX duration of zeros and ones", POGO_CMDS );
#endif // CSR_IR_TX_CONF_TX_ZERO_OFFSET

/**
 * Command "ir_echo_cancel [bool] [target_ir_bit_mask]"
 *
 * Gets and/or set duration echo_cancel to distinguish zeroes, ones, and space.
 *
 */
static void
ir_echo_cancel_handler( int nb_params, char **params )
{
    char *c;

    if ( ( nb_params == 0 ) || ( nb_params > 2 ) )
    {
        printf( "ir_echo_cancel [value] [target_ir_bit_mask]\n" );
        if ( nb_params > 2 )
        {
            return;
        }
    }

    if ( nb_params >= 2 )
    {
        target_ir_bit_mask = (uint32_t)strtoul( params[1], &c, 0 );
        if ( *c != 0 )
        {
            printf( "Incorrect target_ir_bit_mask value : %s\n", params[1] );
            return;
        }
    }

    printf( "Using target_ir_bit_mask=0x%x\n", target_ir_bit_mask );

    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ )
    {
        if ( ( ( target_ir_bit_mask >> ir_i ) & 1 ) == 0 )
        {
            printf( "\nSkipping IR %d\n", ir_i );
            continue;
        }

        printf( "\nProceeding on IR %d\n", ir_i );

        uint32_t value = IRn_rx_conf_read( ir_i );

        //printf( "Just read: 0x%08x\n", value );

        uint32_t echo_cancel = ir_rx0_conf_echo_cancel_extract( value );

        printf( "Echo_Cancel before: %ld\n", echo_cancel );

        if ( nb_params > 0 )
        {
            echo_cancel = (uint32_t)strtoul( params[0], &c, 0 );
            if ( *c != 0 )
            {
                printf( "Incorrect echo_cancel value : %ld\n", echo_cancel );
                return;
            }
            value = ir_rx0_conf_echo_cancel_replace( value, echo_cancel );
        }

        printf( "Echo_Cancel after: %ld\n", echo_cancel );

        //printf( "Writing:   0x%08x\n", value );

        IRn_rx_conf_write( ir_i, value );
    }
}
define_command(
    ir_echo_cancel, ir_echo_cancel_handler,
    "Enable/disable IR echo cancellation (when active, shuts off IR reception "
    "while you emit, when disabled  you receive copy of any IR you send).",
    POGO_CMDS );

/**
 * Command "ir_read [target_ir_bit_mask]"
 *
 * Read IR buffer and print the message if any
 *
 */
static void
ir_read_handler( int nb_params, char **params )
{
    char *c;
    printf( "ir_read [target_ir_bit_mask]\nPress any key to quit\n" );

    if ( nb_params >= 1 )
    {
        target_ir_bit_mask = (uint32_t)strtoul( params[0], &c, 0 );
        if ( *c != 0 )
        {
            printf( "Incorrect target_ir_bit_mask value : %s\n", params[1] );
            return;
        }
    }

    printf( "Using target_ir_bit_mask=0x%x\n", target_ir_bit_mask );
    while(uart_read_nonblock() == 0) {
        for ( uint8_t ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ ) {
            if(ir_uart_read_nonblock(ir_i) != 0) {
                printf("IR %d : ",ir_i);
                while(ir_uart_read_nonblock(ir_i) != 0) {
                    char c = ir_uart_read(ir_i);
                    printf("%c, 0x%02x; ", isprint(c)?c:'_', c);
                }
                printf("\n");
            }
        }
        msleep(200);
    }
    uart_read();    // Dump the caracter sent to stop 
}
define_command( ir_read, ir_read_handler, "Read IR message", POGO_CMDS );

#ifdef REMOCON
/**
 * Command "ir_flash"
 *
 * Write blocks of data to flash
 *
 */
static void
ir_flash_handler( int nb_params, char **params )
{
    ir_boot_loop();
}

define_command( ir_flash, ir_flash_handler,
                "Write to flash from infrared (with CRC)", POGO_CMDS );
#endif

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY( byte )                                                 \
    ( byte & 0x80 ? '|' : '.' ), ( byte & 0x40 ? '|' : '.' ),                  \
        ( byte & 0x20 ? '|' : '.' ), ( byte & 0x10 ? '|' : '.' ),              \
        ( byte & 0x08 ? '|' : '.' ), ( byte & 0x04 ? '|' : '.' ),              \
        ( byte & 0x02 ? '|' : '.' ), ( byte & 0x01 ? '|' : '.' )

void dump_bits( uint32_t value, uint8_t width );

void
dump_bits( uint32_t value, uint8_t width )
{
    uint32_t mask = ( 1 << ( width - 1 ) );
    uint8_t i;
    for ( i = 0; i < width; i++ )
    {
        printf( "%c", value & mask ? '|' : '.' );
        value = value << 1;
    }
}

/**
 * Command "ir_looptest"
 *
 * Send a sequence of words and check that we receive the same.
 *
 */
#define MSG_SIZE 64 // Number of words in a packet
#define WORD_SIZE 4	// Number of bytes per word
const uint32_t msg [MSG_SIZE] = {0x8601c0ff,0xc2b86c2c,0x89c1d7f8,0xbe2214cd,
0x14c97335,0x0b86f9cd,0x20af8f37,0x2dddeafc,0xb29dfe18,0x0eb6f03d,
0xe439b004,0x15805f6d,0xd3cdd5fc,0x8ea05146,0x0a5ccda0,0x7c029fb2,
0x42caa2f1,0xf82968e8,0x8dfeb098,0x2fc88707,0x4412395e,0x5f3d33ac,
0xdedacbd4,0x360b72dc,0xcb3454f1,0x1a63c2c3,0x2430ce34,0xf779fa98,
0x48b04191,0x670b8cdd,0x431d9f74,0x2aa26d04,0x2bbb6203,0x8d6ed229,
0x82016a2e,0x79874c84,0x9788c9d3,0x168bbbc9,0x8dc1dcb5,0xac230d26,
0x136ebfb7,0x961ad678,0x14a995d5,0x2e4d492b,0x485a388f,0xb9aeef56,
0x95b50be6,0x61f05e25,0xa516ecaf,0xa5e5ca09,0x2157ecc1,0xe4d91a0b,
0x655f08bb,0xd3b7c48f,0x779d5e61,0xd5a57a5f,0xfa84fcf8,0x195a8d55,
0xe89f9c35,0x82596677,0x5953b409,0x0e4d42aa,0x00000000,0x00000001};
//const uint8_t * msg_ = msg;
ir_uart_word_t * msg_ = (ir_uart_word_t *)msg;
static void
ir_looptest_handler( int nb_params, char **params )
{
    time_reference_t my_timer;
    uint32_t wordcount;
    uint32_t size_in_word, j, k;

    if ( ( nb_params == 0 ) || ( nb_params > 1 ) ) {
        printf( "ir_looptest <framecount>\n" );
        return;
    }
    else {
        char *c;
        wordcount = (unsigned int)strtoul( params[0], &c, 0 );
        if ( ( *c != 0 ) ) {
            printf( "Not an unsigned integer: %s\n", params[0] );
            return;
        }
    }
    ir_uart_sync();

    printf( "Using target_ir_bit_mask=0x%x\n", target_ir_bit_mask );
    switch(sizeof(ir_uart_word_t)) {
        case 1:
            size_in_word = MSG_SIZE*WORD_SIZE;
            break;
        case 4:
            size_in_word = MSG_SIZE;
            break;
        default:
            printf("Error size of word not supported yet : %d \n", sizeof(ir_uart_word_t));
            break;
    }

    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ ) {
        uint32_t errors = 0;
        uint32_t cycles_wait_send = 0, cycles_wait_recv = 0;
        int32_t elapsed;

        if ( ( ( target_ir_bit_mask >> ir_i ) & 1 ) == 0 ) {
            printf( "\nSkipping IR %d\n", ir_i );
            continue;
        }

        printf( "\nProceeding on IR %d\n", ir_i );
        printf(
            "Will send 64x32 bit words %lu times and check if they are correctly received.\n",
            wordcount );
        printf( "You can press a key at any time to shorten the test.\n" );


        while( ir_uart_read_nonblock(ir_i) != 0 ) {
            ir_uart_read(ir_i); // Purge the buffer first
        }
        pogobot_stopwatch_reset(&my_timer);     // Start the stopwatch
        for ( j = 0; j < wordcount; j++ )
        {	
            //printf("Sending : IRn_tx_write_msg(%d, msg, %d) iteration %d\n", 1<<ir_i, size_in_word, j); 
            if( IRn_tx_write_msg(1<<ir_i, msg_, size_in_word) != 0 ) {
                printf("Error sending message");
            }
            //pogobot_infrared_sendMessageOneDirection( 1<<ir_i, 0, msg_, size_in_word);

            static const int tries_max = 1000;
            uint32_t tries = 0;
            for( k=0; (k< size_in_word) & (tries<tries_max); ) {
                if( ir_uart_read_nonblock(ir_i) != 0 ) {
                    char recd = ir_uart_read(ir_i);
                    if ( recd != msg_[k] ) {
                        printf("Index: %ld, Sent: 0x%02x, Received : 0x%02x\n", k, msg_[k], recd);
                        errors++;
                        if (errors > 10) {
                            printf("Too many errors, aborting\n");
                            j=wordcount;
                            break;
                        }
                    }
                    k++;
                }
                else {
                    cycles_wait_recv++;
                    tries++;
                    if ( tries >= tries_max ) {
                        errors++;
                        printf( "-" );
                    }
                }
            }
            /* SLIP version in progress :
            if( pogobot_infrared_message_available() ) {
                message_t mr;
                pogobot_infrared_recover_next_message( &mr );
                for( k=0; k< size_in_word; k++) {
                    if ( mr.payload[k] != msg_[k] ) {
                        printf("Sent: 0x%02x, Received : 0x%02x from IR %d\n", msg_[k], mr.payload[k], mr.header._receiver_ir_index);
                    }
                }
            }
            */
            //}
            /*if( ir_uart_read_nonblock(ir_i) != 0 ) {
                ir_uart_read(ir_i); // Read trailing zero if any
            }*/
            if ( uart_read_nonblock() != 0 ) {
                printf( "\nCancelled by keypress at %lu.", j );
                break;
            }
        }

        elapsed = pogobot_stopwatch_get_elapsed_microseconds( &my_timer );
       
        if(errors == 0) {
            wordcount = j;
            printf("Spent %ld CPU cycles (one cycle lasts %u ns), total of %lu ns\n", elapsed, CLOCK_NS, elapsed*CLOCK_NS);
            printf("Sent %lu bytes in %lu nanoseconds. Speed : %lu B/s\n", wordcount*size_in_word, elapsed*CLOCK_NS, (long unsigned int)((wordcount*size_in_word*1000000000LL)/(elapsed*CLOCK_NS)));

            uint32_t errors_per_million = errors * 1000000L / (wordcount*size_in_word);
            printf( "Sent %lu words, error count %lu, errors per million=%lu.\n",
                    wordcount*size_in_word, errors, errors_per_million );
            printf( "cycles_wait_send=%lu, cycles_wait_recv=%lu\n",
                    cycles_wait_send, cycles_wait_recv );
        }
    }
}
define_command(
    ir_looptest, ir_looptest_handler,
    "Send a string of characters through IR and check they are received.",
    POGO_CMDS );

/**
 * Command "ir_bouncer"
 *
 * Wait for characters Re-emits Bouncer IR buffer and print the message if any
 *
 *
static void
ir_bouncer_handler( int nb_params, char **params )
{
    printf( "ir_bouncer_handler\n" );

    printf( "Using target_ir_bit_mask=0x%x\n", target_ir_bit_mask );

    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ )
    {
        if ( ( ( target_ir_bit_mask >> ir_i ) & 1 ) == 0 )
        {
            printf( "\nSkipping IR %d\n", ir_i );
            continue;
        }

        printf( "\nProceeding on IR %d\n", ir_i );

        printf( "ir_bouncer started. Press a key to stop and show stats.\n" );

        uint32_t wordcount = 0;


        uint32_t errors = 0;
        uint32_t cycles_wait_send = 0, cycles_wait_recv = 0;

        ir_uart_word_t recd = 0;
        ir_uart_word_t sent = 0;

        while ( uart_read_nonblock() == 0 )
        {
            {
                static const int tries_max = 100;
                uint32_t tries = 0;
                while ( ( IRn_rx_rxempty_read( ir_i ) == 1 ) &&
                        ( tries < tries_max ) )
                {
                    cycles_wait_recv++;
                    tries++;
                }
                if ( tries >= tries_max )
                {
                    // printf("-");
                }
            }

            if ( IRn_rx_rxempty_read( ir_i ) != 1 )
            {
                recd = IRn_rx_read( ir_i );
                IRn_uart_ev_pending_write( ir_i, UART_EV_RX );
            }
            else
            {
                // printf("_");
            }

            if ( recd == sent )
            {
                // printf("=");
                continue;
            }

            wordcount++;

            if ( ( ir_uart_word_t )( recd - sent ) != 1 )
            {
                errors++;
                // printf("e");
            }

            sent = (ir_uart_word_t)recd;

            while ( ir_tx_txfull_read( ir_i ) == 1 )
            {
                printf( "f" );
            }

            IRn_rxtx_write( ir_i, sent );
            IRn_uart_ev_pending_write( ir_i, UART_EV_TX );

            while ( ir_tx_txempty_read() != 1 )
            {
                cycles_wait_send++;
            }
        }

        uint32_t errors_per_million = errors * 1000000L / wordcount;
        printf( "Sent %u words, error count %u, errors per million=%u.\n",
                wordcount, errors, errors_per_million );
        printf( "cycles_wait_send=%lu, cycles_wait_recv=%lu\n",
                cycles_wait_send, cycles_wait_recv );

        uint64_t numerator =
            ( (uint64_t)wordcount ) * ( (uint64_t)CONFIG_CLOCK_FREQUENCY );
        uint64_t denominator = ( (uint64_t)start - (uint64_t)end );
        uint32_t ir_speed = numerator / denominator;
        printf( "IR comm speed:  %lub/s\n", ir_speed );
    }
}
define_command( ir_bouncer, ir_bouncer_handler,
                "Re-send any characters received through IR and, on exit, show "
                "some counters.",
                POGO_CMDS );
*/

#ifdef REMOCON
/************************* Remote Control Commands ***************************/
/**
 * Command "rc_reboot"
 *
 * Reboot the robots through infrared
 *
 */
static void
rc_reboot_handler( int nb_params, char **params )
{
    printf("Rebooting...");
    ir_uart_word_t reboot_val=0xDE; 
    IRn_tx_write_msg(target_ir_bit_mask,&reboot_val,1);
}

define_command( rc_reboot, rc_reboot_handler,
                "Reboot all pogobots available through infrared", POGO_CMDS );

/**
 * Command "rc_flash"
 *
 * flash program
 *
 */
static void
rc_flash_handler( int nb_params, char **params )
{
    uint32_t *meta_addr;
    uint32_t data_addr;
    uint32_t size;
    struct sfl_frame frame;
    meta_addr = USRPRG_CRC+SPIFLASH_BASE;    // 0x180000
    size = *meta_addr;
    meta_addr++;                 // Go to start of first frame metadata
    printf("Address : 0x%08x\nSize to flash = %ld\n", meta_addr, size);
    for( uint32_t i = 0 ; i < size ; ) {                    // For each frame
        // 4 bytes for meta_address, 1 for size, two for CRC, one unused
        printf("DEBUG: treating meta_addr : 0x%08x\n", meta_addr);
        printf("DEBUG: *meta_addr : 0x%08x\n", *meta_addr);
        data_addr = (*meta_addr);   // Contains address in flash + SPIFLASH_BASE
        data_addr -= SPIFLASH_BASE;
        frame.payload[3] = data_addr >> 24;
        frame.payload[2] = (data_addr >> 16) & 0xff;
        frame.payload[1] = (data_addr >> 8) & 0xff;
        frame.payload[0] = data_addr & 0xff;
        if((data_addr < 0x40000 ) || (data_addr > 0x90000)) {
            printf("ERROR: bad address : 0x%08x\n", data_addr);
            return; // Bad meta_address
        }
        meta_addr++; // Next 4 bytes
        frame.payload_length = (*meta_addr) & 0xff;
        frame.crc[1] = ((*meta_addr)>>16 & 0xff);
        frame.crc[0] = ((*meta_addr) & 0xff);
        frame.cmd = SFL_CMD_LOAD;
        for( uint32_t j; j< frame.payload_length; j++) {
            frame.payload[j+4] = *((uint32_t *)data_addr+SPIFLASH_BASE);
        }
        if(check_crc(&frame)) {
            printf("CRC check failed\n");
            print_frame(&frame);
        }
        else {
            IRn_tx_write_msg(0xff, &frame, frame.payload_length + 4);
        }
        i += frame.payload_length;
        meta_addr++;   // Go to next metadata (8 bytes)
    }

}

define_command( rc_flash, rc_flash_handler,
                "Flash pogobots available through infrared", POGO_CMDS );
/**
 * Command "rc_erase"
 *
 * Erase program
 *
 */
static void
rc_erase_handler( int nb_params, char **params )
{
    //erase_userprog
}

define_command( rc_erase, rc_erase_handler,
                "Erase all pogobots available through infrared", POGO_CMDS );


/**
 * Command "rc_send_cmd"
 *
 * Send command through infrared
 *
 */
static void
rc_send_cmd_handler( int nb_params, char **params )
{
    char *c;
    uint8_t i,j;
    unsigned int count=0;
    extern const char * ir_magic_req;

    if ( ( nb_params == 0 ) )
    {
        printf( "rc_send_cmd <command> [arguments] \n" );
        return;
    }

    for(i=0; i<strlen(ir_magic_req); i++) {
        ir_tx_tx_write(ir_magic_req[i]);
        count++;
    }
    for(j=0; j<nb_params; j++) {
        for(i=0; i<strlen(params[j]); i++) {
            ir_tx_tx_write(params[j][i]);
            count++;
        }
        ir_tx_tx_write(' '); // Send space between arguments
    }
    printf( "Command sent to robots: %s, length : %d\n", params[0], count );
    ir_tx_tx_write(0);
    ir_tx_tx_write(0x0a);
    ir_tx_conf_tx_trig_write(1);
}

define_command( rc_send_cmd, rc_send_cmd_handler, "Send command through IR",
                POGO_CMDS );
#endif
#else /* CSR_IR_RX0_BASE */

#warning no IR, skipping IR software

#endif /* CSR_IR_RX0_BASE */
