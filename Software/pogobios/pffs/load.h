/* Private BIOS controller installer. Firmware files are raw SDK binaries
 * linked for 0x260000, not ELF files or UART protocol envelopes. */
#ifndef POGOBIOS_PFFS_LOAD_H
#define POGOBIOS_PFFS_LOAD_H
#include <stdbool.h>
#include <stdint.h>
#include <pffs/flash_file.h>

#define POGOBIOS_CONTROLLER_OFFSET 0x60000u
#define POGOBIOS_CONTROLLER_SIZE   0x20000u

/* Remote commands carry one bounded token. Numeric labels are names too;
 * control/whitespace bytes must never introduce another BIOS argument. */
static inline bool bios_pffs_controller_name_valid(const char *name) {
    if (name == NULL) return false;
    for (unsigned i = 0u; i <= POGO_FLASH_FILE_MAX_NAME; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (c == 0u) return i != 0u;
        if (c < 33u || c > 126u) return false;
    }
    return false;
}

/* Serialized console callers lend their existing page buffer. Success leaves
 * the installed image valid, as serialboot does; the user then executes run.
 * Once installation starts, a failure leaves the image invalid for boot. */
bool bios_pffs_load_controller(uint8_t file_id,
                               uint8_t scratch[POGO_FLASH_FILE_PAGE_SIZE]);
#endif
