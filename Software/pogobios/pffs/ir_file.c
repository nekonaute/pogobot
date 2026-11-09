/* One-hop file sender shared by robot and remote BIOS commands. Receivers
 * publish into PFFS only after verification; no ACKs or implicit controller
 * installation are involved. The PC is not needed once a file is stored. */
#include "ir_file.h"
#include "ir_upload_v2.h"
#include "boot.h"
#include <pogobot.h>
#include <sfl.h>
#include <generated/mem.h>
#include <libbase/crc.h>
#include <uart.h>
#include <stdio.h>
#include <string.h>

/* Bound individual waits, service cancellation and drain reflected traffic.
 * Transmitting owns the console, so incoming messages are not dispatched. */
static bool pause_tx(unsigned milliseconds) {
    while (milliseconds != 0u) {
        if (uart_read_nonblock()) {
            int c = uart_read();
            if (c == 'Q' || c == 'q' || c == 27) return false;
        }
        pogobot_infrared_update();
        while (pogobot_infrared_message_available()) {
            message_t ignored;
            pogobot_infrared_recover_next_message(&ignored);
        }
        unsigned step = milliseconds > 20u ? 20u : milliseconds;
        msleep(step); milliseconds -= step;
    }
    return true;
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
static bool transmit(uint8_t command, const uint8_t *payload, uint8_t length) {
    message_t message;
    message.header._emitting_power_list =
        pogobot_infrared_emitting_power_list(3u, 3u, 3u, 3u);
    message.header._sender_ir_index = ir_all;
    message.header.payload_length = (uint16_t)length + 4u;
    struct sfl_frame *frame = (struct sfl_frame *)message.payload;
    frame->payload_length = length; frame->cmd = command;
    memcpy(frame->payload, payload, length);
    uint16_t crc = crc16(&frame->cmd, (unsigned)length + 1u);
    frame->crc[0] = (uint8_t)(crc >> 8); frame->crc[1] = (uint8_t)crc;
    /* The public user-packet sender initializes identity, actual emitting
     * powers/mask and respects mute. File commands have a distinct namespace. */
    return pogobot_infrared_sendRawLongMessage(&message) == 0;
}

bool ir_file_send(uint8_t id, unsigned passes, bool replace,
                  uint8_t scratch[POGO_FLASH_FILE_PAGE_SIZE]) {
    pogo_flash_file_info_t info;
    pogo_flash_file_status_t status =
        pogo_flash_file_read_page_secure(id, 0u, scratch, &info);
    if (status != POGO_FLASH_FILE_OK) {
        printf("IR file: %s\n", pogo_flash_file_status_string(status)); return false;
    }
    if (passes == 0u || passes > 255u || info.name_length == 0u) {
        puts("IR file: use a named ordinary file and 1..255 passes"); return false;
    }
    for (unsigned i = 0; i < info.name_length; ++i) {
        if ((uint8_t)info.name[i] < 33u || (uint8_t)info.name[i] > 126u) {
            puts("IR file: rename using printable ASCII without spaces"); return false;
        }
    }
    uint32_t source = POGOBOT_USER_FLASH_START_OFFSET + (uint32_t)info.first_page * 256u;
    uint8_t start[IR_FILE_START_LENGTH] = {1u, 1u};
    if (replace) start[1] |= IR_FILE_REPLACE;
    put32(start + 6, info.size_bytes);
    put32(start + 10, ir_flash_crc32(source, info.size_bytes));
    start[14] = (uint8_t)(info.format_version >> 8); start[15] = (uint8_t)info.format_version;
    start[16] = info.name_length; memcpy(start + 17, info.name, info.name_length);
    /* Stable FNV-1a identity includes contents' CRC, length, name, type and
     * replacement policy. Receivers additionally compare all START metadata. */
    uint32_t hash = UINT32_C(2166136261);
    for (unsigned i = 0; i < sizeof(start); ++i)
        hash = (hash ^ start[i]) * UINT32_C(16777619);
    if (hash == 0u) hash = 1u;
    put32(start + 2, hash);
    uint8_t payload[IR_V2_PARITY_LENGTH]; put32(payload, hash);
    uint32_t chunks = (info.size_bytes + 63u) / 64u;
    uint32_t sectors = ((uint32_t)info.page_count + 15u) / 16u;
    printf("IR file send: %s, %lu bytes, %u passes, 16+4, replace=%u; Q/ESC cancels\n",
           info.name, (unsigned long)info.size_bytes, passes, (unsigned)replace);
    for (unsigned pass = 0; pass < passes; ++pass) {
        /* Conservative size-dependent erase allowance. START keepalives also
         * prevent early-finishing receivers timing out during this interval. */
        for (uint32_t elapsed = 0; elapsed < sectors * 500u + 3000u; elapsed += 500u) {
            /* Only the first three STARTs may restart failed storage. Later
             * keepalives must not repeatedly erase a failing replacement. */
            if (elapsed < 1500u) start[1] |= IR_FILE_RESTART;
            else start[1] &= (uint8_t)~IR_FILE_RESTART;
            if (!transmit(IR_FILE_START, start, sizeof(start)) || !pause_tx(500u)) goto cancelled;
        }
        for (uint32_t stripe = 0; stripe < chunks; stripe += 64u) {
            /* Four interleaved groups, matching the PC firmware sender. */
            for (unsigned local = 0; local < 16u; ++local) {
                for (unsigned group = 0; group < 4u; ++group) {
                    uint32_t index = stripe + group * 16u + local;
                    if (index >= chunks) continue;
                    payload[4] = (uint8_t)(index >> 8); payload[5] = (uint8_t)index;
                    unsigned length = info.size_bytes - index * 64u;
                    if (length > 64u) length = 64u;
                    for (unsigned byte = 0; byte < length; ++byte)
                        payload[6u + byte] = POGOBIOS_FLASH_READ(source + index * 64u + byte);
                    if (!transmit(IR_FILE_DATA, payload, (uint8_t)(6u + length)) || !pause_tx(200u))
                        goto cancelled;
                }
            }
            for (unsigned group = 0; group < 4u && stripe + group * 16u < chunks; ++group) {
                uint32_t first = stripe + group * 16u;
                uint16_t number = (uint16_t)(first / 16u);
                payload[4] = (uint8_t)(number >> 8); payload[5] = (uint8_t)number;
                for (unsigned row = 0; row < 4u; ++row) {
                    payload[6] = (uint8_t)row; memset(payload + 7, 0, 64u);
                    for (unsigned local = 0; local < 16u && first + local < chunks; ++local) {
                        uint32_t relative = (first + local) * 64u;
                        unsigned length = info.size_bytes - relative;
                        if (length > 64u) length = 64u;
                        for (unsigned byte = 0; byte < length; ++byte)
                            payload[7u + byte] ^= fec_gf_multiply(fec_coefficients[row][local],
                                POGOBIOS_FLASH_READ(source + relative + byte));
                    }
                    if (!transmit(IR_FILE_PARITY, payload, sizeof(payload)) || !pause_tx(200u))
                        goto cancelled;
                }
            }
            printf("\rIR file pass %u/%u: %lu/%lu chunks", pass + 1u, passes,
                   (unsigned long)(stripe + 64u < chunks ? stripe + 64u : chunks),
                   (unsigned long)chunks);
        }
        for (unsigned copy = 0; copy < 3u; ++copy) {
            if (!transmit(IR_FILE_END, start + 2, 4u) || !pause_tx(500u)) goto cancelled;
        }
        putchar('\n');
    }
    puts("IR file broadcast finished; receiver completion is unconfirmed");
    return true;
cancelled:
    transmit(IR_FILE_ABORT, start + 2, 4u);
    puts("\nIR file broadcast stopped; receivers may retain partial state");
    return false;
}
