/* PFFS v4 commands adapted from pogo-utils ac6d558/examples/flash_file.
 * The catalog and UART protocol are unchanged; Pogobios owns the console,
 * monotonic transfer clock and serialization with other flash operations. */
#include "command.h"
#include "pffs.h"
#include "pffs/flash_file.h"
#include "pffs/flash_log.h"
#include "pffs/transfer.h"
#include "pffs/load.h"
#include "ir_file.h"
#include <pogobot.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifndef REMOCON
#include <generated/csr.h>
#endif

static struct {
    pogo_flash_log_t log; /* One shared page cache, including UART transfers. */
    pffs_transfer_t transfer;
} state;
static time_reference_t clock_origin;
static uint32_t now_ms;
static bool input_reset;

/** Check a raw catalog, including the checksum of an entirely empty page.
 *
 * File lookup intentionally skips CRC work and returns NOT_FOUND for an empty
 * slot. This inventory mirrors the documented PFFS v4 header and CRC layout
 * locally without adding diagnostic code to the mission firmware reader.
 * `scratch` is reusable shared storage.
 */
static pogo_flash_file_status_t
check_catalog(uint8_t catalog_index,
              uint8_t scratch[POGO_FLASH_FILE_PAGE_SIZE]) {
    static const uint8_t magic[4] = {'P', 'F', 'F', 'S'};
    const unsigned crc_offset = POGO_FLASH_FILE_PAGE_SIZE - sizeof(uint32_t);
    bool all_zero = true;
    bool all_erased = true;
    read_page_flash((uint16_t)catalog_index *
                        POGO_FLASH_FILE_ERASE_SECTOR_PAGES,
                    (char *)scratch);
    for (unsigned i = 0u; i < POGO_FLASH_FILE_PAGE_SIZE; ++i) {
        all_zero = all_zero && scratch[i] == 0u;
        all_erased = all_erased && scratch[i] == 0xffu;
    }
    if (memcmp(scratch, magic, sizeof(magic)) == 0 && scratch[4] != 0u &&
        scratch[4] != 4u)
        return POGO_FLASH_FILE_UNSUPPORTED_FORMAT;
    if (memcmp(scratch, magic, sizeof(magic)) != 0 || scratch[4] != 4u ||
        scratch[5] != catalog_index ||
        scratch[6] !=
            POGO_FLASH_FILE_MAX_FILES / POGO_FLASH_FILE_CATALOG_PAGES ||
        scratch[7] != 0u) {
        return all_zero || all_erased ? POGO_FLASH_FILE_UNFORMATTED
                                      : POGO_FLASH_FILE_CORRUPT_CATALOG;
    }
    /* CRC-32/ISO-HDLC covers bytes 0..251; the stored checksum is little
     * endian at bytes 252..255. This bitwise form needs no lookup table. */
    uint32_t crc = UINT32_MAX;
    for (unsigned i = 0u; i < crc_offset; ++i) {
        crc ^= scratch[i];
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    crc ^= UINT32_MAX;
    uint32_t stored = (uint32_t)scratch[crc_offset] |
                      ((uint32_t)scratch[crc_offset + 1u] << 8) |
                      ((uint32_t)scratch[crc_offset + 2u] << 16) |
                      ((uint32_t)scratch[crc_offset + 3u] << 24);
    if (crc != stored) {
        /* An empty slot can coexist with a bad page checksum. Emit only the
         * small fixed header/first-slot diagnostic needed to distinguish an
         * interrupted update from an invalidated catalog; do not dump payloads
         * or modify flash. Offsets follow the documented PFFS v4 layout. */
        uint32_t generation =
            (uint32_t)scratch[8] | ((uint32_t)scratch[9] << 8) |
            ((uint32_t)scratch[10] << 16) | ((uint32_t)scratch[11] << 24);
        printf("# FLASH_CATALOG_CRC,page=%u,stored=%08lx,calculated=%08lx,"
               "generation=%lu,first_id=%u,first_flags=%u\n",
               (unsigned)catalog_index, (unsigned long)stored,
               (unsigned long)crc, (unsigned long)generation,
               (unsigned)scratch[12], (unsigned)scratch[13]);
        return POGO_FLASH_FILE_CORRUPT_CATALOG;
    }
    return POGO_FLASH_FILE_OK;
}

/** Inspect one stable slot and update the summary counters.
 *
 * `find` supplies metadata even when the subsequent CRC scan fails. Ordinary
 * secure reads and log reads reuse the same shared page as scratch.
 */
static void list_file(uint8_t id, unsigned *found, unsigned *empty,
                      unsigned *errors) {
    pogo_flash_file_info_t info;
    pogo_flash_file_status_t status = pogo_flash_file_find(id, &info);
    if (status == POGO_FLASH_FILE_NOT_FOUND) {
        ++*empty;
        return;
    }
    if (status != POGO_FLASH_FILE_OK) {
        ++*errors;
        printf("# FLASH_FILE_ERROR,id=%u,reason=%s\n", (unsigned)id,
               pogo_flash_file_status_string(status));
        return;
    }

    ++*found;
    if (info.format_version == POGO_FLASH_LOG_FORMAT_VERSION) {
        /* A log's catalog CRC covers metadata, not its changing data extent.
         * Its dedicated reader checks each committed page instead. */
        pogo_flash_log_status_t result = pogo_flash_log_open(&state.log, id);
        /* open already verified the committed prefix; do not scan it twice. */
        uint32_t bytes = pogo_flash_log_committed_bytes(&state.log);
        unsigned pages = state.log.next_page;
        if (result != POGO_FLASH_LOG_OK)
            ++*errors;
        printf("# FLASH_LOG_FILE,id=%u,name=%s,first_page=%u,pages=%u,"
               "reserved_bytes=%lu,committed_pages=%u,committed_bytes=%lu,"
               "validation=%s\n",
               (unsigned)id, info.name_length ? info.name : "(unnamed)",
               (unsigned)info.first_page, (unsigned)info.page_count,
               (unsigned long)info.size_bytes, pages, (unsigned long)bytes,
               result == POGO_FLASH_LOG_OK ? "ok" : "damaged");
        return;
    }
    /* A secure read verifies the selected catalog page and every data page.
     * file_page 0 is sufficient because the check covers the whole file. */
    pogo_flash_file_status_t validation =
        pogo_flash_file_read_page_secure(id, 0u, state.log.page, NULL);
    if (validation != POGO_FLASH_FILE_OK)
        ++*errors;

    const char *name = info.name_length > 0u ? info.name : "(unnamed)";
    printf("# FLASH_FILE,id=%u,name=%s,format_version=%u,first_page=%u,"
           "pages=%u,bytes=%u,generation=%lu,data_crc32=%08lx,validation=%s\n",
           (unsigned)info.id, name, (unsigned)info.format_version,
           (unsigned)info.first_page, (unsigned)info.page_count,
           (unsigned)info.size_bytes, (unsigned long)info.generation,
           (unsigned long)info.data_crc32,
           pogo_flash_file_status_string(validation));
}

/** List every slot, checking catalog CRCs before treating holes as empty. */
static void shell_list(void) {
    unsigned found = 0u;
    unsigned empty = 0u;
    unsigned errors = 0u;
    unsigned unreadable = 0u; /* IDs skipped because their catalog failed. */
    printf("# FLASH_FILE_LIST_START,robot=%u,max_files=%u\n",
           (unsigned)pogobot_helper_getid(),
           (unsigned)POGO_FLASH_FILE_MAX_FILES);

    for (uint8_t catalog = 0u; catalog < POGO_FLASH_FILE_CATALOG_PAGES;
         ++catalog) {
        /* The per-ID fast lookup cannot see a bad CRC on an empty slot. Check
         * each catalog first and never report its IDs as healthy empties when
         * its header or checksum is invalid. */
        pogo_flash_file_status_t status =
            check_catalog(catalog, state.log.page);
        if (status != POGO_FLASH_FILE_OK) {
            ++errors;
            unreadable +=
                POGO_FLASH_FILE_MAX_FILES / POGO_FLASH_FILE_CATALOG_PAGES;
            printf("# FLASH_CATALOG_ERROR,page=%u,reason=%s\n",
                   (unsigned)catalog, pogo_flash_file_status_string(status));
            continue;
        }
        for (uint8_t offset = 0u;
             offset < POGO_FLASH_FILE_MAX_FILES / POGO_FLASH_FILE_CATALOG_PAGES;
             ++offset) {
            uint8_t id = (uint8_t)(catalog * (POGO_FLASH_FILE_MAX_FILES /
                                              POGO_FLASH_FILE_CATALOG_PAGES) +
                                   offset + 1u);
            list_file(id, &found, &empty, &errors);
        }
    }
    printf("# FLASH_FILE_LIST_END,robot=%u,found=%u,empty=%u,unreadable=%u,"
           "errors=%u\n",
           (unsigned)pogobot_helper_getid(), found, empty, unreadable, errors);
    /* Green means all catalogs and every discovered file passed CRC checks.
     * Violet marks a bad catalog or damaged data; no flash is changed. */
    pogobot_led_setColor(errors == 0u ? 0u : 25u, errors == 0u ? 25u : 0u,
                         errors == 0u ? 0u : 25u);
}

/** Strict decimal parser: no signs, whitespace, or wraparound are accepted. */
static bool parse_u32(const char *text, uint32_t *value) {
    if (text == NULL || *text == '\0')
        return false;
    uint32_t result = 0u;
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9')
            return false;
        uint32_t digit = (uint32_t)(*p - '0');
        if (result > (UINT32_MAX - digit) / 10u)
            return false;
        result = result * 10u + digit;
    }
    *value = result;
    return true;
}

