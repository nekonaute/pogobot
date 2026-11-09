/* Exercise the actual BIOS dispatcher/editor/PFFS against strict NOR flash.
 * ! commands are host-test controls; they never enter production firmware. */
#include "command.h"
#include "console_input.h"
#include "pffs.h"
#include "pffs/flash_file.h"
#include "pffs/flash_log.h"
#include "pogobot.h"
#include "readline.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t flash[0x200000];
static unsigned programs, erases, dangerous_calls;
static bool torn;
static uint32_t now_us;
static const char *input;

void erase_write_section_flash(void) {
    memset(flash + POGOBOT_USER_FLASH_START_OFFSET, 0xff,
           sizeof(flash) - POGOBOT_USER_FLASH_START_OFFSET);
}
int spiBeginErase4(uint32_t address) {
    assert(address >= POGOBOT_USER_FLASH_START_OFFSET &&
           address <= sizeof(flash) - 4096);
    assert(address % 4096u == 0u);
    memset(flash + address, 0xff, 4096u);
    ++erases;
    return 0;
}
void write_page_flash(uint16_t page, const void *data) {
    assert(page < POGOBOT_USER_FLASH_PAGE_COUNT);
    for (unsigned i = 0; i < (torn ? 8u : 256u); ++i)
        flash[POGOBOT_USER_FLASH_START_OFFSET + (uint32_t)page * 256u + i] &=
            ((const uint8_t *)data)[i];
    ++programs;
}
void read_page_flash(uint16_t page, char *data) {
    assert(page < POGOBOT_USER_FLASH_PAGE_COUNT);
    memcpy(data,
           flash + POGOBOT_USER_FLASH_START_OFFSET + (uint32_t)page * 256u,
           256u);
}
uint16_t pogobot_helper_getid(void) { return 42; }
void pogobot_led_setColor(uint8_t r, uint8_t g, uint8_t b) {
    (void)r;
    (void)g;
    (void)b;
}
void rgb_blink(void) {}
void pogobot_stopwatch_reset(time_reference_t *t) { t->origin = now_us; }
int32_t pogobot_stopwatch_get_elapsed_microseconds(time_reference_t *t) {
    return (int32_t)(now_us - t->origin);
}
void pogobot_stopwatch_offset_origin_microseconds(time_reference_t *t,
                                                  int32_t us) {
    t->origin += (uint32_t)us;
}
int uart_read_nonblock(void) {
    if (input && *input)
        return 1;
    /* A truncated frame waits long enough for production timeout handling. */
    now_us += 1000000u;
    return 0;
}
int getchar(void) {
    assert(input && *input);
    return (unsigned char)*input++;
}
int complete(char *line, char **tail) {
    (void)line;
    *tail = "";
    return 0;
}
static void dangerous(int n, char **p) {
    (void)n;
    (void)p;
    ++dangerous_calls;
}
define_command(serialboot, dangerous, "test unrelated flash command",
               BOOT_CMDS);

