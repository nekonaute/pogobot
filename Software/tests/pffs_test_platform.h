#ifndef PFFS_TEST_PLATFORM_H
#define PFFS_TEST_PLATFORM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define POGOBOT_USER_FLASH_START_OFFSET 0x90000u
#define POGOBOT_USER_FLASH_PAGE_SIZE 256u
#define POGOBOT_USER_FLASH_PAGE_COUNT 5888u
typedef struct {
    uint32_t origin;
} time_reference_t;
void write_page_flash(uint16_t, const void *);
void read_page_flash(uint16_t, char *);
void erase_write_section_flash(void);
int spiBeginErase4(uint32_t);
uint16_t pogobot_helper_getid(void);
void pogobot_led_setColor(uint8_t, uint8_t, uint8_t);
void rgb_blink(void);
void pogobot_stopwatch_reset(time_reference_t *);
int32_t pogobot_stopwatch_get_elapsed_microseconds(time_reference_t *);
void pogobot_stopwatch_offset_origin_microseconds(time_reference_t *, int32_t);
int uart_read_nonblock(void);
#endif
