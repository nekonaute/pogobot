/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#ifndef __POGOBOT_CONSOLE_H__
#define __POGOBOT_CONSOLE_H__

#include <generated/csr.h>
#include "stdbool.h"
#include "pogobot_console.h"

void pogobot_console_init(void);

/* Must run console_init() first. */
static inline bool pogobot_console_is_connected(void)
{
    return !uart_txfull_read();
}

#endif /* __POGOBOT_CONSOLE_H__ */
