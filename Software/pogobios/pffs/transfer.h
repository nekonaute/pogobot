/** @file transfer.h
 * Bounded, stop-and-wait UART file transfers for the flash shell only.
 * PFFS v4 records exact byte lengths. Raw uploads use ordinary payload type 1;
 * optional enveloped uploads use type 0x8002 and a 16-byte PFT1 prefix:
 * magic, LE byte length, LE content CRC32, zero. Downloads stop at catalog EOF,
 * unwrap envelopes, concatenate logs, or explicitly export allocated pages.
 *
 * Call command() only after tokenizing a complete bounded ASCII line; call
 * poll() regularly. While busy(), block ALL other shell/PFFS commands, including
 * simulator robot switching. Commands are serialized, not ISR/thread-safe.
 * A disconnected replacement can damage the old file: abort is not rollback.
 */
#ifndef PFFS_SHELL_TRANSFER_H
#define PFFS_SHELL_TRANSFER_H
#include <pffs/flash_log.h>

enum { PFT_CHUNK = 48, PFT_HEADER = 16, PFT_FORMAT = 0x8002,
       PFT_TIMEOUT_MS = 30000 };

/** Caller-owned transfer state. The shell lends its existing log page cache;
 * only the last small chunk is retained separately for idempotent retries. */
typedef struct {
    union {
        pogo_flash_file_writer_t writer;
        pogo_flash_file_info_t info;
    } file;
    uint32_t token, length, crc, position, running_crc, activity_ms;
    uint32_t last_offset, last_crc, signature;
    uint16_t page, used, page_end;
    uint8_t last[PFT_CHUNK];
    uint8_t last_length, mode;
    bool raw, log, loaded;
} pffs_transfer_t;

bool pffs_transfer_busy(const pffs_transfer_t *transfer);
/** Returns true if this is a transfer command, even if rejected. Replies are
 * newline-terminated PFT1 frames on stdout, tagged with the host's nonce. */
bool pffs_transfer_command(pffs_transfer_t *transfer, pogo_flash_log_t *scratch,
                           char **words, unsigned count, uint32_t now_ms);
void pffs_transfer_poll(pffs_transfer_t *transfer, uint32_t now_ms);
#endif
