#ifndef POGO_UTILS_FLASH_FILE_H
#define POGO_UTILS_FLASH_FILE_H

/* SDK feature marker; the persistent v4 schema and existing APIs are unchanged. */
#define POGOBOT_PFFS_API_VERSION 4u

/**
 * @file flash_file.h
 * @brief Small, bounded named-file catalog over Pogobot user flash.
 *
 * PFFS v4 uses all 5888 SDK user-flash pages. Sixteen catalog pages in
 * separate erase sectors contain five slots each. Stable file IDs 1..80 map
 * directly to those slots, so ID lookup still reads one catalog page. Files
 * occupy contiguous pages and own every 4 KiB sector that they intersect.
 * Human-readable names are optional metadata; IDs are the persistent identity.
 *
 * The fast reader checks structural bounds but deliberately skips CRC checks.
 * The secure reader checks the catalog CRC and the CRC of every ordinary file
 * page. Append-only flash logs use their own per-page CRC reader instead.
 * Replacement erases and rewrites the file's dedicated sectors. Catalog changes
 * erase and rewrite only the selected catalog sector. Mutations, including
 * defragmentation, are not transactional: interrupted writes cannot be rolled
 * back.
 *
 * On-flash page numbers are relative to the SDK user-flash region:
 *
 *   pages 0,16,..240  catalog pages, one per dedicated erase sector
 *   pages 256..5887  data extents, rounded up to complete erase sectors
 *
 * There is no heap allocation, directory tree, byte-stream cursor,
 * automatic compaction, or implicit open-file state. Exact byte lengths are
 * catalog metadata; I/O stays page based. A "file page" is one physical
 * 256-byte page. That restriction is deliberate: callers can keep one page
 * buffer and the fast reader can resolve an ID with two physical reads.
 *
 * Public functions are synchronous. They do not lock the flash peripheral and
 * must not be interleaved by concurrent callers for the same robot. Returned
 * metadata is decoded into ordinary host values; callers never receive pointers
 * into a temporary catalog buffer.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    POGO_FLASH_FILE_PAGE_SIZE = 256,    /**< Physical and logical page size. */
    POGO_FLASH_FILE_USER_PAGES = 5888, /**< SDK user-flash page count. */
    POGO_FLASH_FILE_CATALOG_PAGES = 16, /**< Catalogs in distinct sectors. */
    POGO_FLASH_FILE_ERASE_SECTOR_PAGES = 16, /**< 4 KiB sector in 256-byte pages. */
    POGO_FLASH_FILE_DATA_FIRST_PAGE = 256, /**< First data sector/page. */
    POGO_FLASH_FILE_MAX_FILES = 80,    /**< Number of stable ID slots. */
    POGO_FLASH_FILE_MAX_PAGES = 5632,  /**< Largest contiguous file extent. */
    POGO_FLASH_FILE_MAX_BYTES = 1441792, /**< Maximum exact ordinary-file length. */
    POGO_FLASH_FILE_MAX_NAME = 32      /**< Stored name bytes, excluding NUL. */
};

#define POGO_FLASH_FILE_NAME_MAGNETOMETER_CALIBRATION \
    "magnetometer_calibration"

/** Decoded catalog entry returned to callers.
 *
 * The structure is an in-memory API object, not the serialized representation.
 * Its padding and native byte order therefore have no effect on flash format.
 * `name` is always NUL-terminated by the decoder, including for a 32-byte name.
 */
typedef struct {
    uint8_t id;             /**< Stable ID whose numeric value selects the slot. */
    uint16_t first_page;    /**< First physical page in the contiguous extent. */
    uint16_t page_count;    /**< Immutable number of pages in the extent. */
    uint32_t size_bytes;    /**< Exact ordinary-file length; log container capacity. */
    uint8_t name_length;    /**< Stored label length; zero means unnamed. */
    uint16_t format_version; /**< Payload schema; 0x8001 is reserved for logs. */
    uint32_t generation;    /**< Per-file replacement counter, starting at one. */
    uint32_t data_crc32;    /**< CRC over all allocated pages, in page order. */
    char name[POGO_FLASH_FILE_MAX_NAME + 1]; /**< Optional NUL-terminated label. */
} pogo_flash_file_info_t;

