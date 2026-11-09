/** @file transfer.c
 * ASCII frames fit the shell's 159-byte command limit. CRC-32/ISO-HDLC covers
 * each chunk plus its LE session/offset; a second CRC covers complete content.
 * Every ACK follows page programming/readback, preventing UART input overruns
 * while flash is busy. Duplicate last chunks never program a NOR page twice.
 */
#include "transfer.h"
#include <stdio.h>
#include <string.h>

enum { IDLE, PUT, GET, PUT_DONE, GET_DONE };

static uint32_t crc_update(uint32_t crc, const uint8_t *bytes, size_t length) {
    while (length-- != 0u) {
        crc ^= *bytes++;
        for (unsigned bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0u - (crc & 1u)));
    }
    return crc;
}
static void put32(uint8_t *out, uint32_t value) {
    for (unsigned i = 0; i < 4u; ++i) out[i] = (uint8_t)(value >> (8u * i));
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
/** Strict bounded parsing, without strtoul's sign/overflow ambiguities. */
static bool number(const char *s, unsigned base, uint32_t *value) {
    uint32_t v = 0u;
    if (*s == '\0') return false;
    for (; *s != '\0'; ++s) {
        int d = hex_digit(*s);
        if (d < 0 || (unsigned)d >= base || v > (UINT32_MAX - (unsigned)d) / base)
            return false;
        v = v * base + (unsigned)d;
    }
    *value = v;
    return true;
}
static uint32_t chunk_crc(uint32_t token, uint32_t offset,
                           const uint8_t *bytes, size_t length) {
    uint8_t prefix[8];
    put32(prefix, token); put32(prefix + 4, offset);
    return crc_update(crc_update(UINT32_MAX, prefix, sizeof(prefix)), bytes, length) ^ UINT32_MAX;
}
/** Bind begin retries to the complete request, not just a matching nonce.
 * NUL separators make adjacent argument combinations unambiguous. */
static uint32_t signature(char **words, unsigned count) {
    uint32_t crc = UINT32_MAX;
    for (unsigned i = 0; i < count; ++i)
        crc = crc_update(crc, (const uint8_t *)words[i], strlen(words[i]) + 1u);
    return crc ^ UINT32_MAX;
}
static void error(uint32_t token, const char *reason) {
    printf("PFT1 %08lx ERROR %s\n", (unsigned long)token, reason);
}
bool pffs_transfer_busy(const pffs_transfer_t *t) {
    return t->mode == PUT || t->mode == GET;
}
static void abort_transfer(pffs_transfer_t *t) {
    if (t->mode == PUT) pogo_flash_file_write_abort(&t->file.writer);
    t->mode = IDLE;
}
static void fatal(pffs_transfer_t *t, const char *reason) {
    error(t->token, reason);
    abort_transfer(t);
}
void pffs_transfer_poll(pffs_transfer_t *t, uint32_t now) {
    if (pffs_transfer_busy(t) && (uint32_t)(now - t->activity_ms) >= PFT_TIMEOUT_MS)
        fatal(t, "TIMEOUT");
}
static void ready(const pffs_transfer_t *t, const char *kind) {
    /* Echo the complete begin-request fingerprint before the host sends data;
     * corruption of an ID/name/mode must not redirect an upload silently. */
    printf("PFT1 %08lx %s %lu %08lx %u %08lx\n", (unsigned long)t->token, kind,
           (unsigned long)t->length, (unsigned long)t->crc, (unsigned)PFT_CHUNK,
           (unsigned long)t->signature);
}
static void done(const pffs_transfer_t *t, const char *kind) {
    printf("PFT1 %08lx DONE %s %lu %08lx\n", (unsigned long)t->token, kind,
           (unsigned long)t->length, (unsigned long)t->crc);
}
static void ack(const pffs_transfer_t *t) {
    printf("PFT1 %08lx ACK %lu\n", (unsigned long)t->token, (unsigned long)t->position);
}

/** Strict metadata preflight: never invoke create's implicit autoformat on a
 * blank/corrupt filesystem. The shell serializes all access during transfer. */
static bool catalogs_valid(void) {
    return pogo_flash_file_check() == POGO_FLASH_FILE_OK;
}

static void start_put(pffs_transfer_t *t, pogo_flash_log_t *scratch,
                       char **w, unsigned count, uint32_t token, uint32_t now) {
    uint32_t id, length, crc;
    if (count != 7u || !number(w[2], 10u, &id) || id == 0u ||
        id > POGO_FLASH_FILE_MAX_FILES || !number(w[4], 10u, &length) ||
        !number(w[5], 16u, &crc)) { error(token, "ARGS"); return; }
    bool raw = strcmp(w[6], "raw") == 0 || strcmp(w[6], "raw-replace") == 0;
    bool replace = strcmp(w[6], "replace") == 0 || strcmp(w[6], "raw-replace") == 0;
    if (!raw && !replace && strcmp(w[6], "new") != 0) { error(token, "MODE"); return; }
    uint32_t header = raw ? 0u : PFT_HEADER;
    if (length > (uint32_t)POGO_FLASH_FILE_MAX_PAGES * 256u - header) {
        error(token, "SIZE"); return;
    }
    uint16_t pages = (uint16_t)((length + header + 255u) / 256u);
    if (pages == 0u) pages = 1u;
    const char *name = strcmp(w[3], "-") == 0 ? "" : w[3];
    if (pffs_transfer_busy(t)) {
        /* A lost READY may be retried before any data has been sent. */
        if (t->mode == PUT && token == t->token && t->position == 0u &&
            t->signature == signature(w, count))
            ready(t, "PUT");
        else error(token, "BUSY");
        return;
    }
    if (!catalogs_valid()) { error(token, "CATALOG"); return; }
    pogo_flash_file_info_t info;
    pogo_flash_file_status_t status;
    if (replace) {
        status = pogo_flash_file_find((uint8_t)id, &info);
        if (status != POGO_FLASH_FILE_OK) { error(token, "NOT_FOUND"); return; }
        if (info.page_count != pages) { error(token, "SIZE"); return; }
        if (strcmp(info.name, name) != 0) { error(token, "NAME"); return; }
        if (info.format_version == POGO_FLASH_LOG_FORMAT_VERSION ||
            (raw ? info.format_version == PFT_FORMAT : info.format_version != PFT_FORMAT)) {
            error(token, "FORMAT"); return;
        }
    }
    memset(t, 0, sizeof(*t));
    t->token = token; t->length = length; t->crc = crc; t->raw = raw;
    t->signature = signature(w, count); t->page_end = pages;
    t->running_crc = UINT32_MAX; t->activity_ms = now;
    /* v4 stores EOF in the catalog, including for header-free uploads. */
    status = replace ? pogo_flash_file_write_begin_replace_bytes(&t->file.writer, (uint8_t)id, length + header) :
        pogo_flash_file_write_begin_create_bytes(&t->file.writer, (uint8_t)id, name,
                                                 length + header, raw ? 1u : PFT_FORMAT, NULL);
    if (status != POGO_FLASH_FILE_OK) { error(token, "CREATE"); return; }
    t->mode = PUT;
    memset(scratch->page, 0xff, 256u);
    if (!raw) {
        memcpy(scratch->page, "PFT1", 4u);
        put32(scratch->page + 4, length); put32(scratch->page + 8, crc);
        put32(scratch->page + 12, 0u);
        t->used = PFT_HEADER;
    }
    ready(t, "PUT");
}

/** Copy a verified chunk into the shared page; erase/program only upon filling
 * a page. A 48-byte packet can fill at most one page. Finish handles the tail. */
static bool append(pffs_transfer_t *t, uint8_t page[256], const uint8_t *data, size_t n) {
    while (n != 0u) {
        size_t count = 256u - t->used;
        if (count > n) count = n;
        memcpy(page + t->used, data, count);
        t->used = (uint16_t)(t->used + count); data += count; n -= count;
        if (t->used == 256u) {
            if (pogo_flash_file_write_page(&t->file.writer, page) != POGO_FLASH_FILE_OK)
                return false;
            ++t->page;
            t->used = 0u;
            memset(page, 0xff, 256u);
        }
    }
    return true;
}
static void put_data(pffs_transfer_t *t, pogo_flash_log_t *scratch, char **w, unsigned count) {
    uint32_t offset, crc;
    if (count != 5u || !number(w[2], 10u, &offset) || !number(w[4], 16u, &crc)) {
        error(t->token, "ARGS"); return;
    }
    size_t digits = strlen(w[3]), n = digits / 2u;
    if (n == 0u || n > PFT_CHUNK || digits % 2u != 0u) { error(t->token, "SIZE"); return; }
    /* Decode into the small chunk cache, never directly into an unverified page. */
    uint8_t bytes[PFT_CHUNK];
    for (size_t i = 0; i < n; ++i) {
        int hi = hex_digit(w[3][i * 2u]), lo = hex_digit(w[3][i * 2u + 1u]);
        if (hi < 0 || lo < 0) { error(t->token, "HEX"); return; }
        bytes[i] = (uint8_t)((hi << 4) | lo);
    }
    if (chunk_crc(t->token, offset, bytes, n) != crc) { error(t->token, "CRC"); return; }
    if (t->last_length == n && offset == t->last_offset && crc == t->last_crc) {
        ack(t); return; /* Lost ACK: no repeated CRC accumulation/page write. */
    }
    if (t->mode != PUT || offset != t->position || n > t->length - t->position) {
        error(t->token, "OFFSET"); return;
    }
    if (!append(t, scratch->page, bytes, n)) { fatal(t, "WRITE"); return; }
    t->running_crc = crc_update(t->running_crc, bytes, n);
    t->last_offset = offset; t->last_crc = crc; t->last_length = (uint8_t)n;
    t->position += (uint32_t)n;
    ack(t);
}

static void finish_put(pffs_transfer_t *t, pogo_flash_log_t *scratch) {
    if (t->mode == PUT_DONE) { done(t, "PUT"); return; }
    if (t->position != t->length) { error(t->token, "INCOMPLETE"); return; }
    if ((t->running_crc ^ UINT32_MAX) != t->crc) { fatal(t, "CHECKSUM"); return; }
    /* Includes an all-0xff page for an empty raw upload. */
    if (t->page < t->page_end &&
        pogo_flash_file_write_page(&t->file.writer, scratch->page) != POGO_FLASH_FILE_OK) {
        fatal(t, "WRITE"); return;
    }
    if (pogo_flash_file_write_finish(&t->file.writer) != POGO_FLASH_FILE_OK) {
        fatal(t, "COMMIT"); return;
    }
    t->mode = PUT_DONE;
    done(t, "PUT");
}

/** Load one source page. Log payload is variable length; ordinary pages are
 * fixed length, with a possible envelope prefix on page zero. */
static bool load_page(pffs_transfer_t *t, pogo_flash_log_t *scratch) {
    if (t->page >= t->file.info.page_count) return false;
    t->page_end = 256u;
    if (t->log) {
        uint8_t used;
        if (pogo_flash_log_read_page(scratch, t->page, scratch->page, &used) != POGO_FLASH_LOG_OK)
            return false;
        t->page_end = (uint16_t)(POGO_FLASH_LOG_HEADER_SIZE + used);
    } else if (pogo_flash_file_read_page_fast(t->file.info.id, t->page,
                                              scratch->page, NULL) != POGO_FLASH_FILE_OK) return false;
    t->loaded = true;
    return true;
}
static bool read_chunk(pffs_transfer_t *t, pogo_flash_log_t *scratch, size_t n) {
    size_t copied = 0u;
    while (copied < n) {
        if (!t->loaded && !load_page(t, scratch)) return false;
        size_t count = t->page_end - t->used;
        if (count > n - copied) count = n - copied;
        memcpy(t->last + copied, scratch->page + t->used, count);
        copied += count; t->used = (uint16_t)(t->used + count);
        if (t->used == t->page_end) {
            ++t->page; t->used = t->log ? POGO_FLASH_LOG_HEADER_SIZE : 0u;
            t->loaded = false;
        }
    }
    return true;
}

static void start_get(pffs_transfer_t *t, pogo_flash_log_t *scratch,
                       char **w, unsigned count, uint32_t token, uint32_t now) {
    if (count != 4u || (strcmp(w[3], "file") != 0 && strcmp(w[3], "raw") != 0)) {
        error(token, "ARGS"); return;
    }
    if (pffs_transfer_busy(t)) {
        if (t->mode == GET && token == t->token && t->position == 0u &&
            t->signature == signature(w, count)) ready(t, "GET");
        else error(token, "BUSY");
        return;
    }
    pogo_flash_file_info_t info;
    uint32_t id;
    pogo_flash_file_status_t status = number(w[2], 10u, &id) ?
        (id == 0u || id > POGO_FLASH_FILE_MAX_FILES ? POGO_FLASH_FILE_INVALID_ARGUMENT :
         pogo_flash_file_find((uint8_t)id, &info)) : pogo_flash_file_find_by_name(w[2], &info);
    if (status != POGO_FLASH_FILE_OK) { error(token, "NOT_FOUND"); return; }
    bool is_log = info.format_version == POGO_FLASH_LOG_FORMAT_VERSION;
    if (is_log ? pogo_flash_log_open(scratch, info.id) != POGO_FLASH_LOG_OK :
        pogo_flash_file_read_page_secure(info.id, 0u, scratch->page, &info) != POGO_FLASH_FILE_OK) {
        error(token, "READ"); return;
    }
    memset(t, 0, sizeof(*t));
    t->file.info = info; t->token = token; t->activity_ms = now;
    t->signature = signature(w, count);
    t->raw = strcmp(w[3], "raw") == 0;
    t->log = is_log && !t->raw;
    t->length = t->raw ? (uint32_t)info.page_count * 256u : info.size_bytes;
    uint32_t expected = 0u;
    bool envelope = info.format_version == PFT_FORMAT && !t->raw;
    if (envelope) {
        if (t->length < PFT_HEADER || memcmp(scratch->page, "PFT1", 4u) != 0 || get32(scratch->page + 12) != 0u ||
            get32(scratch->page + 4) > t->length - PFT_HEADER) {
            error(token, "HEADER"); return;
        }
        t->length = get32(scratch->page + 4); expected = get32(scratch->page + 8);
        t->used = PFT_HEADER;
    }
    if (t->log) {
        t->length = pogo_flash_log_committed_bytes(scratch);
        t->used = POGO_FLASH_LOG_HEADER_SIZE;
    }
    /* Read-only preflight computes the wire-content CRC before READY. The
     * ordinary PFFS CRC or per-page log CRC has already validated flash data. */
    uint16_t first_offset = t->used;
    uint32_t left = t->length, crc = UINT32_MAX;
    while (left != 0u) {
        size_t n = left < PFT_CHUNK ? left : PFT_CHUNK;
        if (!read_chunk(t, scratch, n)) { error(token, "READ"); return; }
        crc = crc_update(crc, t->last, n); left -= (uint32_t)n;
    }
    t->crc = crc ^ UINT32_MAX;
    if (envelope && t->crc != expected) { error(token, "CHECKSUM"); return; }
    t->page = 0u; t->used = first_offset; t->loaded = false;
    t->mode = GET;
    ready(t, "GET");
}
static void data_reply(const pffs_transfer_t *t) {
    static const char alphabet[] = "0123456789abcdef";
    char hex[PFT_CHUNK * 2u + 1u];
    for (unsigned i = 0; i < t->last_length; ++i) {
        hex[2u * i] = alphabet[t->last[i] >> 4];
        hex[2u * i + 1u] = alphabet[t->last[i] & 15u];
    }
    hex[t->last_length * 2u] = '\0';
    printf("PFT1 %08lx DATA %lu %s %08lx\n", (unsigned long)t->token,
           (unsigned long)t->last_offset, hex, (unsigned long)t->last_crc);
}

bool pffs_transfer_command(pffs_transfer_t *t, pogo_flash_log_t *scratch,
                           char **w, unsigned count, uint32_t now) {
    if (count == 0u) return false;
    bool begin_put = strcmp(w[0], "put") == 0, begin_get = strcmp(w[0], "get") == 0;
    bool data_put = strcmp(w[0], "putdata") == 0, data_get = strcmp(w[0], "getdata") == 0;
    bool end_put = strcmp(w[0], "putend") == 0, end_get = strcmp(w[0], "getend") == 0;
    bool cancel = strcmp(w[0], "abort") == 0;
    if (!begin_put && !begin_get && !data_put && !data_get && !end_put && !end_get && !cancel)
        return false;
    uint32_t token;
    if (count < 2u || !number(w[1], 16u, &token) || token == 0u) {
        error(0u, "TOKEN"); return true;
    }
    if (token == t->token && pffs_transfer_busy(t)) t->activity_ms = now;
    if (begin_put) start_put(t, scratch, w, count, token, now);
    else if (begin_get) start_get(t, scratch, w, count, token, now);
    else if (token != t->token || t->mode == IDLE) error(token, "SESSION");
    else if (cancel && count == 2u) {
        abort_transfer(t);
        printf("PFT1 %08lx ABORT\n", (unsigned long)token);
    } else if (data_put && (t->mode == PUT || t->mode == PUT_DONE)) put_data(t, scratch, w, count);
    else if (end_put && count == 2u && (t->mode == PUT || t->mode == PUT_DONE)) finish_put(t, scratch);
    else if (data_get && count == 3u && t->mode == GET) {
        uint32_t offset;
        if (!number(w[2], 10u, &offset)) error(token, "ARGS");
        else if (t->last_length != 0u && offset == t->last_offset) data_reply(t);
        else if (offset != t->position || offset >= t->length) error(token, "OFFSET");
        else {
            size_t n = t->length - offset;
            if (n > PFT_CHUNK) n = PFT_CHUNK;
            if (!read_chunk(t, scratch, n)) fatal(t, "READ");
            else {
                t->last_offset = offset; t->last_length = (uint8_t)n;
                t->last_crc = chunk_crc(token, offset, t->last, n);
                t->position += (uint32_t)n;
                data_reply(t);
            }
        }
    } else if (end_get && count == 3u && (t->mode == GET || t->mode == GET_DONE)) {
        uint32_t crc;
        if (!number(w[2], 16u, &crc)) error(token, "ARGS");
        else if (t->position != t->length) error(token, "INCOMPLETE");
        else if (crc != t->crc) error(token, "CHECKSUM");
        else { t->mode = GET_DONE; done(t, "GET"); }
    } else error(token, "STATE");
    return true;
}
