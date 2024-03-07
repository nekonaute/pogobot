/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include "pogobot.h"

/* clang-format-ok */

/* ******************************** ********************************
 * Hardware initialization
 * ******************************** ********************************/

void
pli_timer_sleep_stopwatch_init( void )
{
    timer0_en_write( 0 );
    timer0_load_write( 0 );

    // Setting the reload register to a non-zero value sets the
    // period.  We want the biggest period, hence ~0.
    timer0_reload_write( ~0 );
    timer0_en_write( 1 );
}

/* ******************************** ********************************
 * Helper conversion macros
 * ******************************** ********************************/

/* Since the result can vary covering the full 32bit range, we need a
 * 64bit operation. */
#define microseconds_to_ticks( microseconds )                                  \
    ( ( (uint64_t)microseconds * (uint64_t)CONFIG_CLOCK_FREQUENCY ) / 1000000L )
#define ticks_to_microseconds( ticks )                                         \
    ( ( (uint64_t)time_ticks * 1000000L ) / (uint64_t)CONFIG_CLOCK_FREQUENCY )

/* ******************************** ********************************
 * Stopwatch implementation
 * ******************************** ********************************/

void
pogobot_stopwatch_reset( time_reference_t *stopwatch )
{
    timer0_update_value_write( 1 );
    stopwatch->hardware_value_at_time_origin = timer0_value_read();
}

int32_t
pogobot_stopwatch_lap( time_reference_t *stopwatch )
{
    timer0_update_value_write( 1 );
    uint32_t now = timer0_value_read();

    /* Hardware timer counts down, so we subtract in this order. */
    int32_t time_ticks = ( stopwatch->hardware_value_at_time_origin - now );

    /* time_ticks is signed, operation at binary level is the same.*/
    stopwatch->hardware_value_at_time_origin -= time_ticks;

    return ticks_to_microseconds( time_ticks );
}

int32_t
pogobot_stopwatch_get_elapsed_microseconds( time_reference_t *stopwatch )
{
    timer0_update_value_write( 1 );
    uint32_t now = timer0_value_read();

    /* Hardware timer counts down, so we subtract in this order. */
    int32_t time_ticks = ( stopwatch->hardware_value_at_time_origin - now );

    return ticks_to_microseconds( time_ticks );
}

void
pogobot_stopwatch_offset_origin_microseconds( time_reference_t *stopwatch,
                                              int32_t microseconds_offset )
{
    int32_t ticks_offset = microseconds_to_ticks( microseconds_offset );
    stopwatch->hardware_value_at_time_origin -= ticks_offset;
}

/* ******************************** ********************************
 * Timer implementation
 * ******************************** ********************************/

void
pogobot_timer_init( time_reference_t *timer, int32_t microseconds_to_go )
{
    pogobot_stopwatch_reset( timer );
    pogobot_stopwatch_offset_origin_microseconds( timer, microseconds_to_go );
}

int32_t
pogobot_timer_get_remaining_microseconds( time_reference_t *timer )
{
    timer0_update_value_write( 1 );
    uint32_t now = timer0_value_read();

    /* Hardware timer counts down, but it has not decreased yet to the
     * target value, so we subtract in this order. */
    int32_t time_ticks = ( now - timer->hardware_value_at_time_origin );

    return ticks_to_microseconds( time_ticks );
}

bool
pogobot_timer_has_expired( time_reference_t *timer )
{
    timer0_update_value_write( 1 );
    uint32_t now = timer0_value_read();

    /* Hardware timer counts down, so it has expired if current value
     * is smaller than origin. */
    int32_t time_ticks = ( now - timer->hardware_value_at_time_origin );

    return ( time_ticks < 0 );
}

void
pogobot_timer_wait_for_expiry( time_reference_t *timer )
{
    while ( 1 )
    {
        if ( pogobot_timer_has_expired( timer ) )
            break;
    }
}

/* Same implementation as pogobot_stopwatch_offset_origin_microseconds(). */
void
pogobot_timer_offset_origin_microseconds( time_reference_t *timer,
                                          int32_t microseconds_offset )
{
    int32_t ticks_offset = microseconds_to_ticks( microseconds_offset );
    timer->hardware_value_at_time_origin -= ticks_offset;
}
