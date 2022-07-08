/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


// SPDX-License-Identifier: BSD-Source-Code

#include <stdio.h>
#include <stdlib.h>
#include <memtest.h>

#include <generated/csr.h>

#include <command.h>
#include <helpers.h>
#include <pogobot.h>

/**
 * Command "timer_test"
 *
 * Test that the timer is counting correctly time
 *
 */
static void timer_test_handler(int nb_params, char **params)
{
	char *c;
	unsigned int ms;

	if (nb_params < 1) {
		printf("timer_test <number_of_milliseconds>");
		return;
	}
	ms = strtoul(params[0], &c, 0);
	if (*c != 0) {
		printf("Incorrect number of milliseconds");
		return;
	}
    msleep(ms);
}

define_command(timer_test, timer_test_handler, "Test timer", POGO_CMDS);
