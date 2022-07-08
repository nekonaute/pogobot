/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#ifndef _RGB_H_
#define _RGB_H_
#include <stdint.h>
#include <generated/soc.h>

#define EMPTY_STATE     0x004000        // Green
#define FLASHED_STATE   0x000040        // Blue


void rgb_init(void);
void rgb_set(uint8_t r, uint8_t g, uint8_t b);
void rgb_set_u32(uint32_t word);
void color_wheel(uint8_t WheelPos);

#ifndef RGB_LEDS /* Old prototype or Lattice Breakout board */
// The amount of time to stay off or on
void rgb_on_time(uint8_t ms);
void rgb_off_time(uint8_t ms);

// The amount of time to breathe in/out
void rgb_in_time(uint8_t ms);
void rgb_out_time(uint8_t ms);

// No breathing please
void rgb_fixed(void);

#else /* New prototype */
void rgb_set_led(uint8_t r, uint8_t g, uint8_t b, uint8_t id);
void rgb_set_hex(uint32_t val, uint8_t id);
void rgb_blink_set_time(uint32_t time_on, uint32_t time_off);
void rgb_blink_set_color(uint8_t r, uint8_t g, uint8_t b);
void rgb_blink(void);

#endif

#endif /* _RGB_H_ */
