/**
 * Check the exact firmware image in memory-mapped SPI flash after boot.
 * The four-byte length, generated payload, and four-byte CRC trailer are
 * appended by seal_firmware.py and are part of the uploaded image.
 */

#include "pogobot.h"

#include <stdint.h>
#include <stdio.h>

extern const uint8_t _ftext[];
extern const uint8_t _edata_rom[];

/* The v3 application ROM window is 0x18000 bytes; keep the read inside it. */
#define MAX_IMAGE_BYTES 0x18000u

static uint32_t read_le32(const volatile uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static uint32_t image_crc32(const volatile uint8_t *bytes, uint32_t length)
{
    /* Reflected CRC-32/ISO-HDLC, compatible with Python's zlib.crc32. */
    uint32_t crc = 0xffffffffu;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    }
    return crc ^ 0xffffffffu;
}

int main(void)
{
    pogobot_init();

    uintptr_t start = (uintptr_t)_ftext;
    uintptr_t linked_end = (uintptr_t)_edata_rom;
    uint32_t linked_bytes;
    uint32_t payload_bytes;
    uint32_t checked_bytes;
    uint32_t expected;
    uint32_t actual;

    /* A missing or damaged length must never make the robot scan outside ROM. */
    if (linked_end < start || linked_end - start > MAX_IMAGE_BYTES - 8u) {
        printf("FIRMWARE INTEGRITY FAIL: invalid linked image bounds\n");
        goto failed;
    }
    linked_bytes = (uint32_t)(linked_end - start);
    payload_bytes = read_le32((const volatile uint8_t *)linked_end);
    if (payload_bytes > MAX_IMAGE_BYTES - linked_bytes - 8u) {
        printf("FIRMWARE INTEGRITY FAIL: missing or invalid seal\n");
        goto failed;
    }

    checked_bytes = linked_bytes + 4u + payload_bytes;
    expected = read_le32((const volatile uint8_t *)(start + checked_bytes));
    actual = image_crc32((const volatile uint8_t *)start, checked_bytes);
    if (actual != expected) {
        printf("FIRMWARE INTEGRITY FAIL: %lu bytes, expected %08lx, got %08lx\n",
               (unsigned long)(checked_bytes + 4u),
               (unsigned long)expected, (unsigned long)actual);
        goto failed;
    }

    printf("FIRMWARE INTEGRITY PASS: %lu bytes, CRC32 %08lx\n",
           (unsigned long)(checked_bytes + 4u), (unsigned long)actual);
    pogobot_led_setColor(0, 255, 0);
    for (;;)
        msleep(1000);

failed:
    pogobot_led_setColor(255, 0, 0);
    for (;;)
        msleep(1000);
}
