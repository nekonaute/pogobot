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
#include <system.h>

#include <generated/csr.h>

#include <command.h>
#include <helpers.h>

#include <boot.h>

/**
 * Command "reboot to"
 *
 * Reboots to an address and tries to detect if it's CPU code or gateware
 *
 */
static void reboot_to_handler(int nb_params, char **params)
{
    char *c;
	unsigned int addr;
	//unsigned int length;

	if (nb_params < 1) {
		printf("reboot_to <address>");
		return;
	}

	addr = strtoul(params[0], &c, 0);
	if (*c != 0) {
		printf("Incorrect address");
		return;
	}
    reboot((uint32_t*)addr);
}

define_command(reboot_to, reboot_to_handler, "Reboot to specified address", SYSTEM_CMDS);