/** Result codes shared by reader and writer entry points.
 *
 * `UNFORMATTED` is distinct from `CORRUPT_CATALOG` for diagnostics. Readers
 * never modify flash for either result. create() formats on either result and
 * thus can destroy recoverable files when a catalog is corrupt. Recognizable
 * incompatible catalog versions return UNSUPPORTED_FORMAT without erasing.
 */
typedef enum {
    POGO_FLASH_FILE_OK = 0,             /**< Operation completed successfully. */
    POGO_FLASH_FILE_INVALID_ARGUMENT,   /**< NULL pointer, invalid ID, or name. */
    POGO_FLASH_FILE_UNFORMATTED,        /**< Catalog page is uniformly blank. */
    POGO_FLASH_FILE_CORRUPT_CATALOG,    /**< Header, entry, extent, or CRC fails. */
    POGO_FLASH_FILE_NOT_FOUND,          /**< Selected slot/name is not in use. */
    POGO_FLASH_FILE_ALREADY_EXISTS,     /**< Creation selected an occupied ID. */
    POGO_FLASH_FILE_NAME_EXISTS,        /**< Creation duplicated a nonempty name. */
    POGO_FLASH_FILE_INVALID_SIZE,       /**< Length too large or allocation changed. */
    POGO_FLASH_FILE_NO_SPACE,           /**< No sufficiently long free extent exists. */
    POGO_FLASH_FILE_BAD_CHECKSUM,       /**< Secure read found damaged file data. */
    POGO_FLASH_FILE_VERIFY_FAILED,      /**< Immediate write/readback differed. */
    POGO_FLASH_FILE_GENERATION_EXHAUSTED, /**< A monotonic counter reached UINT32_MAX. */
    POGO_FLASH_FILE_UNSUPPORTED_FORMAT /**< Incompatible catalog or payload reader. */
} pogo_flash_file_status_t;

/** Find an ID using structural checks only; no CRC is calculated.
 *
 * @param file_id Stable ID in the inclusive range 1..80.
 * @param info Required destination for decoded metadata.
 * @return OK, NOT_FOUND, UNFORMATTED, CORRUPT_CATALOG, UNSUPPORTED_FORMAT,
 * or INVALID_ARGUMENT.
 */
pogo_flash_file_status_t pogo_flash_file_find(
    uint8_t file_id,
    pogo_flash_file_info_t *info);

/** Scan the catalog pages for an optional human-readable name.
 *
 * Names are exact, case-sensitive byte strings. Empty names are not searchable
 * and numeric IDs remain authoritative even when a name is present.
 */
pogo_flash_file_status_t pogo_flash_file_find_by_name(
    const char *name,
    pogo_flash_file_info_t *info);

/** Read one file page with no catalog or data CRC validation.
 * Exactly one catalog page and one data page are read on success. output is
 * also used as catalog scratch space, so no persistent cache is required.
 * `file_page` is zero-based within the file, not a physical flash page number.
 * `info` is optional and is written only on success.
 * A full physical page is returned, including padding after info->size_bytes.
 * Empty files still have a readable erased page; callers enforce logical EOF.
 */
pogo_flash_file_status_t pogo_flash_file_read_page_fast(
    uint8_t file_id,
    uint16_t file_page,
    uint8_t output[POGO_FLASH_FILE_PAGE_SIZE],
    pogo_flash_file_info_t *info);

/** Read one page after validating the catalog and the complete file CRC.
 * Validation reads every page in the file and may reread the requested page.
 * The complete-file scan means latency grows linearly with `page_count`.
 * Append-only logs return UNSUPPORTED_FORMAT: use flash_log.h instead.
 */