/** Resolve a stable numeric ID or an exact case-sensitive catalog name. */
static pogo_flash_file_status_t resolve_file(const char *word,
                                             pogo_flash_file_info_t *info) {
    uint32_t id;
    if (parse_u32(word, &id)) {
        if (id == 0u || id > POGO_FLASH_FILE_MAX_FILES) {
            return POGO_FLASH_FILE_INVALID_ARGUMENT;
        }
        return pogo_flash_file_find((uint8_t)id, info);
    }
    return pogo_flash_file_find_by_name(word, info);
}

/** Validate all catalogs and their extents before create_blank: the library's
 * create API otherwise autoformats absent/corrupt metadata without asking. */
static bool catalogs_ready(void) {
    pogo_flash_file_status_t status = pogo_flash_file_check();
    if (status == POGO_FLASH_FILE_OK)
        return true;
    printf("error: %s; use 'format YES' only if erasure is intended\n",
           pogo_flash_file_status_string(status));
    return false;
}

/** Report sector allocation separately from reserved payload pages. A file
 * owns every sector intersecting its contiguous, fixed-size extent. */
static void shell_df(void) {
    if (!catalogs_ready())
        return;

    unsigned files_used = 0u;
    unsigned payload_pages = 0u;
    uint32_t file_bytes =
        0u; /* Logs contribute reserved capacity, not live payload. */
    unsigned sectors_used = 0u;
    for (uint8_t id = 1u; id <= POGO_FLASH_FILE_MAX_FILES; ++id) {
        pogo_flash_file_info_t info;
        pogo_flash_file_status_t status = pogo_flash_file_find(id, &info);
        if (status == POGO_FLASH_FILE_NOT_FOUND)
            continue;
        if (status != POGO_FLASH_FILE_OK) {
            printf("error: ID %u: %s\n", (unsigned)id,
                   pogo_flash_file_status_string(status));
            return;
        }
        ++files_used;
        payload_pages += info.page_count;
        file_bytes += info.size_bytes;
        sectors_used += ((unsigned)info.page_count +
                         POGO_FLASH_FILE_ERASE_SECTOR_PAGES - 1u) /
                        POGO_FLASH_FILE_ERASE_SECTOR_PAGES;
    }

    /* The SDK exposes all 5888 pages. Catalogs each own a whole sector; data
     * extents may span multiple sectors, so file count is not space usage. */
    const unsigned physical_pages = POGO_FLASH_FILE_USER_PAGES;
    const unsigned sector_kib =
        POGO_FLASH_FILE_ERASE_SECTOR_PAGES * POGO_FLASH_FILE_PAGE_SIZE / 1024u;
    const unsigned physical_kib =
        physical_pages * POGO_FLASH_FILE_PAGE_SIZE / 1024u;
    const unsigned catalog_kib =
        POGO_FLASH_FILE_DATA_FIRST_PAGE * POGO_FLASH_FILE_PAGE_SIZE / 1024u;
    const unsigned data_sectors =
        (physical_pages - POGO_FLASH_FILE_DATA_FIRST_PAGE) /
        POGO_FLASH_FILE_ERASE_SECTOR_PAGES;
    const unsigned size_kib = data_sectors * sector_kib;
    const unsigned used_kib = sectors_used * sector_kib;
    const unsigned avail_kib = size_kib - used_kib;
    const unsigned use_percent =
        data_sectors == 0u ? 0u : sectors_used * 100u / data_sectors;

    puts("Filesystem  Sector cap  Used  Avail  Use%  Files  Payload pages");
    printf("pffs        %2uK         %2uK   %2uK    %3u%%  %2u/%2u  %uB\n",
           size_kib, used_kib, avail_kib, use_percent, files_used,
           (unsigned)POGO_FLASH_FILE_MAX_FILES,
           payload_pages * (unsigned)POGO_FLASH_FILE_PAGE_SIZE);
    printf("Physical: %uK user flash; %uK catalog; %uK data extents.\n",
           physical_kib, catalog_kib, size_kib);
    printf("File lengths: %luB (log lengths reserve capacity; ls/stat show "
           "committed payload).\n",
           (unsigned long)file_bytes);
    puts("Payload pages include page padding; Used includes sector rounding. "
         "Free space may be fragmented.");
}

