/* Exercise the actual BIOS dispatcher/editor/PFFS against strict NOR flash.
 * ! commands are host-test controls; they never enter production firmware. */
#include "command.h"
#include "console_input.h"
#include "pffs.h"
#include "pffs/flash_file.h"
#include "pffs/flash_log.h"
#include "pffs/load.h"
#include "pffs/transfer.h"
#include "pogobot.h"
#include "boot.h"
#include "ir_boot.h"
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
/* NOR faults exercise the real controller installer, not a duplicate copy of
 * its state machine. All source/user pages remain guarded separately below. */
static unsigned controller_erases, controller_programs, marker_erases, ir_resets;
static bool marker_erase_fails, controller_erase_fails, late_corruption;
static uint32_t failed_program_address = UINT32_MAX;
static uint32_t corrupt_source_address = UINT32_MAX;

uint8_t pffs_test_flash_read(uint32_t address) {
    assert(address < sizeof(flash));
    return flash[address];
}
uint8_t check_flash_state(const char *text, uint32_t address) {
    assert(address + strlen(text) <= sizeof(flash));
    return memcmp(flash + address, text, strlen(text)) == 0;
}
uint8_t flash_invalidate_user_image(void) {
    if (marker_erase_fails)
        return 0;
    ++marker_erases;
    memset(flash + FLASH_OK_OFFSET, 0xff, 4096u);
    return 1;
}
void ir_boot_reset_v2_upload(void) { ++ir_resets; }
/* File-radio transport is covered by the real IR receiver/sender harness.
 * These doubles keep console-only fixtures independent of radio timing. */
static bool radio_pending;
static unsigned controller_starts;
static uint8_t led_color[3], blink_color[3];
/* A boot request is observable only after the production installer has
 * committed its marker. Tests continue instead of resetting the CPU. */
