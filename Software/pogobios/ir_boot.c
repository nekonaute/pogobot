/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <system.h>
#include <string.h>
#include <irq.h>

#include <generated/mem.h>
#include <generated/csr.h>
#include <generated/soc.h>

#include <sfl.h>
#include <boot.h>
#include <ir_boot.h>
#include <ir_upload_v2.h>
#include <ir_uart.h>
#include <libbase/crc.h>
#include <libbase/spiflash.h>
#include <pogobot.h>

// Versioned IR uploads are restricted to the current gateware and firmware slots.
#define IR_FLASH_START 0x40000u
#define IR_FLASH_END 0x80000u

const char * ir_magic_req = IR_MAGIC_REQ;

static struct {
    uint32_t frames;
    uint32_t accepted;
    uint32_t duplicates;
    uint32_t malformed;
    uint32_t crc_errors;
    uint32_t gaps;  // Kept at zero for compatibility with existing upload logs.
    uint32_t erases;
    uint32_t erase_us;
    uint32_t erase_max_us;
    uint32_t write_us;
    uint32_t write_max_us;
    uint32_t process_us;
    uint32_t process_max_us;
    uint32_t fec_recovered;
    uint32_t fec_decode_us;
    uint32_t fec_decode_max_us;
} upload_stats;

/* One versioned image fits either 128 KiB user slot and needs at most 256 bitmap bytes. */
static struct {
    uint32_t id;
    uint32_t offset;
    uint32_t length;
    uint32_t expected_crc;
    uint16_t chunks;
    uint16_t received;
    uint8_t active;
    uint8_t failed;
    uint8_t complete;
    uint8_t fec_enabled;
    uint8_t fec_mask;
    uint16_t fec_group;
    uint8_t fec_parity[IR_V2_FEC_PARITY_COUNT][IR_V2_CHUNK_SIZE];
    uint8_t bitmap[(IR_V2_MAX_CHUNKS + 7) / 8];
} v2_upload;

/* Cauchy matrix c[row][data] = inverse(data XOR (16 + row)) in GF(256).
 * The primitive polynomial is x^8+x^4+x^3+x^2+1 (0x11d). */
static const uint8_t fec_coefficients[IR_V2_FEC_PARITY_COUNT][IR_V2_FEC_DATA_COUNT] = {
    {0xd8, 0x72, 0xc0, 0x58, 0xe0, 0x3e, 0x4c, 0x66,
     0x90, 0xde, 0x55, 0x80, 0xa0, 0x83, 0x4b, 0x2a},
    {0x72, 0xd8, 0x58, 0xc0, 0x3e, 0xe0, 0x66, 0x4c,
     0xde, 0x90, 0x80, 0x55, 0x83, 0xa0, 0x2a, 0x4b},
    {0xc0, 0x58, 0xd8, 0x72, 0x4c, 0x66, 0xe0, 0x3e,
     0x55, 0x80, 0x90, 0xde, 0x4b, 0x2a, 0xa0, 0x83},
    {0x58, 0xc0, 0x72, 0xd8, 0x66, 0x4c, 0x3e, 0xe0,
     0x80, 0x55, 0xde, 0x90, 0x2a, 0x4b, 0x83, 0xa0},
};