/** Print one file's stable identity and allocation; no payload is changed. */
static void shell_stat(const pogo_flash_file_info_t *info) {
    printf("id=%u name=%s format=%u page=%u pages=%u bytes=%lu "
           "allocated_bytes=%lu generation=%lu crc=%08lx\n",
           (unsigned)info->id, info->name_length ? info->name : "(unnamed)",
           (unsigned)info->format_version, (unsigned)info->first_page,
           (unsigned)info->page_count, (unsigned long)info->size_bytes,
           (unsigned long)info->page_count * POGO_FLASH_FILE_PAGE_SIZE,
           (unsigned long)info->generation, (unsigned long)info->data_crc32);
    if (info->format_version == POGO_FLASH_LOG_FORMAT_VERSION) {
        pogo_flash_log_status_t status =
            pogo_flash_log_open(&state.log, info->id);
        printf("committed_bytes=%lu validation=%s status=%u\n",
               (unsigned long)pogo_flash_log_committed_bytes(&state.log),
               status == POGO_FLASH_LOG_OK ? "ok" : "damaged",
               (unsigned)status);
    }
}

/** Preserve readable text while escaping binary log bytes that could alter
 * terminal state or truncate downstream text capture. */
static void print_log_byte(uint8_t byte) {
    if ((byte >= 32u && byte <= 126u) || byte == '\n' || byte == '\t') {
        putchar((int)byte);
    } else {
        printf("\\x%02x", (unsigned)byte);
    }
}

