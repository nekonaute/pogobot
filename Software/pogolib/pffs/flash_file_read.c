/**
 * @file flash_file_read.c
 * @brief Minimal non-CRC reader for the bounded Pogobot flash-file catalog.
 *
 * This translation unit contains every helper required by the direct-ID fast
 * path. Keeping CRC, name scanning, and mutation in other objects lets a static
 * linker omit those features from small read-only mission firmware.
 */
#include "flash_file_internal.h"

#include "flash_platform.h"

#include <string.h>

/** Four-byte discriminator at the beginning of each catalog page. */
static const uint8_t catalog_magic[4] = {'P', 'F', 'F', 'S'};

/* Fail at build time if a linked SDK exposes a different flash geometry.
 * The page API's uint16_t indices are required to reach the final data page. */
_Static_assert(POGO_FLASH_FILE_PAGE_SIZE == POGOBOT_USER_FLASH_PAGE_SIZE &&
    POGO_FLASH_FILE_USER_PAGES == POGOBOT_USER_FLASH_PAGE_COUNT,
    "PFFS v4 requires the extended SDK user-flash geometry");

/* Compile-time checks tie the arithmetic below to the serialized layout. A
 * layout change must update the constants and cannot silently leave unused or
 * overlapping bytes in a catalog page. */
_Static_assert(POGO_FLASH_FILE_CATALOG_HEADER_SIZE +
    POGO_FLASH_FILE_ENTRIES_PER_CATALOG * POGO_FLASH_FILE_ENTRY_SIZE +
    sizeof(uint32_t) == POGO_FLASH_FILE_PAGE_SIZE,
    "catalog layout must fill exactly one flash page");
_Static_assert(POGO_FLASH_FILE_MAX_FILES ==
    POGO_FLASH_FILE_CATALOG_PAGES * POGO_FLASH_FILE_ENTRIES_PER_CATALOG,
    "stable IDs must map exactly onto catalog slots");
_Static_assert(POGO_FLASH_FILE_ERASE_SECTOR_PAGES *
    POGO_FLASH_FILE_PAGE_SIZE == 4096 &&
    POGO_FLASH_FILE_DATA_FIRST_PAGE ==
        POGO_FLASH_FILE_CATALOG_PAGES * POGO_FLASH_FILE_ERASE_SECTOR_PAGES &&
    POGO_FLASH_FILE_MAX_PAGES ==
        POGO_FLASH_FILE_USER_PAGES - POGO_FLASH_FILE_DATA_FIRST_PAGE,
    "catalogs and data extents must exactly cover user flash");
_Static_assert(POGO_FLASH_FILE_MAX_BYTES ==
    POGO_FLASH_FILE_MAX_PAGES * POGO_FLASH_FILE_PAGE_SIZE &&
    POGO_FLASH_FILE_MAX_BYTES <= POGO_FLASH_FILE_LENGTH_MASK &&
    POGO_FLASH_FILE_USER_PAGES / POGO_FLASH_FILE_ERASE_SECTOR_PAGES <= 512,
    "byte lengths and sector addresses must fit the packed extent");

