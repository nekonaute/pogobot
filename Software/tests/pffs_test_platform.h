#ifndef PFFS_TEST_PLATFORM_H
#define PFFS_TEST_PLATFORM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define POGOBOT_USER_FLASH_START_OFFSET 0x90000u
#define POGOBOT_USER_FLASH_PAGE_SIZE 256u
#define POGOBOT_USER_FLASH_PAGE_COUNT 5888u
#define SPIFLASH_BASE 0x200000u
#define SPIFLASH_SIZE 0x200000u
#define SPIFLASH_SECTOR_SIZE 65536u
/* Host access hook for the private BIOS controller loader's readback. */
uint8_t pffs_test_flash_read(uint32_t);
#define POGOBIOS_FLASH_READ(offset) pffs_test_flash_read(offset)
typedef struct { uint8_t unused; } message_t;
struct sfl_frame;
void erase_flash_sector(unsigned int);
void write_to_flash(unsigned int, const unsigned char *, unsigned int);
typedef struct {
    uint32_t origin;
} time_reference_t;
void write_page_flash(uint16_t, const void *);
void read_page_flash(uint16_t, char *);
void erase_write_section_flash(void);
int spiBeginErase4(uint32_t);
uint16_t pogobot_helper_getid(void);
void pogobot_led_setColor(uint8_t, uint8_t, uint8_t);
void rgb_blink_set_color(uint8_t, uint8_t, uint8_t);
void reboot_ctrl_write(uint32_t);
void rgb_blink(void);
void pogobot_stopwatch_reset(time_reference_t *);
int32_t pogobot_stopwatch_get_elapsed_microseconds(time_reference_t *);
void pogobot_stopwatch_offset_origin_microseconds(time_reference_t *, int32_t);
int uart_read_nonblock(void);
#endif
