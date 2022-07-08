/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include "sleep.h"
#include "pogobot_timer_sleep_stopwatch.h"

/* clang-format-ok */

void
usleep( int us )
{
    time_reference_t tr;
    pogobot_timer_init( &tr, us );
    pogobot_timer_wait_for_expiry( &tr );
}

void
msleep( int ms )
{
    usleep( ms * 1000 );
}

void
sleep( int ms )
{
    usleep( ms * 1000000 );
}