// Powers of primitive element 2 and their logs in GF(256)/0x11d.
// The linker copies .rodata into SRAM: these tables occupy 512 bytes in both
// the flash image and SRAM, replacing eight bitwise rounds per multiply.
static const uint8_t fec_gf_exp[256] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1d, 0x3a, 0x74, 0xe8, 0xcd, 0x87, 0x13, 0x26,
    0x4c, 0x98, 0x2d, 0x5a, 0xb4, 0x75, 0xea, 0xc9, 0x8f, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0,
    0x9d, 0x27, 0x4e, 0x9c, 0x25, 0x4a, 0x94, 0x35, 0x6a, 0xd4, 0xb5, 0x77, 0xee, 0xc1, 0x9f, 0x23,
    0x46, 0x8c, 0x05, 0x0a, 0x14, 0x28, 0x50, 0xa0, 0x5d, 0xba, 0x69, 0xd2, 0xb9, 0x6f, 0xde, 0xa1,
    0x5f, 0xbe, 0x61, 0xc2, 0x99, 0x2f, 0x5e, 0xbc, 0x65, 0xca, 0x89, 0x0f, 0x1e, 0x3c, 0x78, 0xf0,
    0xfd, 0xe7, 0xd3, 0xbb, 0x6b, 0xd6, 0xb1, 0x7f, 0xfe, 0xe1, 0xdf, 0xa3, 0x5b, 0xb6, 0x71, 0xe2,
    0xd9, 0xaf, 0x43, 0x86, 0x11, 0x22, 0x44, 0x88, 0x0d, 0x1a, 0x34, 0x68, 0xd0, 0xbd, 0x67, 0xce,
    0x81, 0x1f, 0x3e, 0x7c, 0xf8, 0xed, 0xc7, 0x93, 0x3b, 0x76, 0xec, 0xc5, 0x97, 0x33, 0x66, 0xcc,
    0x85, 0x17, 0x2e, 0x5c, 0xb8, 0x6d, 0xda, 0xa9, 0x4f, 0x9e, 0x21, 0x42, 0x84, 0x15, 0x2a, 0x54,
    0xa8, 0x4d, 0x9a, 0x29, 0x52, 0xa4, 0x55, 0xaa, 0x49, 0x92, 0x39, 0x72, 0xe4, 0xd5, 0xb7, 0x73,
    0xe6, 0xd1, 0xbf, 0x63, 0xc6, 0x91, 0x3f, 0x7e, 0xfc, 0xe5, 0xd7, 0xb3, 0x7b, 0xf6, 0xf1, 0xff,
    0xe3, 0xdb, 0xab, 0x4b, 0x96, 0x31, 0x62, 0xc4, 0x95, 0x37, 0x6e, 0xdc, 0xa5, 0x57, 0xae, 0x41,
    0x82, 0x19, 0x32, 0x64, 0xc8, 0x8d, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xdd, 0xa7, 0x53, 0xa6,
    0x51, 0xa2, 0x59, 0xb2, 0x79, 0xf2, 0xf9, 0xef, 0xc3, 0x9b, 0x2b, 0x56, 0xac, 0x45, 0x8a, 0x09,
    0x12, 0x24, 0x48, 0x90, 0x3d, 0x7a, 0xf4, 0xf5, 0xf7, 0xf3, 0xfb, 0xeb, 0xcb, 0x8b, 0x0b, 0x16,
    0x2c, 0x58, 0xb0, 0x7d, 0xfa, 0xe9, 0xcf, 0x83, 0x1b, 0x36, 0x6c, 0xd8, 0xad, 0x47, 0x8e, 0x01
};
static const uint8_t fec_gf_log[256] = {
    0x00, 0x00, 0x01, 0x19, 0x02, 0x32, 0x1a, 0xc6, 0x03, 0xdf, 0x33, 0xee, 0x1b, 0x68, 0xc7, 0x4b,
    0x04, 0x64, 0xe0, 0x0e, 0x34, 0x8d, 0xef, 0x81, 0x1c, 0xc1, 0x69, 0xf8, 0xc8, 0x08, 0x4c, 0x71,
    0x05, 0x8a, 0x65, 0x2f, 0xe1, 0x24, 0x0f, 0x21, 0x35, 0x93, 0x8e, 0xda, 0xf0, 0x12, 0x82, 0x45,
    0x1d, 0xb5, 0xc2, 0x7d, 0x6a, 0x27, 0xf9, 0xb9, 0xc9, 0x9a, 0x09, 0x78, 0x4d, 0xe4, 0x72, 0xa6,
    0x06, 0xbf, 0x8b, 0x62, 0x66, 0xdd, 0x30, 0xfd, 0xe2, 0x98, 0x25, 0xb3, 0x10, 0x91, 0x22, 0x88,
    0x36, 0xd0, 0x94, 0xce, 0x8f, 0x96, 0xdb, 0xbd, 0xf1, 0xd2, 0x13, 0x5c, 0x83, 0x38, 0x46, 0x40,
    0x1e, 0x42, 0xb6, 0xa3, 0xc3, 0x48, 0x7e, 0x6e, 0x6b, 0x3a, 0x28, 0x54, 0xfa, 0x85, 0xba, 0x3d,
    0xca, 0x5e, 0x9b, 0x9f, 0x0a, 0x15, 0x79, 0x2b, 0x4e, 0xd4, 0xe5, 0xac, 0x73, 0xf3, 0xa7, 0x57,
    0x07, 0x70, 0xc0, 0xf7, 0x8c, 0x80, 0x63, 0x0d, 0x67, 0x4a, 0xde, 0xed, 0x31, 0xc5, 0xfe, 0x18,
    0xe3, 0xa5, 0x99, 0x77, 0x26, 0xb8, 0xb4, 0x7c, 0x11, 0x44, 0x92, 0xd9, 0x23, 0x20, 0x89, 0x2e,
    0x37, 0x3f, 0xd1, 0x5b, 0x95, 0xbc, 0xcf, 0xcd, 0x90, 0x87, 0x97, 0xb2, 0xdc, 0xfc, 0xbe, 0x61,
    0xf2, 0x56, 0xd3, 0xab, 0x14, 0x2a, 0x5d, 0x9e, 0x84, 0x3c, 0x39, 0x53, 0x47, 0x6d, 0x41, 0xa2,
    0x1f, 0x2d, 0x43, 0xd8, 0xb7, 0x7b, 0xa4, 0x76, 0xc4, 0x17, 0x49, 0xec, 0x7f, 0x0c, 0x6f, 0xf6,
    0x6c, 0xa1, 0x3b, 0x52, 0x29, 0x9d, 0x55, 0xaa, 0xfb, 0x60, 0x86, 0xb1, 0xbb, 0xcc, 0x3e, 0x5a,
    0xcb, 0x59, 0x5f, 0xb0, 0x9c, 0xa9, 0xa0, 0x51, 0x0b, 0xf5, 0x16, 0xeb, 0x7a, 0x75, 0x2c, 0xd7,
    0x4f, 0xae, 0xd5, 0xe9, 0xe6, 0xe7, 0xad, 0xe8, 0x74, 0xd6, 0xf4, 0xea, 0xa8, 0x50, 0x58, 0xaf
};