void reboot_ctrl_write(uint32_t value) {
    assert(value == 0xadu && check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET));
    ++controller_starts;
}
bool ir_file_busy(void) { return radio_pending; }
void ir_file_abort(void) { radio_pending = false; }
bool ir_file_send(uint8_t id, unsigned passes, bool replace, uint8_t *scratch) {
    (void)id; (void)passes; (void)replace; (void)scratch;
    return false;
}
void erase_flash_sector(unsigned address) {
    assert(address >= POGOBIOS_CONTROLLER_OFFSET &&
           address + SPIFLASH_SECTOR_SIZE <=
               POGOBIOS_CONTROLLER_OFFSET + POGOBIOS_CONTROLLER_SIZE);
    assert(address % SPIFLASH_SECTOR_SIZE == 0u);
    ++controller_erases;
    if (!controller_erase_fails)
        memset(flash + address, 0xff, SPIFLASH_SECTOR_SIZE);
    if (corrupt_source_address != UINT32_MAX) {
        flash[corrupt_source_address] ^= 1u;
        corrupt_source_address = UINT32_MAX;
    }
}
void write_to_flash(unsigned address, const unsigned char *data, unsigned length) {
    bool controller = address >= POGOBIOS_CONTROLLER_OFFSET &&
        address + length <= POGOBIOS_CONTROLLER_OFFSET + POGOBIOS_CONTROLLER_SIZE;
    assert(controller || (address >= FLASH_OK_OFFSET &&
                           address + length <= FLASH_V2_PARTIAL_OFFSET + 10u));
    if (controller)
        ++controller_programs;
    for (unsigned i = 0u; i < length; ++i) {
        if (address + i != failed_program_address)
            flash[address + i] &= data[i];
    }
    if (late_corruption && address == POGOBIOS_CONTROLLER_OFFSET + 256u)
        flash[POGOBIOS_CONTROLLER_OFFSET + 16u] ^= 1u;
}

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
    led_color[0] = r; led_color[1] = g; led_color[2] = b;
}
void rgb_blink_set_color(uint8_t r, uint8_t g, uint8_t b) {
    blink_color[0] = r; blink_color[1] = g; blink_color[2] = b;
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

static uint8_t controller_image[POGOBIOS_CONTROLLER_SIZE + 256u];
static uint8_t saved_flash[sizeof(flash)];

static void seed_old_controller(void) {
    memset(flash + POGOBIOS_CONTROLLER_OFFSET, 0xa5, POGOBIOS_CONTROLLER_SIZE);
    memset(flash + FLASH_OK_OFFSET, 0xff, 4096u);
    memcpy(flash + FLASH_OK_OFFSET, FLASH_IS_OK, strlen(FLASH_IS_OK));
}

/* Precondition/console rejections must not even invalidate the prior image or
 * discard its cached IR progress. Whole-flash comparison also guards PFFS. */
static void expect_load_rejected(const char *line, bool serial) {
    memcpy(saved_flash, flash, sizeof(flash));
    unsigned before = ir_resets;
    unsigned erased = controller_erases + marker_erases;
    unsigned written = controller_programs;
    command(line, serial);
    assert(memcmp(saved_flash, flash, sizeof(flash)) == 0);
    assert(ir_resets == before && controller_programs == written &&
           controller_erases + marker_erases == erased);
}

static void controller_selftest(void) {
    memset(flash, 0xa5, sizeof(flash));
    erase_write_section_flash();
    bios_pffs_init();
    seed_old_controller();
    expect_load_rejected("load 20", true); /* Virgin filesystem. */
    assert(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
    expect_load_rejected("load missing", true);
    expect_load_rejected("load 0", true);
    expect_load_rejected("load", true);
    expect_load_rejected("load 20 extra", true);
    for (unsigned i = 0u; i < sizeof(controller_image); ++i)
        controller_image[i] = (uint8_t)(i * 29u + i / 256u);
    controller_image[0] = 0x6f; /* Raw RISC-V JAL entry, not a file envelope. */
    const unsigned sizes[] = {4u, 255u, 256u, 257u, 65536u, 65537u,
                              POGOBIOS_CONTROLLER_SIZE};
    for (unsigned i = 0u; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        unsigned length = sizes[i];
        assert(pogo_flash_file_create_bytes(20, "controller", length, 1,
                                           controller_image) == POGO_FLASH_FILE_OK);
        seed_old_controller();
        memcpy(saved_flash, flash, sizeof(flash));
        unsigned erased = controller_erases, written = controller_programs;
        unsigned reset = ir_resets;
        command(i & 1u ? "load 20" : "load controller", true);
        unsigned extent = (length + 255u) & ~255u;
        assert(controller_programs == written + extent / 256u);
        assert(controller_erases == erased +
               (extent + SPIFLASH_SECTOR_SIZE - 1u) / SPIFLASH_SECTOR_SIZE);
        assert(ir_resets == reset + 1u);
        assert(check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET));
        assert(memcmp(flash + POGOBIOS_CONTROLLER_OFFSET, controller_image,
                      length) == 0);
        for (unsigned byte = length; byte < extent; ++byte)
            assert(flash[POGOBIOS_CONTROLLER_OFFSET + byte] == 0xffu);
        assert(memcmp(flash, saved_flash, POGOBIOS_CONTROLLER_OFFSET) == 0);
        assert(memcmp(flash + USRPRG_CRC, saved_flash + USRPRG_CRC,
                      FLASH_OK_OFFSET - USRPRG_CRC) == 0);
        assert(memcmp(flash + FLASH_OK_OFFSET + 4096u,
                      saved_flash + FLASH_OK_OFFSET + 4096u,
                      sizeof(flash) - FLASH_OK_OFFSET - 4096u) == 0);
        assert(pogo_flash_file_delete(20) == POGO_FLASH_FILE_OK);
    }
    const unsigned bad_sizes[] = {0u, 1u, 3u, POGOBIOS_CONTROLLER_SIZE + 1u};
    for (unsigned i = 0; i < sizeof(bad_sizes) / sizeof(bad_sizes[0]); ++i) {
        assert(pogo_flash_file_create_bytes(20, "controller", bad_sizes[i], 1,
                                           controller_image) == POGO_FLASH_FILE_OK);
        expect_load_rejected("load controller", true);
        assert(pogo_flash_file_delete(20) == POGO_FLASH_FILE_OK);
    }
    assert(pogo_flash_file_create_bytes(20, "controller", 513u, 1,
                                       controller_image) == POGO_FLASH_FILE_OK);
    expect_load_rejected("load controller", false); /* UART-only, like files. */
    command("put deadbeef 21 pending 48 00000000 raw", true);
    assert(bios_pffs_busy());
    expect_load_rejected("load controller", true);
    command("abort deadbeef", true);
    assert(!bios_pffs_busy());

    pogo_flash_file_info_t info;
    assert(pogo_flash_file_find(20, &info) == POGO_FLASH_FILE_OK);
    uint32_t source = POGOBOT_USER_FLASH_START_OFFSET + info.first_page * 256u;
    flash[source + 42u] ^= 1u;
    expect_load_rejected("load 20", true); /* Source CRC. */
    flash[source + 42u] ^= 1u;
    uint32_t catalog_crc = POGOBOT_USER_FLASH_START_OFFSET +
        ((20u - 1u) / 5u) * 4096u + 252u;
    flash[catalog_crc] ^= 1u;
    expect_load_rejected("load 20", true); /* Catalog CRC. */
    flash[catalog_crc] ^= 1u;

    for (unsigned fault = 0; fault < 7u; ++fault) {
        seed_old_controller();
        unsigned written = controller_programs, reset = ir_resets;
        marker_erase_fails = fault == 0;
        controller_erase_fails = fault == 2;
        late_corruption = fault == 4;
        failed_program_address = fault == 1 ? FLASH_V2_PARTIAL_OFFSET :
            fault == 3 ? POGOBIOS_CONTROLLER_OFFSET :
            fault == 6 ? FLASH_OK_OFFSET : UINT32_MAX;
        if (fault == 5)
            corrupt_source_address = source + 256u + 8u;
        command("load controller", true);
        assert(check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET) == (fault == 0));
        assert(ir_resets == reset + (fault != 0));
        if (fault <= 1u) {
            assert(controller_programs == written);
            assert(flash[POGOBIOS_CONTROLLER_OFFSET] == 0xa5);
        } else {
            assert(check_flash_state(FLASH_IS_PARTIAL, FLASH_V2_PARTIAL_OFFSET));
        }
        if (fault == 5)
            flash[source + 256u + 8u] ^= 1u;
        marker_erase_fails = controller_erase_fails = late_corruption = false;
        failed_program_address = UINT32_MAX;
    }
    /* Retry after a failed completion marker installs and commits normally. */
    command("load controller", true);
    assert(check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET));
    assert(pogo_flash_file_delete(20) == POGO_FLASH_FILE_OK);
    const char *bad_magic[] = {"\x7f" "ELF", "PFT1"};
    for (unsigned i = 0u; i < 2u; ++i) {
        memcpy(controller_image, bad_magic[i], 4u);
        assert(pogo_flash_file_create_bytes(20, "controller", 513u, 1,
                                           controller_image) == POGO_FLASH_FILE_OK);
        expect_load_rejected("load 20", true);
        assert(pogo_flash_file_delete(20) == POGO_FLASH_FILE_OK);
    }
    assert(pogo_flash_file_create_bytes(20, "controller", 513u, PFT_FORMAT,
                                       controller_image) == POGO_FLASH_FILE_OK);
    expect_load_rejected("load 20", true);
    assert(pogo_flash_file_delete(20) == POGO_FLASH_FILE_OK);
    pogo_flash_log_t log;
    assert(pogo_flash_log_initialize(&log, 20, "events", 1, false, NULL) ==
           POGO_FLASH_LOG_OK);
    expect_load_rejected("load events", true);
    puts("PFFS controller install/fault injection: PASS");
}

