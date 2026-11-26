/* NOR regression tests from pogo-utils ac6d558; only include paths adapted. */
/** Host regression for append-only logs on one-way-programmable NOR flash. */
#include "pffs/flash_log.h"
#include "pogobot.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t flash[POGO_FLASH_FILE_USER_PAGES][POGO_FLASH_FILE_PAGE_SIZE];
static unsigned sector_erases;
static unsigned page_programs;
static int torn_page = -1;
static int failed_erase_sector = -1;

/** Keep a deliberately malformed entry's outer CRC valid, so structural
 * validation (rather than checksum failure) must detect it. */
static void refresh_catalog_crc(uint8_t catalog) {
    uint32_t crc = UINT32_MAX;
    for (unsigned i = 0u; i < 252u; ++i) {
        crc ^= flash[catalog][i];
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    crc ^= UINT32_MAX;
    for (unsigned i = 0u; i < 4u; ++i) {
        flash[catalog][252u + i] = (uint8_t)(crc >> (8u * i));
    }
}

void erase_write_section_flash(void) { memset(flash, 0xff, sizeof(flash)); }

int spiBeginErase4(uint32_t address) {
    assert(address >= POGOBOT_USER_FLASH_START_OFFSET);
    uint32_t offset = address - POGOBOT_USER_FLASH_START_OFFSET;
    assert(offset < sizeof(flash) && offset % 4096u == 0u);
    if ((int)(offset / 4096u) == failed_erase_sector) return -1;
    memset(&flash[offset / POGO_FLASH_FILE_PAGE_SIZE], 0xff, 4096u);
    ++sector_erases;
    return 0;
}

void write_page_flash(uint16_t page, const void *data) {
    const uint8_t *bytes = data;
    unsigned limit = (int)page == torn_page ? 128u : POGO_FLASH_FILE_PAGE_SIZE;
    for (unsigned i = 0u; i < limit; ++i) flash[page][i] &= bytes[i];
    ++page_programs;
}

void read_page_flash(uint16_t page, char *buffer) {
    memcpy(buffer, flash[page], POGO_FLASH_FILE_PAGE_SIZE);
}

static void append_all(pogo_flash_log_t *log, const void *bytes, size_t length) {
    const uint8_t *p = bytes;
    while (length > 0u) {
        size_t accepted = 0u;
        pogo_flash_log_status_t status = pogo_flash_log_append(
            log, p, length, &accepted);
        assert(status == POGO_FLASH_LOG_OK ||
               status == POGO_FLASH_LOG_NEEDS_SERVICE);
        p += accepted;
        length -= accepted;
        if (status == POGO_FLASH_LOG_NEEDS_SERVICE) {
            assert(pogo_flash_log_service(log) == POGO_FLASH_LOG_OK);
        }
    }
}

static void stale_handles(void) {
    assert(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
    assert(pogo_flash_file_create_blank(1u, "gap", 1u, 1u) == POGO_FLASH_FILE_OK);
    pogo_flash_log_t old, current;
    assert(pogo_flash_log_initialize(&old, 2u, "log", 2u, false, NULL) == POGO_FLASH_LOG_OK);
    append_all(&old, "pending", 7u);
    uint16_t old_page = old.first_page;
    assert(pogo_flash_file_delete(1u) == POGO_FLASH_FILE_OK);
    pogo_flash_file_defrag_t defrag;
    assert(pogo_flash_file_defrag_begin(&defrag) == POGO_FLASH_FILE_OK);
    bool done = false;
    while (!done) assert(pogo_flash_file_defrag_step(&defrag, &done) == POGO_FLASH_FILE_OK);
    assert(defrag.moved_files == 1u);
    assert(pogo_flash_file_create_blank(3u, "new_owner", 1u, 1u) == POGO_FLASH_FILE_OK);
    pogo_flash_file_info_t info;
    assert(pogo_flash_file_find(3u, &info) == POGO_FLASH_FILE_OK && info.first_page == old_page);
    unsigned before_programs = page_programs, before_erases = sector_erases;
    assert(pogo_flash_log_force_flush(&old) == POGO_FLASH_LOG_STALE_HANDLE);
    assert(pogo_flash_log_clear(&old) == POGO_FLASH_LOG_STALE_HANDLE);
    uint8_t page[256], used;
    assert(pogo_flash_log_read_page(&old, 0, page, &used) == POGO_FLASH_LOG_STALE_HANDLE);
    size_t accepted;
    assert(pogo_flash_log_append(&old, "x", 1u, &accepted) == POGO_FLASH_LOG_STALE_HANDLE);
    assert(page_programs == before_programs && sector_erases == before_erases);
    assert(pogo_flash_file_read_page_secure(3u, 0u, page, NULL) == POGO_FLASH_FILE_OK);
    for (unsigned i = 0; i < sizeof(page); ++i) assert(page[i] == 0xffu);

    /* Explicitly reopen at the moved address; the discarded RAM bytes do not
     * leak into the replacement file or the resumed log. */
    assert(pogo_flash_log_open(&current, 2u) == POGO_FLASH_LOG_OK);
    append_all(&current, "fresh", 5u);
    assert(pogo_flash_log_force_flush(&current) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_log_read_page(&current, 0u, page, &used) == POGO_FLASH_LOG_OK && used == 5u);
    assert(memcmp(page + POGO_FLASH_LOG_HEADER_SIZE, "fresh", 5u) == 0);

    old = current;
    assert(pogo_flash_file_delete(2u) == POGO_FLASH_FILE_OK);
    assert(pogo_flash_log_initialize(&current, 2u, "replacement", 2u, false, NULL) == POGO_FLASH_LOG_OK);
    append_all(&old, "stale", 5u);
    before_programs = page_programs; before_erases = sector_erases;
    assert(pogo_flash_log_force_flush(&old) == POGO_FLASH_LOG_STALE_HANDLE);
    assert(pogo_flash_log_clear(&old) == POGO_FLASH_LOG_STALE_HANDLE);
    assert(page_programs == before_programs && sector_erases == before_erases);

    /* Two independently opened writers may share an unchanged entry. A page
     * committed by the first must not be programmed a second time. */
    assert(pogo_flash_log_open(&old, 2u) == POGO_FLASH_LOG_OK);
    append_all(&current, "first", 5u);
    assert(pogo_flash_log_force_flush(&current) == POGO_FLASH_LOG_OK);
    append_all(&old, "second", 6u);
    before_programs = page_programs;
    assert(pogo_flash_log_force_flush(&old) == POGO_FLASH_LOG_STALE_HANDLE);
    assert(page_programs == before_programs);
    before_erases = sector_erases;
    assert(pogo_flash_log_clear(&old) == POGO_FLASH_LOG_STALE_HANDLE);
    assert(sector_erases == before_erases);
    assert(pogo_flash_log_read_page(&current, 0u, page, &used) == POGO_FLASH_LOG_OK && used == 5u);
    assert(memcmp(page + POGO_FLASH_LOG_HEADER_SIZE, "first", 5u) == 0);

    /* Valid-looking metadata with a broken CRC must also fail before writing. */
    append_all(&current, "cached", 6u);
    flash[0][12u + 48u + 16u] ^= 1u; /* Corrupt this entry's name, not its bounds. */
    before_programs = page_programs; before_erases = sector_erases;
    assert(pogo_flash_log_force_flush(&current) == POGO_FLASH_LOG_CORRUPT);
    assert(page_programs == before_programs && sector_erases == before_erases);
}

int main(void) {
    erase_write_section_flash();
    assert(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
    uint8_t calibration[POGO_FLASH_FILE_PAGE_SIZE];
    memset(calibration, 0x5au, sizeof(calibration));
    assert(pogo_flash_file_create(1u, "calibration", 1u, 1u, calibration) ==
           POGO_FLASH_FILE_OK);
    assert(pogo_flash_file_create(4u, "reserved", 1u,
           POGO_FLASH_LOG_FORMAT_VERSION, calibration) ==
           POGO_FLASH_FILE_INVALID_ARGUMENT);

    pogo_flash_log_t print_log;
    pogo_flash_log_t csv_log;
    bool formatted = true;
    assert(pogo_flash_log_initialize(&print_log, 2u, "prints", 2u, false,
                                     &formatted) == POGO_FLASH_LOG_OK);
    assert(!formatted);
    assert(pogo_flash_log_initialize(&csv_log, 3u, "csv", 1u, false,
                                     &formatted) == POGO_FLASH_LOG_OK);
    assert(!formatted);
    pogo_flash_file_info_t info;
    assert(pogo_flash_file_find(2u, &info) == POGO_FLASH_FILE_OK);
    assert(info.format_version == POGO_FLASH_LOG_FORMAT_VERSION);
    assert(info.size_bytes == 512u); /* Reserved container length stays fixed. */
    assert(pogo_flash_log_committed_bytes(&print_log) == 0u);
    assert(pogo_flash_log_committed_bytes(NULL) == 0u);
    assert(pogo_flash_file_read_page_secure(2u, 0u, calibration, NULL) ==
           POGO_FLASH_FILE_UNSUPPORTED_FORMAT);
    assert(pogo_flash_file_replace(2u, 2u, calibration) ==
           POGO_FLASH_FILE_UNSUPPORTED_FORMAT);

    unsigned erases_before = sector_erases;
    unsigned programs_before = page_programs;
    uint8_t full[POGO_FLASH_LOG_PAYLOAD_SIZE];
    memset(full, 'A', sizeof(full));
    append_all(&print_log, full, sizeof(full));
    assert(pogo_flash_log_committed_bytes(&print_log) == 0u); /* Cache only. */
    assert(pogo_flash_log_service(&print_log) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_log_committed_bytes(&print_log) == sizeof(full));
    assert(sector_erases == erases_before);
    assert(page_programs == programs_before + 1u);
    append_all(&print_log, "end\n", 4u);
    assert(pogo_flash_log_service(&print_log) == POGO_FLASH_LOG_OK);
    assert(page_programs == programs_before + 1u); /* Partial stays in RAM. */
    assert(pogo_flash_log_force_flush(&print_log) == POGO_FLASH_LOG_OK);
    assert(sector_erases == erases_before);
    assert(page_programs == programs_before + 2u);
    assert(pogo_flash_log_committed_bytes(&print_log) == sizeof(full) + 4u);
    assert(pogo_flash_file_find(2u, &info) == POGO_FLASH_FILE_OK && info.size_bytes == 512u);

    char number[13];
    size_t n = pogo_flash_log_format_scaled_i32(number, -12345, 2u);
    assert(n == 7u && memcmp(number, "-123.45", n) == 0);
    n = pogo_flash_log_format_i32(number, INT32_MIN);
    assert(n == 11u && memcmp(number, "-2147483648", n) == 0);
    n = pogo_flash_log_format_u32(number, UINT32_MAX);
    assert(n == 10u && memcmp(number, "4294967295", n) == 0);
    n = pogo_flash_log_format_scaled_i32(number, INT32_MIN, 6u);
    assert(n == 12u && memcmp(number, "-2147.483648", n) == 0);
    append_all(&csv_log, "time,value\n", 11u);
    assert(pogo_flash_log_force_flush(&csv_log) == POGO_FLASH_LOG_OK);
    size_t accepted = 99u;
    assert(pogo_flash_log_append(&csv_log, "x", 1u, &accepted) ==
           POGO_FLASH_LOG_FULL && accepted == 0u);

    pogo_flash_log_t reopened;
    assert(pogo_flash_log_open(&reopened, 2u) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_log_committed_bytes(&reopened) == sizeof(full) + 4u);
    uint8_t page[POGO_FLASH_FILE_PAGE_SIZE];
    uint8_t used = 0u;
    assert(pogo_flash_log_read_page(&reopened, 0u, page, &used) ==
           POGO_FLASH_LOG_OK && used == sizeof(full));
    assert(memcmp(page + POGO_FLASH_LOG_HEADER_SIZE, full, sizeof(full)) == 0);
    assert(pogo_flash_log_read_page(&reopened, 1u, page, &used) ==
           POGO_FLASH_LOG_OK && used == 4u);
    assert(memcmp(page + POGO_FLASH_LOG_HEADER_SIZE, "end\n", 4u) == 0);
    assert(pogo_flash_log_clear(&reopened) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_log_committed_bytes(&reopened) == 0u);
    assert(pogo_flash_log_read_page(&reopened, 0u, page, &used) ==
           POGO_FLASH_LOG_END);
    assert(pogo_flash_file_read_page_secure(1u, 0u, page, NULL) ==
           POGO_FLASH_FILE_OK);
    assert(pogo_flash_log_open(&csv_log, 3u) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_log_read_page(&csv_log, 0u, page, &used) ==
           POGO_FLASH_LOG_OK);

    /* Bytes left below one page are intentionally lost on a simulated reset;
     * reopening finds the first erased page and can resume there. */
    append_all(&reopened, "not committed", 13u);
    assert(pogo_flash_log_open(&reopened, 2u) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_log_read_page(&reopened, 0u, page, &used) ==
           POGO_FLASH_LOG_END);

    append_all(&reopened, full, sizeof(full));
    assert(pogo_flash_log_service(&reopened) == POGO_FLASH_LOG_OK);
    append_all(&reopened, full, sizeof(full));
    torn_page = reopened.first_page + 1u;
    assert(pogo_flash_log_service(&reopened) == POGO_FLASH_LOG_VERIFY_FAILED);
    torn_page = -1;
    assert(pogo_flash_log_open(&reopened, 2u) == POGO_FLASH_LOG_CORRUPT);
    assert(pogo_flash_log_committed_bytes(&reopened) == sizeof(full));
    assert(pogo_flash_log_read_page(&reopened, 0u, page, &used) ==
           POGO_FLASH_LOG_OK && used == sizeof(full));
    assert(pogo_flash_log_read_page(&reopened, 1u, page, &used) ==
           POGO_FLASH_LOG_CORRUPT);
    assert(pogo_flash_log_clear(&reopened) == POGO_FLASH_LOG_OK);

    /* Creating on a corrupt catalog deliberately reformats all user files. */
    flash[0][4] = 0u;
    assert(pogo_flash_log_initialize(&reopened, 4u, "new", 1u, false,
                                     &formatted) == POGO_FLASH_LOG_OK);
    assert(formatted);
    assert(pogo_flash_file_find(1u, &info) == POGO_FLASH_FILE_NOT_FOUND);
    erase_write_section_flash();
    assert(pogo_flash_log_initialize(&reopened, 4u, "new", 1u, false,
                                     &formatted) == POGO_FLASH_LOG_OK);
    assert(formatted); /* Virgin flash has the same explicit recovery policy. */
    flash[0][12u + 3u * 48u + 7u] |= 0xc0u; /* Reserved extent bits in ID 4. */
    refresh_catalog_crc(0u);
    assert(pogo_flash_log_initialize(&reopened, 5u, "later", 1u, false,
                                     &formatted) == POGO_FLASH_LOG_OK);
    assert(formatted);
    assert(pogo_flash_file_find(4u, &info) == POGO_FLASH_FILE_NOT_FOUND);

    /* An incompatible catalog is not the corrupt-current recovery case:
     * initialization must preserve every byte for explicit export/format. */
    flash[0][4] = 3u;
    unsigned version_erases = sector_erases;
    unsigned version_programs = page_programs;
    assert(pogo_flash_log_open(&reopened, 5u) == POGO_FLASH_LOG_WRONG_FORMAT);
    assert(pogo_flash_log_initialize(&reopened, 5u, "later", 1u, false,
                                     &formatted) == POGO_FLASH_LOG_WRONG_FORMAT);
    assert(!formatted && flash[0][4] == 3u && sector_erases == version_erases &&
           page_programs == version_programs);

    /* No numeric ID is reserved for calibration; even ID 1 can host a log. */
    erase_write_section_flash();
    assert(pogo_flash_file_create(4u, "hole", 1u, 1u, calibration) ==
           POGO_FLASH_FILE_OK);
    assert(pogo_flash_log_initialize(&reopened, 1u, "id_one_log", 1u, false,
                                     &formatted) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_file_find(1u, &info) == POGO_FLASH_FILE_OK &&
           info.format_version == POGO_FLASH_LOG_FORMAT_VERSION &&
           info.first_page == 272u);
    append_all(&reopened, "moved log\n", 10u);
    assert(pogo_flash_log_force_flush(&reopened) == POGO_FLASH_LOG_OK);
    assert(pogo_flash_file_delete(4u) == POGO_FLASH_FILE_OK);
    pogo_flash_file_defrag_t defrag;
    assert(pogo_flash_file_defrag_begin(&defrag) == POGO_FLASH_FILE_OK);
    bool done = false;
    for (unsigned steps = 0u; steps < 20u && !done; ++steps) {
        assert(pogo_flash_file_defrag_step(&defrag, &done) == POGO_FLASH_FILE_OK);
    }
    assert(done && defrag.moved_files == 1u);
    assert(pogo_flash_log_open(&reopened, 1u) == POGO_FLASH_LOG_OK);
    assert(reopened.first_page == 256u);
    assert(pogo_flash_log_read_page(&reopened, 0u, page, &used) ==
           POGO_FLASH_LOG_OK && used == 10u);
    assert(memcmp(page + POGO_FLASH_LOG_HEADER_SIZE, "moved log\n", 10u) == 0);

    /* A 64 KiB log spans 16 erase sectors and uses page header indices
     * 0..255. Reopening a full log must represent next_page == 256 without
     * wrapping, and clear must erase every sector rather than only the first. */
    erase_write_section_flash();
    assert(pogo_flash_log_initialize(&reopened, 2u, "large",
           POGO_FLASH_LOG_MAX_PAGES, false, &formatted) == POGO_FLASH_LOG_OK);
    for (uint16_t i = 0u; i < POGO_FLASH_LOG_MAX_PAGES; ++i) {
        memset(full, (uint8_t)i, sizeof(full));
        append_all(&reopened, full, sizeof(full));
        assert(pogo_flash_log_service(&reopened) == POGO_FLASH_LOG_OK);
    }
    assert(reopened.next_page == POGO_FLASH_LOG_MAX_PAGES);
    assert(pogo_flash_log_open(&reopened, 2u) == POGO_FLASH_LOG_OK);
    assert(reopened.next_page == POGO_FLASH_LOG_MAX_PAGES);
    assert(pogo_flash_log_read_page(&reopened, 255u, page, &used) ==
           POGO_FLASH_LOG_OK && used == sizeof(full));
    assert(page[5] == 255u && page[POGO_FLASH_LOG_HEADER_SIZE] == 255u);
    assert(pogo_flash_log_read_page(&reopened, 256u, page, &used) ==
           POGO_FLASH_LOG_INVALID_ARGUMENT);
    accepted = 99u;
    assert(pogo_flash_log_append(&reopened, "x", 1u, &accepted) ==
           POGO_FLASH_LOG_FULL && accepted == 0u);
    /* A failed second-sector erase must stop further writes through this
     * handle; a later explicit clear can still recover it. */
    failed_erase_sector = (int)(reopened.first_page /
        POGO_FLASH_FILE_ERASE_SECTOR_PAGES) + 1;
    assert(pogo_flash_log_clear(&reopened) == POGO_FLASH_LOG_VERIFY_FAILED);
    assert(reopened.failed != 0u);
    accepted = 99u;
    assert(pogo_flash_log_append(&reopened, "x", 1u, &accepted) ==
           POGO_FLASH_LOG_VERIFY_FAILED && accepted == 0u);
    failed_erase_sector = -1;
    erases_before = sector_erases;
    assert(pogo_flash_log_clear(&reopened) == POGO_FLASH_LOG_OK);
    assert(sector_erases == erases_before + 16u);
    assert(pogo_flash_log_read_page(&reopened, 255u, page, &used) ==
           POGO_FLASH_LOG_END);
    assert(pogo_flash_log_initialize(&reopened, 3u, "too_large",
           POGO_FLASH_LOG_MAX_PAGES + 1u, false, NULL) ==
           POGO_FLASH_LOG_INVALID_ARGUMENT);
    stale_handles();
    puts("NOR flash log tests passed");
    return 0;
}