uint16_t pogo_flash_file_internal_get_u16(const uint8_t *p) {
    /* Byte assembly is valid for unaligned buffers and every host endianness. */
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

uint32_t pogo_flash_file_internal_get_u32(const uint8_t *p) {
    /* Keep every shift unsigned so promotion cannot invoke signed overflow. */
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool pogo_flash_file_internal_catalog_header_valid(
    const uint8_t page[POGO_FLASH_FILE_PAGE_SIZE],
    uint8_t expected_catalog_index) {
    /* Header byte 7 is reserved. Requiring zero makes future incompatible
     * layouts fail explicitly instead of being misread as version 4. */
    return memcmp(page, catalog_magic, sizeof(catalog_magic)) == 0 &&
        page[4] == POGO_FLASH_FILE_CATALOG_VERSION &&
        page[5] == expected_catalog_index &&
        page[6] == POGO_FLASH_FILE_ENTRIES_PER_CATALOG &&
        page[7] == 0u;
}

bool pogo_flash_file_internal_page_is_blank(
    const uint8_t page[POGO_FLASH_FILE_PAGE_SIZE]) {
    /* Real erased NOR flash reads as 0xff. Some simulator allocations begin as
     * zero. No other uniform value is a valid first-use representation. */
    bool all_zero = true;
    bool all_erased = true;
    for (size_t i = 0u; i < POGO_FLASH_FILE_PAGE_SIZE; ++i) {
        all_zero = all_zero && page[i] == 0u;
        all_erased = all_erased && page[i] == 0xffu;
    }
    return all_zero || all_erased;
}

bool pogo_flash_file_internal_decode_entry(
    const uint8_t *entry,
    uint8_t expected_id,
    pogo_flash_file_info_t *info) {
    /* Decode fixed-width fields before validation. Reading bytes
     * directly avoids alignment and packed-structure assumptions. */
    uint32_t extent = pogo_flash_file_internal_get_u32(entry + 4);
    uint32_t bytes = extent & POGO_FLASH_FILE_LENGTH_MASK;
    uint16_t first_page = (uint16_t)(((extent >> 21) & 0x1ffu) *
        POGO_FLASH_FILE_ERASE_SECTOR_PAGES);
    uint16_t page_count = pogo_flash_file_internal_pages_for_bytes(bytes);
    uint8_t flags = entry[1];
    uint8_t name_length = (uint8_t)((flags >> 1) & 0x3fu);
    /* Widen the sum so a damaged 16-bit extent cannot wrap. */
    unsigned end_page = (unsigned)first_page + (unsigned)page_count;
    if ((extent & POGO_FLASH_FILE_EXTENT_RESERVED) != 0u ||
        bytes > POGO_FLASH_FILE_MAX_BYTES ||
        (pogo_flash_file_internal_get_u16(entry + 2) == 0x8001u &&
         (bytes == 0u || bytes % POGO_FLASH_FILE_PAGE_SIZE != 0u)) ||
        (flags & POGO_FLASH_FILE_ENTRY_IN_USE) == 0u ||
        (flags & 0x80u) != 0u || entry[0] != expected_id || page_count == 0u ||
        page_count > POGO_FLASH_FILE_MAX_PAGES ||
        first_page < POGO_FLASH_FILE_DATA_FIRST_PAGE ||
        end_page > POGO_FLASH_FILE_USER_PAGES ||
        first_page % POGO_FLASH_FILE_ERASE_SECTOR_PAGES != 0u ||
        name_length > POGO_FLASH_FILE_MAX_NAME) {
        return false;
    }
    if (info != NULL) {
        /* Zeroing guarantees deterministic padding and terminates an empty or
         * short optional name before the serialized bytes are copied. */
        memset(info, 0, sizeof(*info));
        info->id = expected_id;
        info->format_version = pogo_flash_file_internal_get_u16(entry + 2);
        info->first_page = first_page;
        info->page_count = page_count;
        info->size_bytes = bytes;
        info->name_length = name_length;
        info->generation = pogo_flash_file_internal_get_u32(entry + 8);
        info->data_crc32 = pogo_flash_file_internal_get_u32(entry + 12);
        memcpy(info->name, entry + 16, name_length);
        info->name[name_length] = '\0';
    }
    return true;
}

pogo_flash_file_status_t pogo_flash_file_internal_load_slot(
    uint8_t file_id,
    pogo_flash_file_info_t *info,
    uint8_t catalog_page[static POGO_FLASH_FILE_PAGE_SIZE]) {
    if (file_id == 0u || file_id > POGO_FLASH_FILE_MAX_FILES) {
        return POGO_FLASH_FILE_INVALID_ARGUMENT;
    }
    uint8_t *page = catalog_page; /* Short alias keeps offset code readable. */
    /* ID 1 maps to slot 0, ID 6 to slot 5. Division selects one of sixteen
     * catalog pages; modulo selects the fixed entry within that page. */
    uint8_t slot = (uint8_t)(file_id - 1u);
    uint8_t catalog_index = (uint8_t)(slot / POGO_FLASH_FILE_ENTRIES_PER_CATALOG);
    uint8_t entry_index = (uint8_t)(slot % POGO_FLASH_FILE_ENTRIES_PER_CATALOG);
    read_page_flash(pogo_flash_file_internal_catalog_page(catalog_index),
                    (char *)page);
    if (pogo_flash_file_internal_old_catalog(page)) return POGO_FLASH_FILE_UNSUPPORTED_FORMAT;
    if (!pogo_flash_file_internal_catalog_header_valid(page, catalog_index)) {
        /* Blank means the filesystem has never been formatted. Any other
         * invalid header is data/corruption and must not be auto-erased here. */
        return pogo_flash_file_internal_page_is_blank(page) ?
            POGO_FLASH_FILE_UNFORMATTED :
            POGO_FLASH_FILE_CORRUPT_CATALOG;
    }
    const uint8_t *entry = page + POGO_FLASH_FILE_CATALOG_HEADER_SIZE +
        (size_t)entry_index * POGO_FLASH_FILE_ENTRY_SIZE;
    if (entry[1] == 0u) {
        /* Zero is the canonical empty-slot representation created by format
         * and delete. Other flag values are validated as corruption below. */
        return POGO_FLASH_FILE_NOT_FOUND;
    }
    if (!pogo_flash_file_internal_decode_entry(entry, file_id, info)) {
        return POGO_FLASH_FILE_CORRUPT_CATALOG;
    }
    return POGO_FLASH_FILE_OK;
}

pogo_flash_file_status_t pogo_flash_file_find(
    uint8_t file_id,
    pogo_flash_file_info_t *info) {
    if (info == NULL) return POGO_FLASH_FILE_INVALID_ARGUMENT;
    /* The public find API does not expose raw catalog bytes, but the internal
     * loader requires a full page destination for the physical read. */
    uint8_t catalog_page[POGO_FLASH_FILE_PAGE_SIZE];
    return pogo_flash_file_internal_load_slot(file_id, info, catalog_page);
}

pogo_flash_file_status_t pogo_flash_file_read_page_fast(
    uint8_t file_id,
    uint16_t file_page,
    uint8_t output[POGO_FLASH_FILE_PAGE_SIZE],
    pogo_flash_file_info_t *info) {
    if (output == NULL) return POGO_FLASH_FILE_INVALID_ARGUMENT;
    pogo_flash_file_info_t decoded;
    /* Reuse the caller's output buffer for the catalog read. On success it is
     * immediately overwritten by the requested data page. This saves a second
     * 256-byte stack allocation on constrained firmware. */
    pogo_flash_file_status_t status = pogo_flash_file_internal_load_slot(
        file_id, &decoded, output);
    if (status != POGO_FLASH_FILE_OK) return status;
    /* file_page is relative to the file. The checked sum stays within the
     * platform's uint16_t page-address range. */
    if (file_page >= decoded.page_count) return POGO_FLASH_FILE_INVALID_ARGUMENT;
    read_page_flash((uint16_t)(decoded.first_page + file_page), (char *)output);
    if (info != NULL) *info = decoded;
    return POGO_FLASH_FILE_OK;
}