static void expect_start_failed(const char *line) {
    unsigned starts = controller_starts;
    expect_load_rejected(line, false);
    assert(controller_starts == starts);
    assert(led_color[0] == 50 && led_color[1] == 0 && led_color[2] == 0);
    assert(memcmp(led_color, blink_color, sizeof(led_color)) == 0);
}

/* Exercise the actual IR-enabled dispatcher and loader together, including
 * exact name lookup and refusal to boot an older image after a failure. */
static void controller_start_selftest(void) {
    char longest[POGO_FLASH_FILE_MAX_NAME + 1u];
    memset(longest, 'x', sizeof(longest) - 1u);
    longest[sizeof(longest) - 1u] = '\0';
    assert(bios_pffs_controller_name_valid(longest));
    assert(!bios_pffs_controller_name_valid(NULL));
    assert(!bios_pffs_controller_name_valid(""));
    assert(!bios_pffs_controller_name_valid("two words"));
    assert(!bios_pffs_controller_name_valid("line\nbreak"));
    assert(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
    bios_pffs_init();
    seed_old_controller();
    controller_image[0] = 0x6f;
    assert(pogo_flash_file_create_bytes(7, "controller", 513u, 1u,
                                       controller_image) == POGO_FLASH_FILE_OK);
    expect_start_failed("load_run missing");
    expect_start_failed("load_run 7"); /* ID 7 exists, but this name does not. */
    expect_start_failed("load_run");
    expect_start_failed("load_run controller extra");
    expect_start_failed("load_run abcdefghijklmnopqrstuvwxyz1234567");
    command("load_run controller", false);
    assert(controller_starts == 1u);
    assert(memcmp(flash + POGOBIOS_CONTROLLER_OFFSET, controller_image, 513u) == 0);
    /* A numeric name must select its named file, not the different ID 7. */
    assert(pogo_flash_file_create_bytes(8, "7", 257u, 1u,
                                       controller_image) == POGO_FLASH_FILE_OK);
    unsigned written = controller_programs;
    command("load_run 7", false);
    assert(controller_starts == 2u && controller_programs == written + 2u);
    pogo_flash_file_info_t info;
    assert(pogo_flash_file_find(7, &info) == POGO_FLASH_FILE_OK);
    uint32_t source = POGOBOT_USER_FLASH_START_OFFSET + info.first_page * 256u;
    flash[source + 3u] ^= 1u;
    expect_start_failed("load_run controller");
    flash[source + 3u] ^= 1u;
    failed_program_address = POGOBIOS_CONTROLLER_OFFSET;
    command("load_run controller", false);
    assert(controller_starts == 2u && !check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET));
    assert(led_color[0] == 50u && blink_color[0] == 50u);
    failed_program_address = UINT32_MAX;
    command("load_run controller", false);
    assert(controller_starts == 3u);
    puts("PFFS named IR controller start/fault injection: PASS");
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
    /* BIOS compaction is unavailable; the shared library still supports
     * maintenance applications. Rejecting the old command must not move data. */
    assert(pogo_flash_file_find(2, &info) == POGO_FLASH_FILE_OK);
    uint16_t original_page = info.first_page;
    unsigned before_programs = programs;
    before = erases;
    char *defrag_args[] = {"YES"};
    assert(bios_console_dispatch("defrag", 1, defrag_args, true) == NULL);
    assert(!bios_pffs_busy() && erases == before && programs == before_programs);
    assert(pogo_flash_file_find(2, &info) == POGO_FLASH_FILE_OK &&
           info.first_page == original_page);
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
    command("load_run binary", false);
    assert(controller_starts == 0u && bios_pffs_busy());
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
    /* A timed-out radio reservation continues to exclude UART writers and
     * controller installation until the explicit local abort releases it. */
    radio_pending = true;
    before = erases;
    assert(bios_pffs_busy());
    command("format YES", true);
    command("serialboot", true);
    command("load 2", true);
    command("load_run later", false);
    assert(controller_starts == 0u);
    assert(erases == before && dangerous_calls == 1u && bios_pffs_busy());
    command("ir_file_abort", true);
    assert(!bios_pffs_busy());
    controller_selftest();
    controller_start_selftest();
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