static void dispatch(char *line, bool serial) {
    char *command, *params[MAX_PARAM];
    int n = bios_console_parse(line, &command, params);
    if (*command)
        bios_console_dispatch(command, n, params, serial);
}
static void command(const char *line, bool serial) {
    char copy[160];
    assert(strlen(line) < sizeof(copy));
    strcpy(copy, line);
    dispatch(copy, serial);
}
static int edited(const char *line, char output[160]) {
    input = line;
    return readline(output, 160);
}
static void selftest(void) {
    uint8_t page[256];
    pogo_flash_file_info_t info;
    unsigned before = erases;
    command("touch 1 notes", true);
    command("format NO", true);
    command("format YES", false);
    assert(erases == before);
    command("format YES", true);
    assert(erases == before + 16);
    command("touch 1 notes", true);
    command("write notes 0 4869", true);
    assert(pogo_flash_file_read_page_secure(1, 0, page, &info) ==
           POGO_FLASH_FILE_OK);
    assert(info.size_bytes == 2 && page[0] == 'H' && page[1] == 'i');
    command("mv notes renamed", true);
    assert(pogo_flash_file_find_by_name("renamed", &info) ==
           POGO_FLASH_FILE_OK);
    command("stat renamed", true);
    command("cat renamed", true);
    command("ls", true);
    command("df", true);
    command("touch 2 later 257", true);
    command("rm renamed", true);
    /* Compaction stays in maintenance applications; BIOS must not move files. */
    assert(!bios_pffs_busy());
    char *defrag_args[] = {"YES"};
    assert(bios_console_dispatch("defrag", 1, defrag_args, true) == NULL);
    assert(pogo_flash_file_read_page_secure(2, 1, page, NULL) ==
           POGO_FLASH_FILE_OK);
    (void)bios_pffs_input_reset();
    command("serialboot", true);
    assert(dangerous_calls == 1);
    /* Corrupt metadata must never cause implicit formatting at shell entry. */
    flash[POGOBOT_USER_FLASH_START_OFFSET] ^= 1;
    before = erases;
    command("touch 3 unsafe", true);
    assert(erases == before);
    command("format YES", true);
    /* Both parameter overflow and overlong editor lines reject the entire
     * input. */
    struct {
        char *p[MAX_PARAM];
        uint32_t guard;
    } args = {.guard = 0x12345678};
    char too_many[] = "format YES a b c d e f g h i j", *name;
    assert(bios_console_parse(too_many, &name, args.p) == -1 &&
           args.guard == 0x12345678);
    struct {
        char line[160];
        uint32_t guard;
    } out = {.guard = 0x12345678};
    char exact[161];
    memset(exact, 'x', 159);
    exact[159] = '\n';
    exact[160] = 0;
    assert(edited(exact, out.line) >= 0 && strlen(out.line) == 159 &&
           out.guard == 0x12345678);
#ifndef TERM_MINI
    /* History recall must preserve the longer line without crossing its NUL. */
    assert(edited("\x1b[A\n", out.line) >= 0 && strlen(out.line) == 159 &&
           out.guard == 0x12345678);
#endif
    char excess[250];
    memset(excess, ' ', sizeof(excess));
    memcpy(excess, "format YES", 10);
    excess[248] = '\n';
    excess[249] = 0;
    before = erases;
    edited(excess, out.line);
    assert(out.line[0] == 0 && out.guard == 0x12345678);
    if (*out.line)
        dispatch(out.line, true);
    assert(erases == before);
    /* An unfinished upload keeps unrelated commands locked until timeout.
     * The editor must discard the partial prefix AND its later tail. */
    command("put 12345678 10 binary 48 00000000 raw", true);
    assert(bios_pffs_busy());
    command("serialboot", true);
    assert(dangerous_calls == 1);
    command("format YES", true);
    assert(erases == before);
    input = "putdata ";
    edited(input, out.line);
    assert(!bios_pffs_busy() && !out.line[0]);
    edited("format YES\n", out.line);
    assert(!out.line[0] && erases == before);
    edited("stat 2\n", out.line);
    assert(strcmp(out.line, "stat 2") == 0);
    /* An ESC prefix also owns input even before any printable characters. */
    command("put 12345678 12 escaped 48 00000000 raw", true);
    assert(bios_pffs_busy());
    edited("\x1b[", out.line);
    assert(!bios_pffs_busy() && !out.line[0]);
    edited("format YES\n", out.line);
    assert(!out.line[0] && erases == before);
    /* Counter wrap does not prematurely expire or indefinitely extend sessions.
     */
    now_us = UINT32_MAX - 1000u;
    bios_pffs_init();
    command("put 12345678 11 wrapped 48 00000000 raw", true);
    assert(bios_pffs_busy());
    for (unsigned ms = 0; ms < 29900; ms += 100) {
        now_us += 100000u;
        bios_pffs_service();
    }
    assert(bios_pffs_busy());
    now_us += 100000u;
    bios_pffs_service();
    assert(!bios_pffs_busy());
    for (unsigned i = 0; i < POGOBOT_USER_FLASH_START_OFFSET; ++i)
        assert(flash[i] == 0xa5);
    puts("PFFS BIOS console/NOR integration: PASS");
}

