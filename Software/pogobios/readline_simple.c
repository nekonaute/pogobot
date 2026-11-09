// This file is Copyright (c) 2013-2014 Sebastien Bourdeauducq <sb@m-labs.hk>
// This file is Copyright (c) 2014-2019 Florent Kermarrec <florent@enjoy-digital.fr>

// SPDX-License-Identifier: BSD-Source-Code

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "readline.h"
#include "console_input.h"

int readline(char *s, int size)
{
    if (!s || size <= 0) return -1;
	static char skip = 0;
	char c[2];
	int ptr;
    int overflow = 0;

	c[1] = 0;
	ptr = 0;
	while(1) {
        int next = bios_console_getchar();
        if (next == BIOS_CONSOLE_RESET) {
            if (ptr) bios_console_discard_line();
            s[0] = 0;
            return -1;
        }
        c[0] = (char)next;
		if (c[0] == skip)
			continue;
		skip = 0;
        if (overflow && c[0] != '\r' && c[0] != '\n') continue;
		switch(c[0]) {
			case 0x7f:
			case 0x08:
				if(ptr > 0) {
					ptr--;
					fputs("\x08 \x08", stdout);
				}
				break;
			case 0x07:
				break;
			case '\r':
				skip = '\n';
				s[overflow ? 0 : ptr] = 0x00;
				fputs("\n", stdout);
				return 0;
			case '\n':
				skip = '\r';
				s[overflow ? 0 : ptr] = 0x00;
				fputs("\n", stdout);
				return 0;
			default:
				fputs(c, stdout);
                if (ptr >= size - 1) { overflow = 1; break; }
				s[ptr] = c[0];
				ptr++;
				break;
		}
	}

	return 0;
}
