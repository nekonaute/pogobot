/**
 * @file flash_file_name.c
 * @brief Optional human-readable lookup, separate from the ID fast path.
 *
 * A name lookup is a convenience for diagnostics and infrequent discovery. It
 * performs a bounded linear scan of eighty slots and never changes the fact that
 * numeric IDs define persistent identity and placement.
 */
#include "flash_file_internal.h"

#include "flash_platform.h"

#include <string.h>

/** Measure at most MAX_NAME+1 bytes.
 *
 * Returning MAX_NAME+1 distinguishes an overlong or unterminated string from a
 * legal 32-byte name without calling strlen on unbounded caller memory.
 */
static size_t bounded_name_length(const char *name) {
    if (name == NULL) return 0u;
    size_t length = 0u;
    while (length <= POGO_FLASH_FILE_MAX_NAME && name[length] != '\0') ++length;
    return length;
}

pogo_flash_file_status_t pogo_flash_file_find_by_name(
    const char *name,
    pogo_flash_file_info_t *info) {
    size_t name_length = bounded_name_length(name);
    if (info == NULL || name_length == 0u ||
        name_length > POGO_FLASH_FILE_MAX_NAME) {
        return POGO_FLASH_FILE_INVALID_ARGUMENT;
    }
    uint8_t page[POGO_FLASH_FILE_PAGE_SIZE];
    /* Read each catalog once, then inspect its five entries. At most sixteen
     * physical reads are performed; ID lookup remains the two-read fast path. */
    for (uint8_t catalog_index = 0u;
         catalog_index < POGO_FLASH_FILE_CATALOG_PAGES; ++catalog_index) {
        read_page_flash(pogo_flash_file_internal_catalog_page(catalog_index),
                        (char *)page);
        if (pogo_flash_file_internal_old_catalog(page)) return POGO_FLASH_FILE_UNSUPPORTED_FORMAT;
        if (!pogo_flash_file_internal_catalog_header_valid(page, catalog_index)) {
            /* Page zero blank means no filesystem. Any later blank/invalid
             * catalog after a valid page zero is an incomplete format. */
            return pogo_flash_file_internal_page_is_blank(page) &&
                catalog_index == 0u ? POGO_FLASH_FILE_UNFORMATTED :
                    POGO_FLASH_FILE_CORRUPT_CATALOG;
        }
        for (uint8_t entry_index = 0u;
             entry_index < POGO_FLASH_FILE_ENTRIES_PER_CATALOG; ++entry_index) {
            uint8_t id = (uint8_t)(catalog_index *
                POGO_FLASH_FILE_ENTRIES_PER_CATALOG + entry_index + 1u);
            const uint8_t *entry = page + POGO_FLASH_FILE_CATALOG_HEADER_SIZE +
                (size_t)entry_index * POGO_FLASH_FILE_ENTRY_SIZE;
            if (entry[1] == 0u) continue;
            pogo_flash_file_info_t candidate;
            /* Decode before comparing so malformed bounds, flags, or reserved
             * fields cannot be hidden by a coincidentally matching name. */
            if (!pogo_flash_file_internal_decode_entry(entry, id, &candidate)) {
                return POGO_FLASH_FILE_CORRUPT_CATALOG;
            }
            if (candidate.name_length == name_length &&
                memcmp(candidate.name, name, name_length) == 0) {
                /* Names are unique when created, so the first match is the only
                 * valid match in a catalog produced by this writer. */
                *info = candidate;
                return POGO_FLASH_FILE_OK;
            }
        }
    }
    return POGO_FLASH_FILE_NOT_FOUND;
}