pogo_flash_file_status_t pogo_flash_file_read_page_secure(
    uint8_t file_id,
    uint16_t file_page,
    uint8_t output[POGO_FLASH_FILE_PAGE_SIZE],
    pogo_flash_file_info_t *info);

/** Erase all catalog sectors and initialize the empty PFFS v4 catalog.
 *
 * This logically deletes every PFFS file, but does not wipe old payload bytes
 * in unallocated sectors. create() invokes it if catalogs are absent/corrupt.
 */
pogo_flash_file_status_t pogo_flash_file_format(void);

/** Validate all catalogs, every occupied entry, and extent non-overlap.
 * This is read-only. Unlike create(), it never autoformats damaged metadata.
 * File payload CRCs are not checked here.
 */
pogo_flash_file_status_t pogo_flash_file_check(void);

/** Create a file in the slot selected by file_id.
 * data must contain exactly page_count consecutive 256-byte pages.
 * `name` may be NULL or empty for an unnamed file. Data pages are written and
 * verified before the catalog entry is published, so a data-write failure does
 * not make the new slot visible.
 * If catalogs are missing or corrupt, creation first clears the catalog sectors.
 * Recognizable incompatible versions require explicit format instead.
 * This deliberately loses any existing PFFS files, including data that
 * might have been recovered from a corrupt catalog. Invalid arguments and
 * occupied IDs do not format an otherwise valid filesystem.
 * Format version 0x8001 is reserved for flash_log and rejected here.
 */
pogo_flash_file_status_t pogo_flash_file_create(
    uint8_t file_id,
    const char *name,
    uint16_t page_count,
    uint16_t format_version,
    const uint8_t *data);

/** Create an ordinary file whose allocated pages initially contain 0xff.
 * This avoids a page_count*256-byte caller buffer. As with create(), an
 * absent or corrupt catalog is automatically formatted before creation.
 */
pogo_flash_file_status_t pogo_flash_file_create_blank(
    uint8_t file_id,
    const char *name,
    uint16_t page_count,
    uint16_t format_version);

/** Byte-count variants. Input contains exactly size_bytes bytes; the final
 * physical page is padded with 0xff. Empty files still reserve one page.
 * Existing page-count APIs remain available for complete page images, recording
 * page_count*256 bytes. data may be NULL only for length zero. Replacement
 * allows any new byte length with the same max(1,ceil(length/256)) page count;
 * it retains the extent and rewrites its sectors, not just a byte suffix. */
pogo_flash_file_status_t pogo_flash_file_create_bytes(
    uint8_t file_id, const char *name, uint32_t size_bytes,
    uint16_t format_version, const uint8_t *data);
pogo_flash_file_status_t pogo_flash_file_create_blank_bytes(
    uint8_t file_id, const char *name, uint32_t size_bytes, uint16_t format_version);
pogo_flash_file_status_t pogo_flash_file_replace_bytes(
    uint8_t file_id, uint32_t size_bytes, const uint8_t *data);

/** Caller-owned state for bounded-RAM, sequential ordinary-file writes.
 * Fields are private to the writer and must not be modified by applications.
 * One page is programmed per write_page() call; no file-sized buffer is used.
 */
typedef struct {
    uint16_t first_page, page_count, next_page, format_version;
    uint8_t file_id, name_length, mode;
    uint32_t crc, file_generation, catalog_generation, size_bytes;
    char name[POGO_FLASH_FILE_MAX_NAME + 1];
} pogo_flash_file_writer_t;

/** Prepare an unpublished new file. A missing/corrupt catalog is reformatted.
 * Allocation is not visible until exactly page_count calls to write_page()
 * are followed by write_finish(). `formatted` may be NULL.
 */
pogo_flash_file_status_t pogo_flash_file_write_begin_create(
    pogo_flash_file_writer_t *writer, uint8_t file_id, const char *name,
    uint16_t page_count, uint16_t format_version, bool *formatted);

/** Prepare replacement retaining the existing exact length and page count.
 * Its old data becomes invalid as soon as the first sector is erased by write_page().
 */
