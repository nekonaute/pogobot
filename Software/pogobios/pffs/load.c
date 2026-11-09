/* Install a PFFS controller into the same application slot as serialboot.
 * This runs from the BIOS's SRAM copy, including when replacing normal BIOS.
 * The PFFS source and bootloader/gateware are never erased by this command. */
#include "load.h"
#ifndef REMOCON
#include <generated/mem.h>
#include <generated/soc.h>
#include <libbase/spiflash.h>
#include <stdio.h>
#include <string.h>
#include <pogobot.h>
#include "boot.h"
#include "ir_boot.h"
#include "transfer.h"

_Static_assert(POGOBIOS_CONTROLLER_OFFSET + POGOBIOS_CONTROLLER_SIZE <=
                   USRPRG_CRC,
               "controller must not overlap CRC/validity metadata");
_Static_assert(POGOBIOS_CONTROLLER_OFFSET + POGOBIOS_CONTROLLER_SIZE <=
                   SPIFLASH_SIZE,
               "controller must fit physical flash");

static bool load_error(const char *reason) {
    printf("load: %s\n", reason);
    return false;
}

bool bios_pffs_load_controller(uint8_t file_id,
                               uint8_t scratch[POGO_FLASH_FILE_PAGE_SIZE]) {
    pogo_flash_file_info_t info;
    pogo_flash_file_status_t status = pogo_flash_file_find(file_id, &info);
    if (status != POGO_FLASH_FILE_OK)
        return load_error(pogo_flash_file_status_string(status));
    /* Check bounds before the whole-file CRC scan. The ceiling also bounds
     * padded-page copies and excludes oversized reserved allocations. */
    if (info.size_bytes < 4u || info.size_bytes > POGOBIOS_CONTROLLER_SIZE ||
        info.page_count != (info.size_bytes + POGO_FLASH_FILE_PAGE_SIZE - 1u) /
                               POGO_FLASH_FILE_PAGE_SIZE)
        return load_error("controller must be 4..131072 bytes");
    if (info.format_version == POGO_FLASH_LOG_FORMAT_VERSION ||
        info.format_version == PFT_FORMAT)
        return load_error("use a raw firmware.bin, not a log or envelope");
    /* Validate the selected catalog and every source page before changing
     * application bytes, the validity marker, or resumable IR state. */
    puts("load: verifying stored controller");
    status = pogo_flash_file_read_page_secure(file_id, 0u, scratch, &info);
    if (status != POGO_FLASH_FILE_OK)
        return load_error(pogo_flash_file_status_string(status));
    if (memcmp(scratch, "\x7f" "ELF", 4u) == 0 ||
        memcmp(scratch, "PFT1", 4u) == 0)
        return load_error("use a raw firmware.bin, not ELF or an envelope");

    if (!flash_invalidate_user_image())
        return load_error("validity erase failed; controller unchanged");
    ir_boot_reset_v2_upload();
    write_to_flash(FLASH_V2_PARTIAL_OFFSET,
                   (const unsigned char *)FLASH_IS_PARTIAL,
                   strlen(FLASH_IS_PARTIAL));
    if (!check_flash_state(FLASH_IS_PARTIAL, FLASH_V2_PARTIAL_OFFSET))
        return load_error("partial marker write failed; controller unchanged");

    printf("load: installing %lu bytes at 0x%lx\n",
           (unsigned long)info.size_bytes,
           (unsigned long)(SPIFLASH_BASE + POGOBIOS_CONTROLLER_OFFSET));
    uint32_t extent = (uint32_t)info.page_count * POGO_FLASH_FILE_PAGE_SIZE;
    for (uint32_t offset = 0u; offset < extent;
         offset += SPIFLASH_SECTOR_SIZE)
        erase_flash_sector(POGOBIOS_CONTROLLER_OFFSET + offset);
    /* The secure reader left the first page in scratch. No other console
     * caller can mutate the file while this synchronous operation runs. */
    for (uint16_t page = 0u; page < info.page_count; ++page) {
        if (page != 0u)
            read_page_flash((uint16_t)(info.first_page + page), (char *)scratch);
        uint32_t address = POGOBIOS_CONTROLLER_OFFSET +
                           (uint32_t)page * POGO_FLASH_FILE_PAGE_SIZE;
        write_to_flash(address, scratch, POGO_FLASH_FILE_PAGE_SIZE);
        for (unsigned i = 0u; i < POGO_FLASH_FILE_PAGE_SIZE; ++i) {
            if (POGOBIOS_FLASH_READ(address + i) != scratch[i])
                return load_error("controller write verification failed");
        }
    }
    /* Re-read the complete installed extent after all writes. Its CRC covers
     * the same padding as PFFS, catching source changes during the copy and
     * faults that damage previously programmed pages. No second buffer/table. */
    uint32_t crc = UINT32_MAX;
    for (uint32_t offset = 0u; offset < extent; ++offset) {
        crc ^= POGOBIOS_FLASH_READ(POGOBIOS_CONTROLLER_OFFSET + offset);
        for (unsigned bit = 0u; bit < 8u; ++bit)
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0u - (crc & 1u)));
    }
    if ((crc ^ UINT32_MAX) != info.data_crc32)
        return load_error("installed controller checksum failed");
    /* Commit last. A failed marker leaves the adjacent partial marker intact,
     * so the existing idle status reports orange and run refuses the image. */
    write_to_flash(FLASH_OK_OFFSET, (const unsigned char *)FLASH_IS_OK,
                   strlen(FLASH_IS_OK));
    if (!check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET))
        return load_error("completion marker write failed");
    puts("load: controller verified and installed; execute 'run' to start");
    return true;
}
#endif
