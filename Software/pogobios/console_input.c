/* Bounded console input shared by the full and minimal BIOS editors. */
#include "console_input.h"
#include "command.h"
#include "pffs.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <uart.h>

static bool discard_line;
void bios_console_discard_line(void) { discard_line = true; }

int bios_console_getchar(void) {
    for (;;) {
        /* Service even a partial line, including an unfinished ESC sequence. */
        bios_pffs_service();
        if (bios_pffs_input_reset())
            return BIOS_CONSOLE_RESET;
        if (!uart_read_nonblock()) {
            continue;
        }
        int c = getchar();
        if (!discard_line)
            return c;
        if (c == '\r' || c == '\n') {
            discard_line = false;
            return BIOS_CONSOLE_RESET;
        }
    }
}

int bios_console_parse(char *line, char **command, char **params) {
    /* The old helper did not bound parameter writes. Longer transfer lines
     * make rejecting excess tokens before dispatch particularly important. */
    int count = 0;
    for (int i = 0; i < MAX_PARAM; ++i)
        params[i] = NULL;
    while (*line == ' ' || *line == '\t')
        ++line;
    *command = line;
    while (*line && *line != ' ' && *line != '\t')
        ++line;
    if (*line)
        *line++ = '\0';
    while (*line) {
        while (*line == ' ' || *line == '\t')
            ++line;
        if (!*line)
            break;
        if (count == MAX_PARAM)
            return -1;
        params[count++] = line;
        while (*line && *line != ' ' && *line != '\t')
            ++line;
        if (*line)
            *line++ = '\0';
    }
    return count;
}

struct command_struct *bios_console_dispatch(char *command, int count,
                                             char **params, bool serial) {
    if (count < 0) {
        puts("Too many command arguments; discarded");
        return NULL;
    }
    for (struct command_struct *const *entry = __bios_cmd_start;
         entry != __bios_cmd_end; ++entry) {
        struct command_struct *cmd = *entry;
        if (strcmp(command, cmd->name) != 0)
            continue;
        /* PFFS uses an acknowledged UART protocol. IR broadcasts cannot own a
         * filesystem session, and may not interrupt an existing transfer. */
        if ((!serial && cmd->group == PFFS_CMDS) ||
            !bios_pffs_allows(command)) {
            puts("PFFS busy or command requires UART; command rejected");
            return cmd;
        }
        cmd->func(count, params);
        return cmd;
    }
    return NULL;
}