static uint8_t fec_gf_multiply(uint8_t a, uint8_t b)
{
    if (!a || !b)
        return 0;
    uint16_t exponent = (uint16_t)fec_gf_log[a] + fec_gf_log[b];
    if (exponent >= 255u)
        exponent -= 255u;
    return fec_gf_exp[exponent];
}

static uint8_t fec_gf_inverse(uint8_t value)
{
    /* Every nonzero GF(256) element has inverse value^254. */
    uint8_t result = 1;
    uint8_t exponent = 254;
    while (exponent) {
        if (exponent & 1u)
            result = fec_gf_multiply(result, value);
        value = fec_gf_multiply(value, value);
        exponent >>= 1;
    }
    return result;
}

static uint8_t time_erase_marker(void)
{
    time_reference_t timer;
    uint32_t elapsed;
    pogobot_stopwatch_reset(&timer);
    uint8_t erased = flash_invalidate_user_image();
    elapsed = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    upload_stats.erase_us += elapsed;
    if (elapsed > upload_stats.erase_max_us)
        upload_stats.erase_max_us = elapsed;
    upload_stats.erases++;
    return erased;
}

static void time_erase_sector(uint32_t addr)
{
    time_reference_t timer;
    uint32_t elapsed;
    pogobot_stopwatch_reset(&timer);
    erase_flash_sector(addr);
    elapsed = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    upload_stats.erase_us += elapsed;
    if (elapsed > upload_stats.erase_max_us)
        upload_stats.erase_max_us = elapsed;
    upload_stats.erases++;
}

static void time_write(uint32_t addr, unsigned char *data, uint32_t length)
{
    time_reference_t timer;
    uint32_t elapsed;
    pogobot_stopwatch_reset(&timer);
    write_to_flash(addr, data, length);
    elapsed = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    upload_stats.write_us += elapsed;
    if (elapsed > upload_stats.write_max_us)
        upload_stats.write_max_us = elapsed;
}

#ifndef IR_UPLOAD_FLASH_READ
// SPI flash is memory-mapped on the robot; tests can substitute a simulated NOR byte.
#define IR_UPLOAD_FLASH_READ(offset) \
    (*(volatile const uint8_t *)(uintptr_t)(SPIFLASH_BASE + (offset)))
#endif

