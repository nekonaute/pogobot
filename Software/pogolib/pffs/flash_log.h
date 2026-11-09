#ifndef POGO_UTILS_FLASH_LOG_H
#define POGO_UTILS_FLASH_LOG_H

/**
 * @file flash_log.h
 * @brief Small append-only byte streams in separate PFFS flash files.
 *
 * One instance owns one 256-byte RAM cache and one existing PFFS file extent.
 * Appends only copy into RAM; service() programs at most one *full* page, and
 * force_flush() programs a partial page on explicit request. Each page is
 * programmed once. There is no heap, printf interception, or background work.
 * The stored content is a byte stream: callers supply their own line endings
 * or binary record framing, and readers concatenate each page's used payload.
 *
 * Calls are synchronous and not safe to interleave on one flash peripheral.
 * An interrupted/invalid page makes further appends fail until that log is
 * explicitly cleared; already valid pages remain readable. A reset loses
 * cache bytes that have not been committed. Warnings belong to the caller.
 */
#include "flash_file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    POGO_FLASH_LOG_FORMAT_VERSION = 0x8001, /**< Reserved PFFS payload type. */
    POGO_FLASH_LOG_DEFAULT_PAGES = 256, /**< 64 KiB for newly created logs. */
    POGO_FLASH_LOG_MAX_PAGES = 256, /**< 64 KiB; 8-bit header indexes 0..255. */
    POGO_FLASH_LOG_PAGE_VERSION = 1,
    POGO_FLASH_LOG_HEADER_SIZE = 12,
    POGO_FLASH_LOG_PAYLOAD_SIZE =
        POGO_FLASH_FILE_PAGE_SIZE - POGO_FLASH_LOG_HEADER_SIZE
};

typedef enum {
    POGO_FLASH_LOG_OK = 0,
    POGO_FLASH_LOG_INVALID_ARGUMENT,
    POGO_FLASH_LOG_NOT_FOUND,
    POGO_FLASH_LOG_WRONG_FORMAT,
    POGO_FLASH_LOG_NAME_EXISTS,
    POGO_FLASH_LOG_NO_SPACE,
    POGO_FLASH_LOG_CORRUPT,
    POGO_FLASH_LOG_FULL,
    POGO_FLASH_LOG_NEEDS_SERVICE,
    POGO_FLASH_LOG_END,
    POGO_FLASH_LOG_VERIFY_FAILED,
    POGO_FLASH_LOG_FLASH_ERROR,
    POGO_FLASH_LOG_STALE_HANDLE /**< File moved/replaced; discard cache and reopen. */
} pogo_flash_log_status_t;

/** Caller-owned per-file state; keep in robot USERDATA rather than on a small
 * task stack. Fields are implementation state and must not be edited directly. */
typedef struct {
    uint8_t page[POGO_FLASH_FILE_PAGE_SIZE];
    uint8_t file_id;
    uint16_t first_page;
    uint16_t page_count;
    uint16_t next_page;
    uint8_t used;
    uint8_t ready;
    uint8_t failed;
    uint32_t identity_crc; /**< Fingerprint of the catalog entry read at open. */
    uint32_t committed_bytes; /**< Verified stream bytes, excluding page headers/padding. */
} pogo_flash_log_t;

/** Open an existing clean log, scanning at most 256 pages. Never writes.
 * Reopening an active handle discards its uncommitted RAM cache bytes. */
pogo_flash_log_status_t pogo_flash_log_open(pogo_flash_log_t *log,
                                           uint8_t file_id);

/** Create if missing, or optionally clear an existing log, then open it.
 * Pass POGO_FLASH_LOG_DEFAULT_PAGES for a new 64 KiB allocation; this API
 * keeps page_count explicit so smaller fixed-size logs remain possible.
 * If the PFFS catalog is absent/corrupt, creation automatically formats the
 * all PFFS catalogs, losing access to unrelated files. `formatted` reports
 * that event so applications can warn; NULL suppresses the report. Existing
 * IDs with a different size, name, or payload type are never overwritten. */
pogo_flash_log_status_t pogo_flash_log_initialize(
    pogo_flash_log_t *log, uint8_t file_id, const char *name,
    uint16_t page_count, bool clear_existing, bool *formatted);

/** Erase every sector owned by this log and reset its cache. */
pogo_flash_log_status_t pogo_flash_log_clear(pogo_flash_log_t *log);

/** Copy up to the available cache space and report exactly how many bytes were
 * accepted. NEEDS_SERVICE means the caller must retain/retry the remainder
 * after service() commits the full page; no flash I/O occurs here. */
pogo_flash_log_status_t pogo_flash_log_append(
    pogo_flash_log_t *log, const void *bytes, size_t length, size_t *accepted);

/** Commit one full cached page, or do nothing if it is not full.
 * Before programming, validate the current catalog entry and require an erased
 * destination. A stale handle fails without programming any data. Reopen after
 * moving, deleting/recreating or renaming a file; serialize filesystem access. */
pogo_flash_log_status_t pogo_flash_log_service(pogo_flash_log_t *log);

/** Commit even a partial page. Its unused capacity cannot be used later. */
pogo_flash_log_status_t pogo_flash_log_force_flush(pogo_flash_log_t *log);

/** Exact durable payload length, reconstructed at open and updated on commit.
 * Excludes the RAM cache. This is a cached count with no flash I/O; reopen the
 * handle after external file changes. Returns zero for NULL/unopened handles. */
uint32_t pogo_flash_log_committed_bytes(const pogo_flash_log_t *log);

/** Read and CRC-check one committed page. Returns END for an unused page.
 * output is caller-owned scratch; only payload bytes at offset 12, of length
 * `used`, are part of the byte stream. Output must be separate from log->page
 * if this handle will append later. Read-only callers may reuse log->page to
 * save RAM, but must reopen the handle before switching back to writes. */
pogo_flash_log_status_t pogo_flash_log_read_page(
    const pogo_flash_log_t *log, uint16_t page_index,
    uint8_t output[POGO_FLASH_FILE_PAGE_SIZE], uint8_t *used);

/** Allocation-free decimal helpers for constructing CSV in a small local
 * array before passing its bytes to append(). No terminator is written. */
size_t pogo_flash_log_format_u32(char output[11], uint32_t value);
size_t pogo_flash_log_format_i32(char output[12], int32_t value);
size_t pogo_flash_log_format_scaled_i32(
    char output[13], int32_t scaled_value, uint8_t decimal_places);

#ifdef __cplusplus
}
#endif

#endif /* POGO_UTILS_FLASH_LOG_H */
