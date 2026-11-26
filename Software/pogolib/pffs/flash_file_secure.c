/**
 * @file flash_file_secure.c
 * @brief CRC-validating reader kept separate from the minimal fast reader.
 *
 * Secure here means corruption-detecting, not authenticated or encrypted. A
 * CRC detects accidental damage and interrupted writes but provides no defense
 * against an attacker who can rewrite both payload and checksum.
 */
#include "flash_file_internal.h"
#include "flash_log.h"

#include "flash_platform.h"

#include <stdint.h>

/** Incremental CRC-32/ISO-HDLC update using the reflected polynomial.
 *
 * The bitwise implementation avoids a 1 KiB lookup table. Callers provide the
 * running state so a multi-page extent can be processed with one page buffer.
 */
static uint32_t crc32_update(uint32_t crc, const uint8_t *bytes, size_t length) {
    for (size_t i = 0u; i < length; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            /* Expands the low bit to either 0x00000000 or 0xffffffff, avoiding
             * a data-dependent branch in the inner loop. */
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    return crc;
}

static bool catalog_crc_valid(const uint8_t page[POGO_FLASH_FILE_PAGE_SIZE]) {
    /* The stored four-byte checksum is excluded from its own calculation. */
    uint32_t calculated = crc32_update(
        UINT32_MAX, page, POGO_FLASH_FILE_CATALOG_CRC_OFFSET) ^ UINT32_MAX;
    return calculated == pogo_flash_file_internal_get_u32(
        page + POGO_FLASH_FILE_CATALOG_CRC_OFFSET);
}

pogo_flash_file_status_t pogo_flash_file_read_page_secure(
    uint8_t file_id,
    uint16_t file_page,
    uint8_t output[POGO_FLASH_FILE_PAGE_SIZE],
    pogo_flash_file_info_t *info) {
    if (output == NULL) return POGO_FLASH_FILE_INVALID_ARGUMENT;
    pogo_flash_file_info_t decoded;
    /* As in the fast reader, output first holds the selected catalog page. It
     * must remain untouched until its CRC has been checked below. */
    pogo_flash_file_status_t status = pogo_flash_file_internal_load_slot(
        file_id, &decoded, output);
    if (status != POGO_FLASH_FILE_OK) return status;
    if (file_page >= decoded.page_count) return POGO_FLASH_FILE_INVALID_ARGUMENT;
    if (!catalog_crc_valid(output)) return POGO_FLASH_FILE_CORRUPT_CATALOG;
    /* Append-only logs intentionally leave the catalog's whole-file CRC
     * unchanged. Their reader verifies each committed page independently. */
    if (decoded.format_version == POGO_FLASH_LOG_FORMAT_VERSION) {
        return POGO_FLASH_FILE_UNSUPPORTED_FORMAT;
    }

    /* CRC coverage includes padding and unused bytes within every allocated
     * page. Writers therefore require callers to define all 256 bytes/page. */
    uint32_t crc = UINT32_MAX;
    for (uint16_t i = 0u; i < decoded.page_count; ++i) {
        read_page_flash((uint16_t)(decoded.first_page + i), (char *)output);
        crc = crc32_update(crc, output, POGO_FLASH_FILE_PAGE_SIZE);
    }
    crc ^= UINT32_MAX;
    if (crc != decoded.data_crc32) return POGO_FLASH_FILE_BAD_CHECKSUM;
    /* The loop leaves the final file page in output. Avoid a redundant read
     * when that is the page the caller requested. */
    if (file_page != (uint16_t)(decoded.page_count - 1u)) {
        read_page_flash((uint16_t)(decoded.first_page + file_page), (char *)output);
    }
    if (info != NULL) *info = decoded;
    return POGO_FLASH_FILE_OK;
}
