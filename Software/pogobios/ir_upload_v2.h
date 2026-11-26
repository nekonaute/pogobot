#ifndef POGOBOT_IR_UPLOAD_V2_H
#define POGOBOT_IR_UPLOAD_V2_H

#include <stdint.h>

/* Versioned SFL commands are distinct from the legacy LOAD/JUMP/ABORT set. */
#define IR_V2_CAPABILITY_BANNER "POGOBOT-IR-V2\n"
#define IR_V2_CMD_START 0x10u
#define IR_V2_CMD_DATA  0x11u
#define IR_V2_CMD_END   0x12u
#define IR_V2_CMD_ABORT 0x13u
#define IR_V2_CMD_PARITY 0x14u
#define IR_V2_VERSION   1u
#define IR_V2_FLAG_FEC16X4 0x01u
#define IR_V2_CHUNK_SIZE 64u
#define IR_V2_FEC_DATA_COUNT 16u
#define IR_V2_FEC_PARITY_COUNT 4u
#define IR_V2_START_LENGTH 20u
#define IR_V2_DATA_HEADER_LENGTH 6u
#define IR_V2_PARITY_LENGTH (7u + IR_V2_CHUNK_SIZE)
#define IR_V2_ID_LENGTH 4u
#define IR_V2_SLOT_SIZE 0x20000u
#define IR_V2_MAX_CHUNKS (IR_V2_SLOT_SIZE / IR_V2_CHUNK_SIZE)

/* All multibyte fields on the wire use network (big-endian) byte order. */
static inline uint16_t ir_v2_read_u16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static inline uint32_t ir_v2_read_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

#endif
