#ifndef POGOBIOS_CONSOLE_INPUT_H
#define POGOBIOS_CONSOLE_INPUT_H
#include <stdbool.h>
struct command_struct;
/* Negative value asks the editor to discard a timed-out/overlong input frame.
 */
#define BIOS_CONSOLE_RESET (-2)
int bios_console_getchar(void);
void bios_console_discard_line(void);
int bios_console_parse(char *line, char **command, char **params);
struct command_struct *bios_console_dispatch(char *command, int count,
                                             char **params, bool serial);
#endif
