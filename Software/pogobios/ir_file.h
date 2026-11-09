/* Private BIOS one-way PFFS broadcasts. Distinct user-packet commands keep
 * file reception separate from firmware-slot programming and its markers. */
#ifndef POGOBIOS_IR_FILE_H
#define POGOBIOS_IR_FILE_H
#include <stdbool.h>
#include <stdint.h>
#include <pffs/flash_file.h>
#define IR_FILE_START  0x15u
#define IR_FILE_DATA   0x16u
#define IR_FILE_END    0x17u
#define IR_FILE_ABORT  0x18u
#define IR_FILE_PARITY 0x19u
#define IR_FILE_START_LENGTH 49u
#define IR_FILE_REPLACE 2u
#define IR_FILE_RESTART 4u /* Fresh attempt, not an erase-preparation keepalive. */
#define IR_FILE_MAX_CHUNKS ((POGO_FLASH_FILE_MAX_BYTES + 63u) / 64u)

/* One reception reserves its extent, including between broadcast attempts.
 * Only a matching sender/START may resume it; local abort releases the lock. */
bool ir_file_busy(void);
void ir_file_abort(void);
bool ir_file_send(uint8_t id, unsigned passes, bool replace,
                  uint8_t scratch[POGO_FLASH_FILE_PAGE_SIZE]);

/* Encoder and firmware decoder share the existing GF tables/math. */
extern const uint8_t fec_coefficients[4][16];
uint8_t fec_gf_multiply(uint8_t a, uint8_t b);
uint32_t ir_flash_crc32(uint32_t offset, uint32_t length);
#endif