int main(int argc, char **argv) {
    memset(flash, 0xa5, sizeof(flash));
    erase_write_section_flash();
    bios_pffs_init();
#ifndef TERM_MINI
    hist_init();
#endif
    if (argc > 1 && strcmp(argv[1], "--selftest") == 0) {
        selftest();
        return 0;
    }
    assert(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        if (line[0] != '!') {
            input = line;
            do {
                char output[160] = {0};
                readline(output, sizeof(output));
                putchar('\n');
                if (*output)
                    dispatch(output, true);
            } while (*input);
            fflush(stdout);
            continue;
        }
        char *words[8];
        unsigned count = 0;
        char *word = strtok(line, " \t\r\n");
        while (word && count < 8) {
            words[count++] = word;
            word = strtok(NULL, " \t\r\n");
        }
        if (strcmp(words[0], "!stats") == 0)
            printf("TEST %u %u %u\n", programs, erases, bios_pffs_busy());
        else if (strcmp(words[0], "!blank") == 0) {
            erase_write_section_flash();
            puts("TEST OK");
        } else if (strcmp(words[0], "!format") == 0) {
            assert(!bios_pffs_busy());
            assert(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
            puts("TEST OK");
        } else if (strcmp(words[0], "!reboot") == 0) {
            bios_pffs_init();
            puts("TEST OK");
        } else if (strcmp(words[0], "!poll") == 0) {
            now_us = (uint32_t)strtoul(words[1], NULL, 10) * 1000u;
            bios_pffs_service();
            puts("TEST OK");
        } else if (strcmp(words[0], "!torn") == 0) {
            torn = true;
            puts("TEST OK");
        } else if (strcmp(words[0], "!corruptcat") == 0) {
            flash[POGOBOT_USER_FLASH_START_OFFSET] ^= 1;
            puts("TEST OK");
        } else if (strcmp(words[0], "!corrupt") == 0) {
            pogo_flash_file_info_t info;
            assert(pogo_flash_file_find((uint8_t)atoi(words[1]), &info) ==
                   POGO_FLASH_FILE_OK);
            flash[POGOBOT_USER_FLASH_START_OFFSET +
                  (uint32_t)info.first_page * 256] ^= 1;
            puts("TEST OK");
        } else if (strcmp(words[0], "!native") == 0) {
            uint8_t data[512];
            for (unsigned i = 0; i < 512; ++i)
                data[i] = (uint8_t)(i * 13u);
            assert(pogo_flash_file_create(71, "native", 2, 1, data) ==
                   POGO_FLASH_FILE_OK);
            puts("TEST OK");
        } else if (strcmp(words[0], "!log") == 0) {
            pogo_flash_log_t log;
            size_t accepted;
            uint8_t tail[] = {0, 255, 'x', '\n'};
            assert(pogo_flash_log_initialize(&log, 70, "events", 4, false,
                                             NULL) == POGO_FLASH_LOG_OK);
            assert(pogo_flash_log_append(&log, "alpha\n", 6, &accepted) ==
                   POGO_FLASH_LOG_OK);
            assert(pogo_flash_log_force_flush(&log) == POGO_FLASH_LOG_OK);
            assert(pogo_flash_log_append(&log, tail, sizeof(tail), &accepted) ==
                   POGO_FLASH_LOG_OK);
            assert(pogo_flash_log_force_flush(&log) == POGO_FLASH_LOG_OK);
            puts("TEST OK");
        } else
            puts("TEST UNKNOWN");
        fflush(stdout);
    }
    return 0;
}
