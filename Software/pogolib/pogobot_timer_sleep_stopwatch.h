/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#ifndef __POGOBOT_TIMER_SLEEP_STOPWATCH_H__
#define __POGOBOT_TIMER_SLEEP_STOPWATCH_H__

/* clang-format-ok */

#include <generated/csr.h>
#include <stdbool.h>

/** This is a simple timer implementation based on the available
 * hardware timer.
 *
 * The benefit is that the only hardware timer we have is configured
 * only once, and all software that depend on it just works without
 * conflict.
 *
 * The drawback is that we can't use the interrupt of the hardware
 * timer, if available.
 *
 * To keep the implementation simple, we only rely on the 32bit timer0
 * counter.  This limit the timespan to 2^31/CLOCKRATE, which is 107
 * seconds with a 20MHz system clock.  We can extent the
 * implementation by maintaining a rollover count in some function
 * that we promise to call often enough, add a rollover count in
 * time_reference_t, and extend computations accordingly.
 *
 * Sample code:
 *
 *
 * One time interval, measuring duration, think "get elapsed microseconds".
 *
 * time_reference_t mystopwatch;
 * pogobot_stopwatch_reset( &mystopwatch );
 * my long_computation( arguments );
 * uint32_t microseconds =
 *     pogobot_stopwatch_get_elapsed_microseconds( &mystopwatch );
 * printf( "Duration: %u microseconds", milliseconds );
 *
 *
 * Series of events, measuring duration of each, think "stopwatch lap".
 *
 * time_reference_t mystopwatch;
 * pogobot_stopwatch_reset( &mystopwatch );
 * while ( 1 )
 * {
 *     wait_for_some_external_event();
 *     uint32_t microseconds = pogobot_stopwatch_lap( &mystopwatch );
 *     printf( "Time since previous event: %u microseconds", milliseconds );
 * }
 *
 *
 * You want your code to periodically do something.
 *
 * time_reference_t mytimer;
 * uint32_t period_microseconds = 250000;
 * pogobot_timer_init( &mytimer, period_microseconds );
 * while (1)
 * {
 *    wait_for_some_external_event();
 *    if (pogobot_timer_has_expired())
 *    {
 *        pogobot_timer_offset_origin_microseconds( &mytimer,
 *                                                  period_microseconds );
 *        react_do_something();
 *    }
 * }
 *
 *
 */

void pli_timer_sleep_stopwatch_init( void );

typedef struct time_reference_t
{
    uint32_t hardware_value_at_time_origin;
} time_reference_t;

/** You can allocate a time_reference_t freely.  To use it as a
 * stopwatch you must reset it using pogobot_stopwatch_reset(); */
void pogobot_stopwatch_reset( time_reference_t *stopwatch );

/** This function, in one operation, measure time elapsed from origin and
offsets so that origin is zero at this point in time.  It returns the number of
microseconds elapsed (which may be negative if you offset the origin to the
future).

This is similar to doing:

uint32_t duration = pogobot_stopwatch_get_elapsed_microseconds( *mystopwatch);
pogobot_stopwatch_offset_origin_microseconds(duration);

but using pogobot_stopwatch_lap() is cycle accurate, while the sequence above is
accurate only to the microsecond.
 */
int32_t pogobot_stopwatch_lap( time_reference_t *stopwatch );

/** As its name implies, just provides the current number of elapsed
 * microseconds without otherwise interfering with the stopwatch
 * state.
 *
 * If you have offset origin to the future and not waited more than
 * the offset, you may get a negative result.
 */
int32_t
pogobot_stopwatch_get_elapsed_microseconds( time_reference_t *stopwatch );

/** This manually  is very useful for perfectly precise measurements of events
 * that start one after another.
 *
 */
void
pogobot_stopwatch_offset_origin_microseconds( time_reference_t *stopwatch,
                                              int32_t microseconds_offset );

/** Set a timer that will expire in the defined number of microseconds
 * in the future. */
void pogobot_timer_init( time_reference_t *timer, int32_t microseconds_to_go );

/** As its name implies, just provides the current of microseconds
 * until the timer has expired, without otherwise interfering with the
 * timer state.
 *
 * The result is a signed number, positive when the timer has not
 * expired yet, negative when the timer has expired.
 */
int32_t pogobot_timer_get_remaining_microseconds( time_reference_t *timer );

/** Returns true when the timer has expired, false when the timer has
 * not expired yet.
 */
bool pogobot_timer_has_expired( time_reference_t *timer );

/** As the names imples, waits until the timer has expired.
 */
void pogobot_timer_wait_for_expiry( time_reference_t *timer );

void pogobot_timer_offset_origin_microseconds( time_reference_t *timer,
                                               int32_t microseconds_offset );

#endif /* __POGOBOT_TIMER_SLEEP_STOPWATCH_H__ */