static uint32_t v2_flash_crc32(uint32_t offset, uint32_t length)
{
    // Reflected CRC-32/ISO-HDLC, matching Python's zlib.crc32 over exact image bytes.
    uint32_t crc = 0xffffffffu;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= IR_UPLOAD_FLASH_READ(offset + i);
        for (uint8_t bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static uint8_t v2_command(uint8_t cmd)
{
    return cmd >= IR_V2_CMD_START && cmd <= IR_V2_CMD_PARITY;
}

static void v2_try_fec_recover(void)
{
    uint16_t first = v2_upload.fec_group * IR_V2_FEC_DATA_COUNT;
    uint8_t missing[IR_V2_FEC_PARITY_COUNT];
    uint8_t rows[IR_V2_FEC_PARITY_COUNT];
    uint8_t matrix[IR_V2_FEC_PARITY_COUNT][IR_V2_FEC_PARITY_COUNT] = {{0}};
    uint8_t residual[IR_V2_FEC_PARITY_COUNT][IR_V2_CHUNK_SIZE];
    uint8_t missing_count = 0, row_count = 0;
    time_reference_t timer;

    if (!v2_upload.fec_enabled || !v2_upload.fec_mask ||
        v2_upload.failed || v2_upload.complete || first >= v2_upload.chunks)
        return;

    for (uint8_t local = 0; local < IR_V2_FEC_DATA_COUNT &&
                            first + local < v2_upload.chunks; local++) {
        uint16_t index = first + local;
        if (!(v2_upload.bitmap[index >> 3] & (1u << (index & 7u)))) {
            if (missing_count == IR_V2_FEC_PARITY_COUNT)
                return;  // More than four erasures need a later pass.
            missing[missing_count++] = local;
        }
    }
    if (!missing_count)
        return;
    for (uint8_t row = 0; row < IR_V2_FEC_PARITY_COUNT; row++) {
        if (v2_upload.fec_mask & (1u << row))
            rows[row_count++] = row;
        if (row_count == missing_count)
            break;
    }
    if (row_count < missing_count)
        return;

    // Subtract accepted data read from flash; absent final-group symbols are
    // zero padding and never contribute to a parity equation.
    pogobot_stopwatch_reset(&timer);
    for (uint8_t equation = 0; equation < missing_count; equation++) {
        uint8_t row = rows[equation];
        memcpy(residual[equation], v2_upload.fec_parity[row], IR_V2_CHUNK_SIZE);
        for (uint8_t column = 0; column < missing_count; column++)
            matrix[equation][column] = fec_coefficients[row][missing[column]];
        for (uint8_t local = 0; local < IR_V2_FEC_DATA_COUNT &&
                                first + local < v2_upload.chunks; local++) {
            uint16_t index = first + local;
            if (!(v2_upload.bitmap[index >> 3] & (1u << (index & 7u))))
                continue;
            uint32_t relative = (uint32_t)index * IR_V2_CHUNK_SIZE;
            uint32_t available = v2_upload.length - relative;
            uint8_t coefficient = fec_coefficients[row][local];
            if (available > IR_V2_CHUNK_SIZE)
                available = IR_V2_CHUNK_SIZE;
            for (uint32_t byte = 0; byte < available; byte++) {
                uint8_t value = IR_UPLOAD_FLASH_READ(v2_upload.offset + relative + byte);
                residual[equation][byte] ^= fec_gf_multiply(coefficient, value);
            }
            // Drain hardware RX between symbols while this group is decoded.
            // Messages stay queued until the current frame handler returns.
            pogobot_infrared_update();
            rgb_blink();
        }
    }

    // Gauss-Jordan elimination solves at most four equations of 64 bytes.
    // The Cauchy matrix guarantees a pivot for every valid erasure pattern.
    for (uint8_t pivot = 0; pivot < missing_count; pivot++) {
        uint8_t selected = pivot;
        while (selected < missing_count && !matrix[selected][pivot])
            selected++;
        if (selected == missing_count)
            return;
        if (selected != pivot) {
            for (uint8_t column = 0; column < missing_count; column++) {
                uint8_t value = matrix[pivot][column];
                matrix[pivot][column] = matrix[selected][column];
                matrix[selected][column] = value;
            }
            for (uint8_t byte = 0; byte < IR_V2_CHUNK_SIZE; byte++) {
                uint8_t value = residual[pivot][byte];
                residual[pivot][byte] = residual[selected][byte];
                residual[selected][byte] = value;
            }
        }
        uint8_t inverse = fec_gf_inverse(matrix[pivot][pivot]);
        for (uint8_t column = pivot; column < missing_count; column++)
            matrix[pivot][column] = fec_gf_multiply(matrix[pivot][column], inverse);
        for (uint8_t byte = 0; byte < IR_V2_CHUNK_SIZE; byte++)
            residual[pivot][byte] = fec_gf_multiply(residual[pivot][byte], inverse);
        for (uint8_t row = 0; row < missing_count; row++) {
            if (row == pivot)
                continue;
            uint8_t factor = matrix[row][pivot];
            for (uint8_t column = pivot; column < missing_count; column++)
                matrix[row][column] ^= fec_gf_multiply(factor, matrix[pivot][column]);
            for (uint8_t byte = 0; byte < IR_V2_CHUNK_SIZE; byte++)
                residual[row][byte] ^= fec_gf_multiply(factor, residual[pivot][byte]);
            pogobot_infrared_update();
            rgb_blink();
        }
    }
    uint32_t decode_us = pogobot_stopwatch_get_elapsed_microseconds(&timer);
    upload_stats.fec_decode_us += decode_us;
    if (decode_us > upload_stats.fec_decode_max_us)
        upload_stats.fec_decode_max_us = decode_us;

    // Commit only solved data bytes. Flash readback and the final exact-image
    // CRC still guard validity, including a short last chunk.
    for (uint8_t slot = 0; slot < missing_count; slot++) {
        uint16_t index = first + missing[slot];
        uint32_t relative = (uint32_t)index * IR_V2_CHUNK_SIZE;
        uint32_t length = v2_upload.length - relative;
        uint32_t offset = v2_upload.offset + relative;
        if (length > IR_V2_CHUNK_SIZE)
            length = IR_V2_CHUNK_SIZE;
        time_write(offset, residual[slot], length);
        for (uint32_t byte = 0; byte < length; byte++) {
            if (IR_UPLOAD_FLASH_READ(offset + byte) != residual[slot][byte]) {
                v2_upload.failed = 1;
                printf("IR v2 FEC readback failed at %08lx; restart required\n",
                       (unsigned long)(offset + byte));
                return;
            }
        }
        v2_upload.bitmap[index >> 3] |= 1u << (index & 7u);
        v2_upload.received++;
        upload_stats.accepted++;
        upload_stats.fec_recovered++;
    }
    v2_upload.fec_mask = 0;
}

static uint8_t exec_v2_frame(struct sfl_frame *frame)
{
    const uint8_t *p = frame->payload;
    uint32_t id;

    if (frame->cmd == IR_V2_CMD_START) {
        uint32_t address, offset, length, expected_crc;
        if (frame->payload_length != IR_V2_START_LENGTH ||
            p[0] != IR_V2_VERSION ||
            (p[1] != 0 && p[1] != IR_V2_FLAG_FEC16X4) ||
            ir_v2_read_u16(p + 14) != IR_V2_CHUNK_SIZE) {
            upload_stats.malformed++;
            return 0;
        }
        id = ir_v2_read_u32(p + 2);
        address = ir_v2_read_u32(p + 6);
        length = ir_v2_read_u32(p + 10);
        expected_crc = ir_v2_read_u32(p + 16);
        if (address < SPIFLASH_BASE || length == 0 || length > IR_V2_SLOT_SIZE) {
            upload_stats.malformed++;
            return 0;
        }
        offset = address - SPIFLASH_BASE;
        // Only whole user slots are valid destinations; subtraction avoids wraparound.
        if ((offset != IR_FLASH_START && offset != 0x60000u) ||
            offset >= SPIFLASH_SIZE || length > SPIFLASH_SIZE - offset ||
            length > IR_V2_SLOT_SIZE) {
            upload_stats.malformed++;
            return 0;
        }
        if (v2_upload.active && !v2_upload.failed && v2_upload.id == id &&
            v2_upload.offset == offset && v2_upload.length == length &&
            v2_upload.expected_crc == expected_crc) {
            // Coding can change between attempts without changing image bytes.
            // Drop buffered parity but retain every verified DATA chunk.
            if (v2_upload.fec_enabled != (p[1] == IR_V2_FLAG_FEC16X4)) {
                v2_upload.fec_enabled = (p[1] == IR_V2_FLAG_FEC16X4);
                v2_upload.fec_mask = 0;
            }
            return 0;
        }

        memset(&v2_upload, 0, sizeof(v2_upload));
        if (!time_erase_marker()) {
            printf("IR v2 validity erase failed; image unchanged\n");
            return 0;
        }
        // Erase all image sectors before accepting DATA, including a lost first chunk.
        uint32_t first = offset / SPIFLASH_SECTOR_SIZE;
        uint32_t last = (offset + length - 1) / SPIFLASH_SECTOR_SIZE;
        for (uint32_t sector = first; sector <= last; sector++)
            time_erase_sector(sector * SPIFLASH_SECTOR_SIZE);
        v2_upload.id = id;
        v2_upload.offset = offset;
        v2_upload.length = length;
        v2_upload.expected_crc = expected_crc;
        v2_upload.chunks = (length + IR_V2_CHUNK_SIZE - 1) / IR_V2_CHUNK_SIZE;
        v2_upload.fec_enabled = (p[1] == IR_V2_FLAG_FEC16X4);
        v2_upload.active = 1;
        printf("IR v2 START: id=%08lx bytes=%lu chunks=%u fec=%u\n",
               (unsigned long)id, (unsigned long)length,
               (unsigned int)v2_upload.chunks, (unsigned int)v2_upload.fec_enabled);
        return 0;
    }

    if (frame->payload_length < IR_V2_ID_LENGTH) {
        upload_stats.malformed++;
        return 0;
    }
    id = ir_v2_read_u32(p);
    if (!v2_upload.active || id != v2_upload.id)
        return 0;  // Stale frames must not affect an active image.

    if (frame->cmd == IR_V2_CMD_DATA) {
        uint16_t index;
        uint32_t relative, length, offset;
        uint8_t mask;
        if (frame->payload_length < IR_V2_DATA_HEADER_LENGTH + 1 ||
            frame->payload_length > IR_V2_DATA_HEADER_LENGTH + IR_V2_CHUNK_SIZE) {
            upload_stats.malformed++;
            return 0;
        }
        index = ir_v2_read_u16(p + 4);
        if (index >= v2_upload.chunks) {
            upload_stats.malformed++;
            return 0;
        }
        relative = (uint32_t)index * IR_V2_CHUNK_SIZE;
        length = v2_upload.length - relative;
        if (length > IR_V2_CHUNK_SIZE)
            length = IR_V2_CHUNK_SIZE;
        if (frame->payload_length != IR_V2_DATA_HEADER_LENGTH + length) {
            upload_stats.malformed++;
            return 0;
        }
        mask = 1u << (index & 7u);
        if (v2_upload.bitmap[index >> 3] & mask) {
            upload_stats.duplicates++;
            return 0;
        }
        if (v2_upload.failed || v2_upload.complete)
            return 0;
        offset = v2_upload.offset + relative;
        // Leave the OK marker erased until END verifies the whole image. The
        // separate partial marker survives a timeout or power loss and lets
        // completion write OK without another marker-sector erase.
        if (v2_upload.received == 0) {
            time_write(FLASH_V2_PARTIAL_OFFSET,
                       (unsigned char *)FLASH_IS_PARTIAL, strlen(FLASH_IS_PARTIAL));
        }
        time_write(offset, &frame->payload[IR_V2_DATA_HEADER_LENGTH], length);
        // A failed NOR write cannot be repaired by programming more zero bits.
        for (uint32_t i = 0; i < length; i++) {
            if (IR_UPLOAD_FLASH_READ(offset + i) != p[IR_V2_DATA_HEADER_LENGTH + i]) {
                v2_upload.failed = 1;
                printf("IR v2 flash readback failed at %08lx; restart required\n",
                       (unsigned long)(offset + i));
                return 0;
            }
        }
        v2_upload.bitmap[index >> 3] |= mask;
        v2_upload.received++;
        upload_stats.accepted++;
        if (v2_upload.fec_mask &&
            v2_upload.fec_group == index / IR_V2_FEC_DATA_COUNT)
            v2_try_fec_recover();
        return 0;
    }

    if (frame->cmd == IR_V2_CMD_PARITY) {
        uint16_t group;
        uint8_t row;
        if (frame->payload_length != IR_V2_PARITY_LENGTH ||
            !v2_upload.fec_enabled) {
            upload_stats.malformed++;
            return 0;
        }
        group = ir_v2_read_u16(p + 4);
        row = p[6];
        if (group >= (v2_upload.chunks + IR_V2_FEC_DATA_COUNT - 1) /
                     IR_V2_FEC_DATA_COUNT || row >= IR_V2_FEC_PARITY_COUNT) {
            upload_stats.malformed++;
            return 0;
        }
        if (v2_upload.failed || v2_upload.complete)
            return 0;
        // One 256-byte parity group is enough for the ordered sender schedule.
        if (!v2_upload.fec_mask || v2_upload.fec_group != group) {
            v2_upload.fec_mask = 0;
            v2_upload.fec_group = group;
        }
        if (v2_upload.fec_mask & (1u << row)) {
            upload_stats.duplicates++;
            return 0;
        }
        memcpy(v2_upload.fec_parity[row], p + 7, IR_V2_CHUNK_SIZE);
        v2_upload.fec_mask |= 1u << row;
        v2_try_fec_recover();
        return 0;
    }

    if (frame->payload_length != IR_V2_ID_LENGTH) {
        upload_stats.malformed++;
        return 0;
    }
    if (frame->cmd == IR_V2_CMD_ABORT) {
        // Q closes the current remote broadcast; keep the invalid image and
        // its bitmap so another rc_flash_robot can send a repair copy.
        printf("IR v2 broadcast stopped; %u of %u chunks retained\n",
               (unsigned int)v2_upload.received, (unsigned int)v2_upload.chunks);
        return 1;
    }
    if (v2_upload.fec_mask)
        v2_try_fec_recover();
    if (v2_upload.complete) {
        if (check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET))
            return 1;  // Repeated END has no flash side effects.
        v2_upload.complete = 0;  // Retry a damaged or missing marker below.
    }
    if (v2_upload.failed) {
        printf("IR v2 transfer failed; the next START will erase and restart\n");
        return 1;
    }
    if (v2_upload.received != v2_upload.chunks) {
        printf("IR v2 incomplete: %u of %u chunks\n",
               (unsigned int)v2_upload.received, (unsigned int)v2_upload.chunks);
        return 0;
    }
    uint32_t actual_crc = v2_flash_crc32(v2_upload.offset, v2_upload.length);
    if (actual_crc != v2_upload.expected_crc) {
        v2_upload.failed = 1;
        printf("IR v2 image CRC mismatch: %08lx != %08lx; restart required\n",
               (unsigned long)actual_crc, (unsigned long)v2_upload.expected_crc);
        return 1;
    }
    // A failed marker write may have left zero bits that cannot be repaired
    // by programming again. Erase only its sector and retain the image bitmap.
    if (!check_flash_state(FLASH_ERASED_MARKER, FLASH_OK_OFFSET) &&
        !check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET)) {
        if (!time_erase_marker()) {
            printf("IR v2 marker erase failed; END can retry\n");
            return 0;
        }
        time_write(FLASH_V2_PARTIAL_OFFSET, (unsigned char *)FLASH_IS_PARTIAL,
                   strlen(FLASH_IS_PARTIAL));
    }
    time_write(FLASH_OK_OFFSET, (unsigned char *)FLASH_IS_OK, strlen(FLASH_IS_OK));
    if (!check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET)) {
        printf("IR v2 marker write failed; END can retry\n");
        return 0;
    }
    v2_upload.complete = 1;
    printf("IR v2 image verified and complete\n");
    return 1;
}