/** Hexadecimal dump keeps binary files and erased (0xff) padding unambiguous.
 * For an ordinary file, one secure read checks all pages before fast reads. */
static void shell_cat(const pogo_flash_file_info_t *info) {
    if (info->format_version == POGO_FLASH_LOG_FORMAT_VERSION) {
        pogo_flash_log_status_t status =
            pogo_flash_log_open(&state.log, info->id);
        if (status != POGO_FLASH_LOG_OK) {
            printf("error: log open status %u\n", (unsigned)status);
            return;
        }
        for (uint16_t page = 0u; page < info->page_count; ++page) {
            uint8_t used = 0u;
            status = pogo_flash_log_read_page(&state.log, page, state.log.page,
                                              &used);
            if (status == POGO_FLASH_LOG_END)
                break;
            if (status != POGO_FLASH_LOG_OK) {
                printf("\nerror: log page %u status %u\n", (unsigned)page,
                       (unsigned)status);
                return;
            }
            for (unsigned i = 0u; i < used; ++i) {
                print_log_byte(state.log.page[POGO_FLASH_LOG_HEADER_SIZE + i]);
            }
        }
        putchar('\n');
        return;
    }
    pogo_flash_file_status_t status =
        pogo_flash_file_read_page_secure(info->id, 0u, state.log.page, NULL);
    if (status != POGO_FLASH_FILE_OK) {
        printf("error: %s\n", pogo_flash_file_status_string(status));
        return;
    }
    for (uint16_t page = 0u; page < info->page_count; ++page) {
        if (page != 0u) {
            status = pogo_flash_file_read_page_fast(info->id, page,
                                                    state.log.page, NULL);
            if (status != POGO_FLASH_FILE_OK) {
                printf("error: page %u: %s\n", (unsigned)page,
                       pogo_flash_file_status_string(status));
                return;
            }
        }
        unsigned used =
            info->size_bytes - (uint32_t)page * POGO_FLASH_FILE_PAGE_SIZE;
        if (used > POGO_FLASH_FILE_PAGE_SIZE)
            used = POGO_FLASH_FILE_PAGE_SIZE;
        for (unsigned offset = 0u; offset < used; offset += 16u) {
            printf("%04x:",
                   (unsigned)page * POGO_FLASH_FILE_PAGE_SIZE + offset);
            for (unsigned i = 0u; i < 16u && offset + i < used; ++i) {
                printf(" %02x", (unsigned)state.log.page[offset + i]);
            }
            putchar('\n');
        }
    }
}

