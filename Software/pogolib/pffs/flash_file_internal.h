#ifndef POGO_UTILS_FLASH_FILE_INTERNAL_H
#define POGO_UTILS_FLASH_FILE_INTERNAL_H

/**
 * @file flash_file_internal.h
 * @brief Serialized catalog layout and helpers shared by flash-file modules.
 *
 * This header is private to pogolib. Applications should include
 * flash_file.h and must not depend on byte offsets below.
 *
 * Each 256-byte catalog page is encoded explicitly in little-endian order:
 *
 *   0..3     ASCII magic "PFFS"
 *   4        catalog format version (4 for exact byte lengths)
 *   5        catalog index (0..15)
 *   6        number of fixed entries (5)
 *   7        reserved, must be zero
 *   8..11    catalog generation counter
 *   12..251  five 48-byte entries
 *   252..255 CRC-32 of bytes 0..251
 *
 * Entry layout relative to its 48-byte slot:
 *
 *   0        stable file ID
 *   1        bit 0: in use; bits 1..6: name length; bit 7: reserved
 *   2..3     caller-owned payload format version
 *   4..7     packed extent: bits 0..20 byte length, bits 21..29 first sector,
 *            bits 30..31 reserved zero. Page count = max(1, ceil(bytes/256)).
 *   8..11    file replacement generation
 *   12..15   CRC-32 over every byte in the data extent
 *   16..47   optional name bytes, not NUL-terminated on flash
 */
#include "flash_file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    POGO_FLASH_FILE_CATALOG_VERSION = 4,     /**< Serialized schema version. */
    POGO_FLASH_FILE_CATALOG_HEADER_SIZE = 12, /**< Bytes before entry zero. */
    POGO_FLASH_FILE_ENTRY_SIZE = 48,         /**< Serialized bytes per slot. */
    POGO_FLASH_FILE_ENTRIES_PER_CATALOG = 5, /**< Fixed slots in one page. */
    POGO_FLASH_FILE_CATALOG_CRC_OFFSET = 252, /**< Stored CRC byte offset. */
    POGO_FLASH_FILE_ENTRY_IN_USE = 1         /**< Low bit marks active slots. */
};

/* Sector alignment saves address bits, retaining 32-byte names and 80 IDs. */
#define POGO_FLASH_FILE_LENGTH_MASK UINT32_C(0x001fffff)
#define POGO_FLASH_FILE_EXTENT_RESERVED UINT32_C(0xc0000000)
/** Caller validates bytes <= MAX_BYTES before rounding; zero still owns a page. */
static inline uint16_t pogo_flash_file_internal_pages_for_bytes(uint32_t bytes) {
    return bytes == 0u ? 1u : (uint16_t)((bytes + 255u) / 256u);
}
/** Caller supplies a validated sector-aligned address and bounded byte length. */
static inline uint32_t pogo_flash_file_internal_pack_extent(uint16_t first_page,
                                                            uint32_t bytes) {
    return ((uint32_t)(first_page / POGO_FLASH_FILE_ERASE_SECTOR_PAGES) << 21) | bytes;
}

/** Identify incompatible versions without decoding or erasing their catalogs. */
static inline bool pogo_flash_file_internal_old_catalog(const uint8_t *page) {
    return page[0] == 'P' && page[1] == 'F' && page[2] == 'F' && page[3] == 'S' &&
        page[4] != 0u && page[4] != POGO_FLASH_FILE_CATALOG_VERSION;
}

/** Each catalog page is the first page of its own 4 KiB erase sector. */
static inline uint16_t pogo_flash_file_internal_catalog_page(uint8_t index) {
    return (uint16_t)index * POGO_FLASH_FILE_ERASE_SECTOR_PAGES;
}

/** Load and structurally decode the fixed slot selected by file_id.
 * catalog_page is required and receives the raw page. CRC is intentionally not
 * checked here so fast readers do not pull CRC code into their binaries. The
 * caller may pass NULL for info when it only needs validation/raw catalog data.
 */
pogo_flash_file_status_t pogo_flash_file_internal_load_slot(
    uint8_t file_id,
    pogo_flash_file_info_t *info,
    uint8_t catalog_page[static POGO_FLASH_FILE_PAGE_SIZE]);

/** Check magic, version, page identity, entry count, and reserved header byte.
 * This does not check the catalog CRC or inspect any entry.
 */
bool pogo_flash_file_internal_catalog_header_valid(
    const uint8_t page[POGO_FLASH_FILE_PAGE_SIZE],
    uint8_t expected_catalog_index);

/** Return true for a virgin simulator page (all zeroes) or an erased hardware
 * flash page (all ones). Mixed contents are not considered blank.
 */
bool pogo_flash_file_internal_page_is_blank(
    const uint8_t page[POGO_FLASH_FILE_PAGE_SIZE]);

/** Validate one occupied serialized entry and optionally decode it.
 *
 * Validation covers ID/flags/reserved fields, name length, page-count bounds,
 * and physical extent bounds. Cross-entry overlap is a writer-side check and
 * is not performed here because the fast reader deliberately touches one slot.
 */
bool pogo_flash_file_internal_decode_entry(
    const uint8_t *entry,
    uint8_t expected_id,
    pogo_flash_file_info_t *info);

/** Decode unaligned little-endian integers without casting serialized bytes. */
uint16_t pogo_flash_file_internal_get_u16(const uint8_t *p);
uint32_t pogo_flash_file_internal_get_u32(const uint8_t *p);

/* Infrequent log setup/clear operations live with the existing writer so the
 * hot append path does not carry catalog allocation or sector-erase logic. */
pogo_flash_file_status_t pogo_flash_file_internal_create_log(
    uint8_t file_id, const char *name, uint16_t page_count, bool *formatted);
pogo_flash_file_status_t pogo_flash_file_internal_clear_log(uint8_t file_id);

#endif /* POGO_UTILS_FLASH_FILE_INTERNAL_H */
