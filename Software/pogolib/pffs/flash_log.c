/**
 * @file flash_log.c
 * @brief Page-at-a-time append and recovery for bounded PFFS byte streams.
 *
 * Page bytes: 0..3 "PLOG", 4 version, 5 zero-based page index, 6 payload
 * length, 7 reserved zero, 8..11 little-endian CRC-32 of bytes 0..7 and the
 * used payload, 12..255 payload followed by 0xff padding. A complete page is
 * the unit of durability. The PFFS catalog retains its original layout; its
 * whole-file CRC field is unused for this reserved payload format.
 */
#include "flash_log.h"

#include "flash_file_internal.h"
#include "flash_platform.h"

#include <limits.h>
#include <string.h>

static const uint8_t log_magic[4] = {'P', 'L', 'O', 'G'};

_Static_assert(POGO_FLASH_LOG_PAYLOAD_SIZE > 0u &&
    POGO_FLASH_LOG_PAYLOAD_SIZE <= UINT8_MAX,
    "the one-byte page length must encode every possible payload");
_Static_assert(POGO_FLASH_LOG_MAX_PAGES == UINT8_MAX + 1u,
    "the one-byte page index must cover every log page");

/** Bitwise CRC-32 avoids a 1 KiB table in the firmware image. */
static uint32_t crc_update(uint32_t crc, const uint8_t *bytes, size_t length) {
    for (size_t i = 0u; i < length; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    return crc;
}

static void put_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

/** Verify all catalogs before a log writes to an extent selected by them. */
static pogo_flash_log_status_t check_catalogs(uint8_t scratch[256]) {
    /* The 368-byte map checks all extents without pulling the writer's
     * catalog-validation object into read-only log firmware. */
    bool occupied_sectors[POGO_FLASH_FILE_USER_PAGES /
        POGO_FLASH_FILE_ERASE_SECTOR_PAGES] = {false};
    for (unsigned i = 0u; i < POGO_FLASH_FILE_CATALOG_PAGES; ++i) {
        occupied_sectors[i] = true;
    }
    for (uint8_t i = 0u; i < POGO_FLASH_FILE_CATALOG_PAGES; ++i) {
        read_page_flash(pogo_flash_file_internal_catalog_page(i),
                        (char *)scratch);
        if (pogo_flash_file_internal_old_catalog(scratch)) return POGO_FLASH_LOG_WRONG_FORMAT;
        if (!pogo_flash_file_internal_catalog_header_valid(scratch, i)) {
            return i == 0u && pogo_flash_file_internal_page_is_blank(scratch) ?
                POGO_FLASH_LOG_NOT_FOUND : POGO_FLASH_LOG_CORRUPT;
        }
        uint32_t calculated = crc_update(UINT32_MAX, scratch,
            POGO_FLASH_FILE_CATALOG_CRC_OFFSET) ^ UINT32_MAX;
        if (calculated != pogo_flash_file_internal_get_u32(
                scratch + POGO_FLASH_FILE_CATALOG_CRC_OFFSET)) {
            return POGO_FLASH_LOG_CORRUPT;
        }
        /* A catalog with a valid CRC can still encode an invalid or
         * overlapping extent. Check all slots before trusting one for writes. */
        for (uint8_t slot = 0u; slot < POGO_FLASH_FILE_ENTRIES_PER_CATALOG;
             ++slot) {
            const uint8_t *entry = scratch +
                POGO_FLASH_FILE_CATALOG_HEADER_SIZE +
                (size_t)slot * POGO_FLASH_FILE_ENTRY_SIZE;
            if (entry[1] == 0u) continue;
            uint8_t id = (uint8_t)(i * POGO_FLASH_FILE_ENTRIES_PER_CATALOG +
                                   slot + 1u);
            pogo_flash_file_info_t info;
            if (!pogo_flash_file_internal_decode_entry(entry, id, &info)) {
                return POGO_FLASH_LOG_CORRUPT;
            }
            unsigned first = info.first_page /
                POGO_FLASH_FILE_ERASE_SECTOR_PAGES;
            unsigned end = ((unsigned)info.first_page + info.page_count +
                POGO_FLASH_FILE_ERASE_SECTOR_PAGES - 1u) /
                POGO_FLASH_FILE_ERASE_SECTOR_PAGES;
            for (unsigned sector = first; sector < end; ++sector) {
                if (occupied_sectors[sector]) return POGO_FLASH_LOG_CORRUPT;
                occupied_sectors[sector] = true;
            }
        }
    }
    return POGO_FLASH_LOG_OK;
}

/** Erased hardware pages are 0xff. Unlike a virgin simulator catalog, an
 * allocated log sector is always explicitly erased before its first append. */
static bool erased_page(const uint8_t page[256]) {
    for (size_t i = 0u; i < POGO_FLASH_FILE_PAGE_SIZE; ++i) {
        if (page[i] != 0xffu) return false;
    }
    return true;
}

static bool valid_page(const uint8_t page[256], uint16_t page_index,
                       uint8_t *used) {
    uint8_t length = page[6];
    if (memcmp(page, log_magic, sizeof(log_magic)) != 0 ||
        page[4] != POGO_FLASH_LOG_PAGE_VERSION ||
        page[5] != (uint8_t)page_index ||
        length == 0u || length > POGO_FLASH_LOG_PAYLOAD_SIZE ||
        page[7] != 0u) return false;
    uint32_t crc = crc_update(UINT32_MAX, page, 8u);
    crc = crc_update(crc, page + POGO_FLASH_LOG_HEADER_SIZE, length) ^
        UINT32_MAX;
    if (crc != pogo_flash_file_internal_get_u32(page + 8u)) return false;
    for (size_t i = POGO_FLASH_LOG_HEADER_SIZE + length;
         i < POGO_FLASH_FILE_PAGE_SIZE; ++i) {
        if (page[i] != 0xffu) return false;
    }
    if (used != NULL) *used = length;
    return true;
}

static pogo_flash_log_status_t file_error(pogo_flash_file_status_t status) {
    switch (status) {
    case POGO_FLASH_FILE_OK: return POGO_FLASH_LOG_OK;
    case POGO_FLASH_FILE_NOT_FOUND:
    case POGO_FLASH_FILE_UNFORMATTED: return POGO_FLASH_LOG_NOT_FOUND;
    case POGO_FLASH_FILE_CORRUPT_CATALOG: return POGO_FLASH_LOG_CORRUPT;
    case POGO_FLASH_FILE_UNSUPPORTED_FORMAT: return POGO_FLASH_LOG_WRONG_FORMAT;
    case POGO_FLASH_FILE_NAME_EXISTS: return POGO_FLASH_LOG_NAME_EXISTS;
    case POGO_FLASH_FILE_NO_SPACE: return POGO_FLASH_LOG_NO_SPACE;
    case POGO_FLASH_FILE_INVALID_ARGUMENT:
    case POGO_FLASH_FILE_INVALID_SIZE: return POGO_FLASH_LOG_INVALID_ARGUMENT;
    case POGO_FLASH_FILE_VERIFY_FAILED: return POGO_FLASH_LOG_VERIFY_FAILED;
    default: return POGO_FLASH_LOG_FLASH_ERROR;
    }
}

static bool requested_name_matches(const char *name,
                                   const pogo_flash_file_info_t *info) {
    if (name == NULL || name[0] == '\0') return true;
    size_t length = 0u;
    while (length <= POGO_FLASH_FILE_MAX_NAME && name[length] != '\0') {
        ++length;
    }
    return length == info->name_length &&
        memcmp(name, info->name, length) == 0;
}

/** Bind only this file's entry: changes to other files must not interrupt two
 * independent logs. Include ID, type, extent, generation and name in the CRC. */
static uint32_t entry_identity(const uint8_t catalog[256], uint8_t file_id) {
    unsigned slot = (file_id - 1u) % POGO_FLASH_FILE_ENTRIES_PER_CATALOG;
    const uint8_t *entry = catalog + POGO_FLASH_FILE_CATALOG_HEADER_SIZE +
        slot * POGO_FLASH_FILE_ENTRY_SIZE;
    return crc_update(UINT32_MAX, entry, POGO_FLASH_FILE_ENTRY_SIZE) ^ UINT32_MAX;
}

/** Recheck ownership immediately before I/O using caller-provided scratch.
 * Append remains RAM-only; no global handle registry or writer dependency is
 * needed. CRC validation rejects accidental catalog damage before any write. */
static pogo_flash_log_status_t check_owner(const pogo_flash_log_t *log,
                                           uint8_t scratch[256]) {
    pogo_flash_file_info_t info;
    pogo_flash_file_status_t status = pogo_flash_file_internal_load_slot(
        log->file_id, &info, scratch);
    if (status == POGO_FLASH_FILE_NOT_FOUND || status == POGO_FLASH_FILE_UNFORMATTED)
        return POGO_FLASH_LOG_STALE_HANDLE;
    if (status != POGO_FLASH_FILE_OK) return file_error(status);
    uint32_t crc = crc_update(UINT32_MAX, scratch,
        POGO_FLASH_FILE_CATALOG_CRC_OFFSET) ^ UINT32_MAX;
    if (crc != pogo_flash_file_internal_get_u32(
            scratch + POGO_FLASH_FILE_CATALOG_CRC_OFFSET)) return POGO_FLASH_LOG_CORRUPT;
    if (info.format_version != POGO_FLASH_LOG_FORMAT_VERSION ||
        info.first_page != log->first_page || info.page_count != log->page_count ||
        entry_identity(scratch, log->file_id) != log->identity_crc)
        return POGO_FLASH_LOG_STALE_HANDLE;
    return POGO_FLASH_LOG_OK;
}

/* Keep stale ownership distinguishable from a torn page on subsequent calls:
 * clearing a stale handle must never erase its replacement's data. */
static pogo_flash_log_status_t failed_status(const pogo_flash_log_t *log) {
    return log->failed == 2u ? POGO_FLASH_LOG_STALE_HANDLE : POGO_FLASH_LOG_VERIFY_FAILED;
}

pogo_flash_log_status_t pogo_flash_log_open(pogo_flash_log_t *log,
                                            uint8_t file_id) {
    if (log == NULL || file_id == 0u ||
        file_id > POGO_FLASH_FILE_MAX_FILES) return POGO_FLASH_LOG_INVALID_ARGUMENT;
    memset(log, 0, sizeof(*log));
    pogo_flash_log_status_t status = check_catalogs(log->page);
    if (status != POGO_FLASH_LOG_OK) return status;
    pogo_flash_file_info_t info;
    status = file_error(pogo_flash_file_internal_load_slot(file_id, &info, log->page));
    if (status != POGO_FLASH_LOG_OK) return status;
    if (info.format_version != POGO_FLASH_LOG_FORMAT_VERSION) {
        return POGO_FLASH_LOG_WRONG_FORMAT;
    }
    if (info.page_count > POGO_FLASH_LOG_MAX_PAGES) {
        return POGO_FLASH_LOG_WRONG_FORMAT;
    }
    log->file_id = file_id;
    log->first_page = info.first_page;
    log->page_count = info.page_count;
    log->identity_crc = entry_identity(log->page, file_id);
    /* A valid log consists of a contiguous prefix of committed pages followed
     * only by erased pages. A torn page fails closed; explicit clear is needed
     * before writing to that sector again. */
    bool seen_erased = false;
    for (uint16_t i = 0u; i < info.page_count; ++i) {
        uint8_t used = 0u;
        read_page_flash((uint16_t)(info.first_page + i), (char *)log->page);
        if (erased_page(log->page)) {
            seen_erased = true;
        } else if (seen_erased || !valid_page(log->page, i, &used)) {
            memset(log->page, 0xff, sizeof(log->page));
            /* Preserve the valid prefix for diagnostic readback, but prevent
             * any further write until clear() erases the damaged sector. */
            log->ready = 1u;
            log->failed = 1u;
            return POGO_FLASH_LOG_CORRUPT;
        } else {
            log->next_page = (uint16_t)(i + 1u);
            log->committed_bytes += used;
        }
    }
    memset(log->page, 0xff, sizeof(log->page));
    log->ready = 1u;
    return POGO_FLASH_LOG_OK;
}

pogo_flash_log_status_t pogo_flash_log_initialize(
    pogo_flash_log_t *log, uint8_t file_id, const char *name,
    uint16_t page_count, bool clear_existing, bool *formatted) {
    if (formatted != NULL) *formatted = false;
    if (log == NULL || file_id == 0u ||
        file_id > POGO_FLASH_FILE_MAX_FILES || page_count == 0u ||
        page_count > POGO_FLASH_LOG_MAX_PAGES) {
        return POGO_FLASH_LOG_INVALID_ARGUMENT;
    }
    /* The caller-provided cache is scratch during initialization too; avoid
     * another 256-byte local array on the robot's task stack. */
    pogo_flash_log_status_t catalog = check_catalogs(log->page);
    if (catalog == POGO_FLASH_LOG_WRONG_FORMAT) return catalog;
    pogo_flash_file_info_t info;
    pogo_flash_file_status_t found = catalog == POGO_FLASH_LOG_OK ?
        pogo_flash_file_find(file_id, &info) : POGO_FLASH_FILE_NOT_FOUND;
    if (catalog != POGO_FLASH_LOG_OK ||
        found == POGO_FLASH_FILE_NOT_FOUND ||
        found == POGO_FLASH_FILE_CORRUPT_CATALOG) {
        /* The writer owns the destructive missing/corrupt-catalog policy.
         * It reports whether it reformatted so the application can warn. */
        pogo_flash_file_status_t result = pogo_flash_file_internal_create_log(
            file_id, name, page_count, formatted);
        if (result != POGO_FLASH_FILE_OK) return file_error(result);
    } else if (found != POGO_FLASH_FILE_OK) {
        return file_error(found);
    } else {
        if (info.format_version != POGO_FLASH_LOG_FORMAT_VERSION ||
            info.page_count != page_count ||
            !requested_name_matches(name, &info)) {
            return POGO_FLASH_LOG_WRONG_FORMAT;
        }
        if (clear_existing) {
            pogo_flash_file_status_t result =
                pogo_flash_file_internal_clear_log(file_id);
            if (result != POGO_FLASH_FILE_OK) return file_error(result);
        }
    }
    return pogo_flash_log_open(log, file_id);
}

pogo_flash_log_status_t pogo_flash_log_clear(pogo_flash_log_t *log) {
    if (log == NULL || log->ready == 0u) return POGO_FLASH_LOG_INVALID_ARGUMENT;
    /* A conflicting writer can leave the same catalog identity in place.
     * Once ownership is lost, only an explicit reopen can authorize a clear. */
    if (log->failed == 2u) return POGO_FLASH_LOG_STALE_HANDLE;
    /* clear() discards cached bytes anyway, so reuse them as catalog scratch. */
    pogo_flash_log_status_t owner = check_owner(log, log->page);
    if (owner != POGO_FLASH_LOG_OK) {
        log->failed = owner == POGO_FLASH_LOG_STALE_HANDLE ? 2u : 1u;
        return owner;
    }
    uint8_t id = log->file_id;
    pogo_flash_file_status_t result = pogo_flash_file_internal_clear_log(id);
    if (result != POGO_FLASH_FILE_OK) {
        /* An interrupted multi-sector clear leaves an unknown mix of old and
         * erased pages; this handle must not append until reopened/repaired. */
        log->failed = 1u;
        return file_error(result);
    }
    return pogo_flash_log_open(log, id);
}

pogo_flash_log_status_t pogo_flash_log_append(
    pogo_flash_log_t *log, const void *bytes, size_t length, size_t *accepted) {
    if (accepted != NULL) *accepted = 0u;
    if (log == NULL || accepted == NULL || log->ready == 0u ||
        (length > 0u && bytes == NULL)) return POGO_FLASH_LOG_INVALID_ARGUMENT;
    if (log->failed != 0u) return failed_status(log);
    if (length == 0u) return POGO_FLASH_LOG_OK;
    if (log->next_page >= log->page_count) return POGO_FLASH_LOG_FULL;
    size_t space = POGO_FLASH_LOG_PAYLOAD_SIZE - log->used;
    size_t count = length < space ? length : space;
    memcpy(log->page + POGO_FLASH_LOG_HEADER_SIZE + log->used, bytes, count);
    log->used = (uint8_t)(log->used + count);
    *accepted = count;
    return count == length ? POGO_FLASH_LOG_OK : POGO_FLASH_LOG_NEEDS_SERVICE;
}

/** Complete the page header only when the user explicitly requests a write. */
static pogo_flash_log_status_t commit_page(pogo_flash_log_t *log) {
    if (log == NULL || log->ready == 0u) return POGO_FLASH_LOG_INVALID_ARGUMENT;
    if (log->failed != 0u) return failed_status(log);
    if (log->used == 0u) return POGO_FLASH_LOG_OK;
    if (log->next_page >= log->page_count) return POGO_FLASH_LOG_FULL;
    memcpy(log->page, log_magic, sizeof(log_magic));
    log->page[4] = POGO_FLASH_LOG_PAGE_VERSION;
    /* next_page may become 256 only after the final page is committed; the
     * capacity check above keeps every serialized index within one byte. */
    log->page[5] = (uint8_t)log->next_page;
    log->page[6] = log->used;
    log->page[7] = 0u;
    uint32_t crc = crc_update(UINT32_MAX, log->page, 8u);
    crc = crc_update(crc, log->page + POGO_FLASH_LOG_HEADER_SIZE,
                     log->used) ^ UINT32_MAX;
    put_u32(log->page + 8u, crc);
    uint8_t actual[POGO_FLASH_FILE_PAGE_SIZE];
    pogo_flash_log_status_t owner = check_owner(log, actual);
    if (owner != POGO_FLASH_LOG_OK) {
        log->failed = owner == POGO_FLASH_LOG_STALE_HANDLE ? 2u : 1u;
        return owner;
    }
    uint16_t physical = (uint16_t)(log->first_page + log->next_page);
    /* A second handle may have committed this page since open(). Never damage
     * it with another NOR program, even if the catalog entry is unchanged. */
    read_page_flash(physical, (char *)actual);
    if (!erased_page(actual)) {
        log->failed = 2u;
        return POGO_FLASH_LOG_STALE_HANDLE;
    }
    write_page_flash(physical, log->page);
    read_page_flash(physical, (char *)actual);
    if (memcmp(actual, log->page, sizeof(actual)) != 0) {
        /* Never retry programming a possibly torn NOR page in place. */
        log->failed = 1u;
        return POGO_FLASH_LOG_VERIFY_FAILED;
    }
    ++log->next_page;
    log->committed_bytes += log->used;
    log->used = 0u;
    memset(log->page, 0xff, sizeof(log->page));
    return POGO_FLASH_LOG_OK;
}

pogo_flash_log_status_t pogo_flash_log_service(pogo_flash_log_t *log) {
    if (log == NULL || log->ready == 0u) return POGO_FLASH_LOG_INVALID_ARGUMENT;
    if (log->used < POGO_FLASH_LOG_PAYLOAD_SIZE) return
        log->failed != 0u ? failed_status(log) : POGO_FLASH_LOG_OK;
    return commit_page(log);
}

pogo_flash_log_status_t pogo_flash_log_force_flush(pogo_flash_log_t *log) {
    return commit_page(log);
}

uint32_t pogo_flash_log_committed_bytes(const pogo_flash_log_t *log) {
    return log != NULL && log->ready != 0u ? log->committed_bytes : 0u;
}

pogo_flash_log_status_t pogo_flash_log_read_page(
    const pogo_flash_log_t *log, uint16_t page_index,
    uint8_t output[POGO_FLASH_FILE_PAGE_SIZE], uint8_t *used) {
    if (log == NULL || log->ready == 0u || output == NULL || used == NULL ||
        page_index >= log->page_count) return POGO_FLASH_LOG_INVALID_ARGUMENT;
    *used = 0u;
    pogo_flash_log_status_t owner = check_owner(log, output);
    if (owner != POGO_FLASH_LOG_OK) return owner;
    read_page_flash((uint16_t)(log->first_page + page_index), (char *)output);
    if (erased_page(output)) return POGO_FLASH_LOG_END;
    return valid_page(output, page_index, used) ? POGO_FLASH_LOG_OK :
        POGO_FLASH_LOG_CORRUPT;
}

size_t pogo_flash_log_format_u32(char output[11], uint32_t value) {
    if (output == NULL) return 0u;
    char reversed[10];
    size_t n = 0u;
    do {
        reversed[n++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    for (size_t i = 0u; i < n; ++i) output[i] = reversed[n - i - 1u];
    return n;
}

size_t pogo_flash_log_format_i32(char output[12], int32_t value) {
    if (output == NULL) return 0u;
    if (value >= 0) return pogo_flash_log_format_u32(output, (uint32_t)value);
    output[0] = '-';
    /* Unsigned subtraction handles INT32_MIN without signed overflow. */
    return 1u + pogo_flash_log_format_u32(output + 1u,
                                           0u - (uint32_t)value);
}

size_t pogo_flash_log_format_scaled_i32(
    char output[13], int32_t scaled_value, uint8_t decimal_places) {
    if (output == NULL || decimal_places > 6u) return 0u;
    if (decimal_places == 0u) return pogo_flash_log_format_i32(output,
                                                                 scaled_value);
    uint32_t divisor = 1u;
    for (uint8_t i = 0u; i < decimal_places; ++i) divisor *= 10u;
    uint32_t magnitude = scaled_value < 0 ? 0u - (uint32_t)scaled_value :
                                               (uint32_t)scaled_value;
    size_t n = 0u;
    if (scaled_value < 0) output[n++] = '-';
    n += pogo_flash_log_format_u32(output + n, magnitude / divisor);
    output[n++] = '.';
    uint32_t fraction = magnitude % divisor;
    for (uint8_t i = decimal_places; i > 0u; --i) {
        divisor /= 10u;
        output[n++] = (char)('0' + fraction / divisor);
        fraction %= divisor;
    }
    return n;
}