/** Turn one hex digit into its value without relying on locale or libc. */
static int hex_digit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/** One-page-only edit uses the existing 256-byte shared scratch buffer.
 * Multi-page replacement would require retaining up to 2 KiB through erase. */
static void shell_write(const pogo_flash_file_info_t *info,
                        const char *offset_word, const char *hex) {
    uint32_t offset;
    if (!parse_u32(offset_word, &offset) ||
        offset >= POGO_FLASH_FILE_PAGE_SIZE || info->page_count != 1u ||
        info->format_version == POGO_FLASH_LOG_FORMAT_VERSION) {
        puts("error: write needs a one-page ordinary file and offset 0..255");
        return;
    }
    size_t digits = strlen(hex);
    if (digits == 0u || (digits & 1u) != 0u ||
        digits / 2u > POGO_FLASH_FILE_PAGE_SIZE - offset) {
        puts("error: hex bytes are empty, odd-length, or exceed the page");
        return;
    }
    /* Check the entire input before reading/writing flash. */
    for (size_t i = 0u; i < digits; ++i) {
        if (hex_digit(hex[i]) < 0) {
            puts("error: write expects plain hexadecimal byte pairs");
            return;
        }
    }
    pogo_flash_file_status_t status =
        pogo_flash_file_read_page_secure(info->id, 0u, state.log.page, NULL);
    if (status != POGO_FLASH_FILE_OK) {
        printf("error: %s\n", pogo_flash_file_status_string(status));
        return;
    }
    for (size_t i = 0u; i < digits; i += 2u) {
        state.log.page[offset + i / 2u] =
            (uint8_t)((hex_digit(hex[i]) << 4) | hex_digit(hex[i + 1u]));
    }
    uint32_t size_bytes = offset + (uint32_t)(digits / 2u);
    if (size_bytes < info->size_bytes)
        size_bytes = info->size_bytes;
    status =
        pogo_flash_file_replace_bytes(info->id, size_bytes, state.log.page);
    printf("write: %s\n", pogo_flash_file_status_string(status));
}

/** Fixed-size, whitespace-delimited command dispatcher. Labels cannot
 * contain spaces, matching the stored exact-name lookup and small line cap. */