pogo_flash_file_status_t pogo_flash_file_write_begin_replace(
    pogo_flash_file_writer_t *writer, uint8_t file_id);

/** Stream an exact byte length using full physical pages. The caller pads the
 * last page with 0xff; noncanonical padding is rejected before programming.
 * Replacement may change length only while retaining the allocated page count. */
pogo_flash_file_status_t pogo_flash_file_write_begin_create_bytes(
    pogo_flash_file_writer_t *writer, uint8_t file_id, const char *name,
    uint32_t size_bytes, uint16_t format_version, bool *formatted);
pogo_flash_file_status_t pogo_flash_file_write_begin_replace_bytes(
    pogo_flash_file_writer_t *writer, uint8_t file_id, uint32_t size_bytes);

/** Erase a sector when entering it, then program/readback one 256-byte page.
 * A failed page write invalidates the writer; retry requires a new begin call.
 */
pogo_flash_file_status_t pogo_flash_file_write_page(
    pogo_flash_file_writer_t *writer,
    const uint8_t data[POGO_FLASH_FILE_PAGE_SIZE]);

/** Publish the accumulated CRC after all pages have been written. */
pogo_flash_file_status_t pogo_flash_file_write_finish(
    pogo_flash_file_writer_t *writer);

/** Discard only RAM state. It does not undo any erased/programmed flash pages. */
void pogo_flash_file_write_abort(pogo_flash_file_writer_t *writer);

/** Caller-owned state for a bounded-RAM, forward-only compaction pass.
 * Fields are private. A move can overlap its old extent, so interruption may
 * damage that file before its catalog entry can be updated. Back up first.
 */
typedef struct {
    uint16_t cursor_page, source_page, target_page, page_count;
    uint16_t checked_pages, next_page;
    uint32_t expected_crc, running_crc, file_generation;
    uint8_t file_id, phase, moved_files, last_moved_id;
    uint16_t last_from_page, last_to_page;
} pogo_flash_file_defrag_t;

/** Validate catalog metadata and prepare a compaction pass without writing.
 * Ordinary-file payload CRCs are checked incrementally by step(). Log payload
 * integrity should be checked by the caller before starting.
 */
pogo_flash_file_status_t pogo_flash_file_defrag_begin(
    pogo_flash_file_defrag_t *defrag);

/** Perform at most one catalog scan, one source-page check, one copied page,
 * or one catalog update per call. The caller must serialize all other PFFS
 * access until *done becomes true. On error, the pass stops; prior moves may
 * remain committed, and the current source may need restoration from backup.
 */
pogo_flash_file_status_t pogo_flash_file_defrag_step(
    pogo_flash_file_defrag_t *defrag, bool *done);

/** Replace an existing file in its dedicated sectors. The page count stays fixed.
 * This page-count wrapper records page_count*256 bytes, including padding.
 *
 * Replacement is deliberately non-transactional: the data sector is erased
 * and programmed before the catalog CRC/generation is updated. A reset or
 * power loss may leave the old catalog referring to incomplete new data.
 */
pogo_flash_file_status_t pogo_flash_file_replace(
    uint8_t file_id,
    uint16_t page_count,
    const uint8_t *data);

/** Remove a catalog entry. Data pages are left untouched and may be reused.
 *
 * Deletion therefore does not securely erase payload bytes. The next creation
 * may allocate the released extent and overwrite them.
 */
pogo_flash_file_status_t pogo_flash_file_delete(uint8_t file_id);

/** Change only a file's optional human-readable name; its ID and data stay.
 * The new nonempty name must be unique. This rewrites the catalog sector and
 * is not transactional across a power failure.
 */
pogo_flash_file_status_t pogo_flash_file_rename(
    uint8_t file_id, const char *new_name);

/** Return a static diagnostic string. No allocation or formatting is done. */
const char *pogo_flash_file_status_string(pogo_flash_file_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* POGO_UTILS_FLASH_FILE_H */
