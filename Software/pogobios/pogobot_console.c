/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include "pogobot_console.h"

#include <uart.h>

void pogobot_console_init(void)
{
    uart_init();

    {
        int stuffing_done = 0;
        while (!uart_txfull_read() && stuffing_done < 64)
        {
            uart_rxtx_write('*');
            uart_ev_pending_write(UART_EV_TX);
        }
    }
}