static void shell_execute(char **words, unsigned count) {
    if (count == 0u)
        return;
    /* Transfer frames precede ordinary file commands. A live session owns
     * the page scratch and PFFS until finish/abort/timeout, until the UART
     * session ends. */
    if (pffs_transfer_command(&state.transfer, &state.log, words, count,
                              now_ms))
        return;
    if (pffs_transfer_busy(&state.transfer)) {
        puts("transfer in progress; finish or abort it first");
        return;
    }
    const char *command = words[0];
#ifndef REMOCON
    if (strcmp(command, "load_run") == 0) {
        pogo_flash_file_info_t info;
        if (count != 2u || !bios_pffs_controller_name_valid(words[1])) {
            puts("Usage: load_run <name> (1..32 printable bytes, no spaces)");
        } else {
            /* Name-only lookup avoids silently selecting numeric file IDs.
             * A failed lookup/load must never boot a previously valid image. */
            pogo_flash_file_status_t status =
                pogo_flash_file_find_by_name(words[1], &info);
            if (status == POGO_FLASH_FILE_OK) {
                if (bios_pffs_load_controller(info.id, state.log.page)) {
                    reboot_ctrl_write(0xad); /* Same user image as run 1. */
                    return;
                }
            } else {
                printf("load_run: %s: %s\n", words[1],
                       pogo_flash_file_status_string(status));
            }
        }
        /* Show red immediately and retain red for subsequent idle blinks,
         * including when a UART connection has disabled the idle animator. */
        rgb_blink_set_color(50, 0, 0);
        pogobot_led_setColor(50, 0, 0);
        return;
    }
#endif
    if (strcmp(command, "ir_file_abort") == 0 && count == 1u) {
        ir_file_abort(); puts("IR file reservation released"); return;
    }
    if ((strcmp(command, "ir_send_file") == 0 || strcmp(command, "rc_send_file") == 0) &&
        count >= 2u && count <= 4u) {
        uint32_t passes = 1u; bool replace = false, have_passes = false;
        for (unsigned i = 2u; i < count; ++i) {
            if (strcmp(words[i], "--replace") == 0 && !replace) replace = true;
            else if (!have_passes && parse_u32(words[i], &passes) && passes > 0u && passes <= 255u)
                have_passes = true;
            else { puts("usage: ir_send_file/rc_send_file <id|name> [1..255 passes] [--replace]"); return; }
        }
        pogo_flash_file_info_t info;
        pogo_flash_file_status_t status = resolve_file(words[1], &info);
        if (status != POGO_FLASH_FILE_OK) printf("IR file: %s\n", pogo_flash_file_status_string(status));
        else ir_file_send(info.id, (unsigned)passes, replace, state.log.page);
        return;
    }
    if (strcmp(command, "help") == 0 && count == 1u) {
        puts("help | ls | df | stat <id|name> | cat <id|name> | rm <id|name>");
        puts("touch <id> <name|-> [bytes] | mv <id|name> <new_name|->");
        puts("write <id|name> <offset> <hexbytes> | format YES");
#ifndef REMOCON
        puts("load <id|name>: install raw controller; then execute run");
        puts("load_run <name>: install/start named controller (UART or IR)");
#endif
        puts("UART upload/download: Software/pffs_transfer.py (put/get, CRC, "
             "ACK)");
        puts("abort <session_hex> cancels a transfer; interrupted replacement "
             "can lose data.");
        puts("Ordinary cat is hex; log bytes are text with binary escaped.");
        puts("write supports one-page ordinary files only.");
#ifdef REMOCON
        puts("rc_send_file <id|name> [passes] [--replace]; Q cancels");
#else
        puts("ir_send_file <id|name> [passes] [--replace]; Q cancels");
        puts("ir_file_abort releases a pending IR reception; partial replacement can lose data.");
#endif
        return;
    }
    if (strcmp(command, "ls") == 0 && count == 1u) {
        shell_list();
        return;
    }
    if (strcmp(command, "df") == 0 && count == 1u) {
        shell_df();
        return;
    }
    if (strcmp(command, "format") == 0 && count == 2u &&
        strcmp(words[1], "YES") == 0) {
        /* Formatting drops all PFFS entries, including calibration, but does
         * not physically wipe data sectors until they are reused. */
        pogo_flash_file_status_t status = pogo_flash_file_format();
        printf("format: %s (all PFFS files removed; old data not securely "
               "wiped)\n",
               pogo_flash_file_status_string(status));
        return;
    }
    if (strcmp(command, "touch") == 0 && (count == 3u || count == 4u)) {
        uint32_t id, bytes = 0u;
        if (!parse_u32(words[1], &id) || id == 0u ||
            id > POGO_FLASH_FILE_MAX_FILES ||
            (count == 4u && !parse_u32(words[3], &bytes)) ||
            bytes > POGO_FLASH_FILE_MAX_BYTES) {
            puts("error: touch needs ID 1..80 and bytes 0..1441792");
            return;
        }
        if (!catalogs_ready())
            return;
        const char *name = strcmp(words[2], "-") == 0 ? "" : words[2];
        pogo_flash_file_status_t status =
            pogo_flash_file_create_blank_bytes((uint8_t)id, name, bytes, 1u);
        printf("touch: %s\n", pogo_flash_file_status_string(status));
        return;
    }
    if ((strcmp(command, "stat") == 0 || strcmp(command, "cat") == 0 ||
         strcmp(command, "rm") == 0) &&
        count == 2u) {
        pogo_flash_file_info_t info;
        pogo_flash_file_status_t status = resolve_file(words[1], &info);
        if (status != POGO_FLASH_FILE_OK) {
            printf("error: %s\n", pogo_flash_file_status_string(status));
            return;
        }
        if (strcmp(command, "stat") == 0)
            shell_stat(&info);
        else if (strcmp(command, "cat") == 0)
            shell_cat(&info);
        else {
            status = pogo_flash_file_delete(info.id);
            printf("rm: %s\n", pogo_flash_file_status_string(status));
        }
        return;
    }
#ifndef REMOCON
    /* Like serialboot, installation replaces the normal application image
     * but leaves the prompt and PFFS source available until run is requested. */
    if (strcmp(command, "load") == 0 && count == 2u) {
        pogo_flash_file_info_t info;
        pogo_flash_file_status_t status = resolve_file(words[1], &info);
        if (status != POGO_FLASH_FILE_OK)
            printf("load: %s\n", pogo_flash_file_status_string(status));
        else
            bios_pffs_load_controller(info.id, state.log.page);
        return;
    }
#endif
    if (strcmp(command, "mv") == 0 && count == 3u) {
        pogo_flash_file_info_t info;
        pogo_flash_file_status_t status = resolve_file(words[1], &info);
        if (status == POGO_FLASH_FILE_OK) {
            const char *name = strcmp(words[2], "-") == 0 ? "" : words[2];
            status = pogo_flash_file_rename(info.id, name);
        }
        printf("mv: %s\n", pogo_flash_file_status_string(status));
        return;
    }
    if (strcmp(command, "write") == 0 && count == 4u) {
        pogo_flash_file_info_t info;
        pogo_flash_file_status_t status = resolve_file(words[1], &info);
        if (status == POGO_FLASH_FILE_OK)
            shell_write(&info, words[2], words[3]);
        else
            printf("error: %s\n", pogo_flash_file_status_string(status));
        return;
    }
    puts("error: unknown command or arguments; use 'help'");
}