uint8_t check_crc(struct sfl_frame* frame) {
	/* Check Frame CRC */
    int computed_crc;
    int received_crc;
	received_crc = ((int)frame->crc[0] << 8)|(int)frame->crc[1];
	computed_crc = crc16(&frame->cmd, frame->payload_length+1);
	return(computed_crc == received_crc);
}

void print_frame(struct sfl_frame* frame) {
    int i;
    uint32_t addr;
    printf("size: 0x%02x\n", frame->payload_length);
    printf("crc: 0x%04x\n", ((int)frame->crc[0] << 8)|(int)frame->crc[1]);
    printf("cmd: 0x%02x\ndata:", frame->cmd);
    addr = frame->payload_length >= 4 ? get_uint32(&frame->payload[0]) : 0;
    for( i=4; i<frame->payload_length; i++ ) {
        if( (i-4)%16 == 0 ) {
            printf("\n%08lx ", addr+i-4);
        }
        else {
            if( (i-4)%8 == 0 ) {
                printf(" ");
            }
        }
        printf(" %02x", frame->payload[i]);
    }
    printf("\n");
}

static void ir_boot_loop_with_initial(const message_t *initial) {
    time_reference_t mytimer;
    // The remote spends about two seconds repeating ir_flash before the PC
    // handshake, so even the first START needs the full preparation timeout.
    uint32_t timeout = 8000000;
    struct sfl_frame * frame;
    message_t msg;
    slip_error_counter_s slip_start[IR_RX_COUNT];
    uint32_t ring_drop_start[IR_RX_COUNT];
    uint32_t queue_drop_start;
    uint32_t malformed_start;
    uint8_t stopped = 0;
    uint8_t initial_pending = initial != NULL;

    memset(&upload_stats, 0, sizeof(upload_stats));
    rgb_blink_set_time(5, 95);  // Flash 5 ms every 100 ms while receiving.
    rgb_blink_set_color(0, 0, 50);  // Active uploads always blink blue.

    if (v2_upload.active) {
        // A new remote command can resume the RAM bitmap after timeout or Q.
        if (v2_upload.received)
            printf("IR v2 resume: %u of %u chunks retained\n",
                   (unsigned int)v2_upload.received, (unsigned int)v2_upload.chunks);
        timeout = 8000000;
    }
    pogobot_timer_init(&mytimer, timeout);

    queue_drop_start = pogobot_infrared_get_queue_drop_count();
    malformed_start = pogobot_infrared_get_malformed_count();
    for (uint8_t i = 0; i < IR_RX_COUNT; i++) {
        pogobot_infrared_get_receiver_error_counter(&slip_start[i], i);
        ring_drop_start[i] = ir_uart_rx_drop_count(i);
    }

    while(!pogobot_timer_has_expired(&mytimer)) {
        pogobot_infrared_update();
        rgb_blink();
        /* Get one Frame */
        if(initial_pending || pogobot_infrared_message_available()) {
            // A START received in the Pogobios command loop must not be lost
            // when it hands control to the upload loop.
            if (initial_pending) {
                msg = *initial;
                initial_pending = 0;
            } else {
                pogobot_infrared_recover_next_message(&msg);
            }
            if (msg.header._packet_type != ir_t_flash)
                continue;

            pogobot_timer_init(&mytimer, timeout);
            upload_stats.frames++;
            if (msg.header.payload_length < 4 ||
                msg.header.payload_length > sizeof(msg.payload)) {
                upload_stats.malformed++;
                continue;
            }

            frame = (struct sfl_frame *)msg.payload;
            if (frame->payload_length != msg.header.payload_length - 4) {
                upload_stats.malformed++;
                continue;
            }
            if (!check_crc(frame)) {
                upload_stats.crc_errors++;
                continue;
            }
            if (v2_command(frame->cmd)) {
                time_reference_t process_timer;
                uint8_t finished;
                uint32_t process_us;
                timeout = 8000000;
                // Measure receiver work separately from the interframe wait.
                pogobot_stopwatch_reset(&process_timer);
                finished = exec_v2_frame(frame);
                process_us = pogobot_stopwatch_get_elapsed_microseconds(&process_timer);
                upload_stats.process_us += process_us;
                if (process_us > upload_stats.process_max_us)
                    upload_stats.process_max_us = process_us;
                if (finished) {
                    stopped = 1;
                    break;
                }
                // Start/erase may take time; wait for the host's preparation pause.
                pogobot_timer_init(&mytimer, timeout);
            } else {
                // Legacy LOAD/JUMP/ABORT has no image length or final CRC.
                // Reject it without modifying flash or the resumable v2 state.
                upload_stats.malformed++;
            }
        }
	}
    if (!stopped)
        printf("IR upload timed out\n");

    printf("IR upload: frames=%lu written=%lu duplicates=%lu malformed=%lu "
           "inner_crc=%lu gaps=%lu erases=%lu erase_us=%lu erase_max_us=%lu "
           "write_us=%lu write_max_us=%lu "
           "process_us=%lu process_max_us=%lu "
           "fec_recovered=%lu fec_decode_us=%lu fec_decode_max_us=%lu "
           "queue_drops=%lu outer_malformed=%lu\n",
           (unsigned long)upload_stats.frames, (unsigned long)upload_stats.accepted,
           (unsigned long)upload_stats.duplicates, (unsigned long)upload_stats.malformed,
           (unsigned long)upload_stats.crc_errors, (unsigned long)upload_stats.gaps,
           (unsigned long)upload_stats.erases, (unsigned long)upload_stats.erase_us,
           (unsigned long)upload_stats.erase_max_us,
           (unsigned long)upload_stats.write_us,
           (unsigned long)upload_stats.write_max_us,
           (unsigned long)upload_stats.process_us,
           (unsigned long)upload_stats.process_max_us,
           (unsigned long)upload_stats.fec_recovered,
           (unsigned long)upload_stats.fec_decode_us,
           (unsigned long)upload_stats.fec_decode_max_us,
           (unsigned long)(pogobot_infrared_get_queue_drop_count() - queue_drop_start),
           (unsigned long)(pogobot_infrared_get_malformed_count() - malformed_start));
    for (uint8_t i = 0; i < IR_RX_COUNT; i++) {
        slip_error_counter_s current;
        pogobot_infrared_get_receiver_error_counter(&current, i);
        printf("IR RX%u: slip_crc=%lu slip_overflow=%lu slip_escape=%lu ring_drops=%lu\n",
               (unsigned int)i,
               (unsigned long)(current.crc_mismatch_counter - slip_start[i].crc_mismatch_counter),
               (unsigned long)(current.overflow_counter - slip_start[i].overflow_counter),
               (unsigned long)(current.unknown_escaped_byte_counter -
                               slip_start[i].unknown_escaped_byte_counter),
               (unsigned long)(ir_uart_rx_drop_count(i) - ring_drop_start[i]));
    }

    rgb_blink_set_time(5, 995);
    update_led_status();
}

void ir_boot_loop(void)
{
    ir_boot_loop_with_initial(NULL);
}

void ir_boot_reset_v2_upload(void)
{
    // Called by erase_userprog before its marker/flash erase takes effect.
    memset(&v2_upload, 0, sizeof(v2_upload));
}

uint8_t ir_boot_try_v2_start(const message_t *message)
{
    // The normal Pogobios loop may miss the separate ir_flash command.
    // A complete START frame is an independent, CRC-protected entry point.
    if (message->header._packet_type != ir_t_flash ||
        message->header.payload_length != IR_V2_START_LENGTH + 4u)
        return 0;
    struct sfl_frame *frame = (struct sfl_frame *)message->payload;
    if (frame->cmd != IR_V2_CMD_START ||
        frame->payload_length != IR_V2_START_LENGTH || !check_crc(frame))
        return 0;
    ir_boot_loop_with_initial(message);
    return 1;
}