void bios_pffs_init(void) {
    /* Mounting is read-only and lazy: never format on boot or reset. */
    memset(&state, 0, sizeof(state));
    now_ms = 0;
    input_reset = false;
    pogobot_stopwatch_reset(&clock_origin);
}

bool bios_pffs_busy(void) {
    /* Compaction stays in pogolib for maintenance applications; the BIOS
     * reserves the filesystem only for UART and IR file sessions. */
    return pffs_transfer_busy(&state.transfer) || ir_file_busy();
}

bool bios_pffs_allows(const char *command) {
    if (ir_file_busy()) return strcmp(command, "ir_file_abort") == 0 ||
                               strcmp(command, "help") == 0;
    if (!pffs_transfer_busy(&state.transfer))
        return true;
    /* Transfers own the shared cache and flash until END, abort or timeout. */
    static const char *const protocol[] = {
        "put", "get", "putdata", "getdata", "putend", "getend", "abort"};
    for (unsigned i = 0; i < sizeof(protocol) / sizeof(protocol[0]); ++i)
        if (strcmp(command, protocol[i]) == 0)
            return true;
    return false;
}

void bios_pffs_service(void) {
    /* Ordinary BIOS/IR work pays no filesystem clock cost while idle. */
    if (!bios_pffs_busy())
        return;
    /* Advance the origin often, retaining fractional milliseconds. This avoids
     * the hardware counter's half-range limit during multi-minute transfers. */
    int32_t us = pogobot_stopwatch_get_elapsed_microseconds(&clock_origin);
    if (us < 0) {
        /* Only possible after a long synchronous command while no transfer ran.
         */
        pogobot_stopwatch_reset(&clock_origin);
    } else if (us >= 1000) {
        int32_t whole_ms = us / 1000;
        now_ms += (uint32_t)whole_ms;
        pogobot_stopwatch_offset_origin_microseconds(&clock_origin,
                                                     whole_ms * 1000);
    }
    bool active = pffs_transfer_busy(&state.transfer);
    pffs_transfer_poll(&state.transfer, now_ms);
    if (active && !pffs_transfer_busy(&state.transfer))
        input_reset = true;
}

bool bios_pffs_input_reset(void) {
    bool reset = input_reset;
    input_reset = false;
    return reset;
}

/* Native BIOS handlers share the existing dispatcher and console history. */
static void pffs_command(const char *name, int count, char **params) {
    char *words[MAX_PARAM + 1];
    if (count < 0 || count > MAX_PARAM)
        return;
    words[0] = (char *)name;
    for (int i = 0; i < count; ++i)
        words[i + 1] = params[i];
    if (!bios_pffs_allows(name)) {
        puts("PFFS busy; wait for completion or finish/abort the transfer");
        return;
    }
    if (!bios_pffs_busy()) {
        ir_file_abort(); /* Drop completed-file cache before local PFFS access. */
        /* Each new session starts with a nearby origin; old completed tokens
         * remain cached for idempotent END retries. No flash is touched. */
        now_ms = 0;
        pogobot_stopwatch_reset(&clock_origin);
    }
    shell_execute(words, (unsigned)count + 1u);
}
#define PFFS_COMMAND(name, help_text)                                          \
    static void name##_handler(int n, char **p) { pffs_command(#name, n, p); } \
    define_command(name, name##_handler, help_text, PFFS_CMDS)
#ifdef REMOCON
PFFS_COMMAND(rc_send_file, "rc_send_file <id|name> [passes] [--replace]");
#else
PFFS_COMMAND(ir_send_file, "ir_send_file <id|name> [passes] [--replace]");
PFFS_COMMAND(ir_file_abort, "Discard pending IR file and release PFFS");
#endif
PFFS_COMMAND(ls, "List PFFS files and validate CRCs");
PFFS_COMMAND(df, "PFFS sector and file capacity");
PFFS_COMMAND(stat, "stat <id|name>");
PFFS_COMMAND(cat, "cat <id|name> (hex file / escaped log)");
#ifndef REMOCON
PFFS_COMMAND(load, "load <id|name> (install raw controller; then run)");
/* This bounded boot action is intentionally available over IR. Ordinary
 * PFFS management remains UART-only, and the same session locks still apply. */
static void load_run_handler(int n, char **p) { pffs_command("load_run", n, p); }
define_command(load_run, load_run_handler, "Load/run named PFFS controller; red on failure", BOOT_CMDS);
#endif
PFFS_COMMAND(touch, "touch <id> <name|-> [bytes]");
PFFS_COMMAND(write, "write <id|name> <offset> <hexbytes> (one page)");
PFFS_COMMAND(mv, "mv <id|name> <new_name|->");
PFFS_COMMAND(rm, "rm <id|name>");
PFFS_COMMAND(format, "format YES (delete all PFFS files)");
PFFS_COMMAND(put, "UART file upload: use pffs_transfer.py");
PFFS_COMMAND(get, "UART file download: use pffs_transfer.py");
PFFS_COMMAND(putdata, "UART upload chunk");
PFFS_COMMAND(getdata, "UART download chunk");
PFFS_COMMAND(putend, "Finish UART upload");
PFFS_COMMAND(getend, "Finish UART download");
PFFS_COMMAND(abort, "abort <session_hex> (cancel UART transfer)");
static void pffs_handler(int n, char **p) {
    if (n != 0) {
        puts("Usage: pffs (filesystem command help)");
        return;
    }
    pffs_command("help", 0, p);
}
define_command(pffs, pffs_handler, "PFFS filesystem command help", PFFS_CMDS);
