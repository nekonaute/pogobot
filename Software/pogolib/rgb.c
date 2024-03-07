/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include <pogobot.h>

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void color_wheel(uint8_t WheelPos) {
    WheelPos = 255 - WheelPos;
    uint8_t r, g, b;
    if(WheelPos < 85) {
        r = 255 - WheelPos * 3;
        g = 0;
        b = WheelPos * 3;
    }
    else if(WheelPos < 170) {
        WheelPos -= 85;
        r = 0;
        g = WheelPos * 3;
        b = 255 - WheelPos * 3;
    }
    else {
        WheelPos -= 170;
        r =WheelPos * 3;
        g = 255 - WheelPos * 3;
        b = 0;
    }
    // Dim the values
    rgb_set(r>>5, g>>5, b>>5);
}

/* ------- For old prototype or Lattice board using discrete RGB LED ---------*/
#ifdef CSR_RGB_CTRL_ADDR    // One passive RGB LED controlled by the FPGA
enum led_registers {
    LEDDCR0 = 8,
    LEDDBR = 9,
    LEDDONR = 10,
    LEDDOFR = 11,
    LEDDBCRR = 5,
    LEDDBCFR = 6,
    LEDDPWRR = 1,
    LEDDPWRG = 2,
    LEDDPWRB = 3,
};

#define BREATHE_ENABLE (1 << 7)
#define BREATHE_EDGE_ON (0 << 6)
#define BREATHE_EDGE_BOTH (1 << 6)
#define BREATHE_MODE_MODULATE (1 << 5)
#define BREATHE_MODE_FIXED (0 << 5)

// Breathe rate is in 128 ms increments
#define BREATHE_RATE_MS(x) ((((x)+1) / 128 & 7) << 0)

// Blink on/off time is in 32 ms increments
#define BLINK_TIME_MS(x) (((x)) / 32)

#define LEDDEN (1 << 7)
#define FR250 (1 << 6)
#define OUTPUL (1 << 5)
#define OUTSKEW (1 << 4)
#define QUICK_STOP (1 << 3)
#define PWM_MODE_LFSR (1 << 2)
#define PWM_MODE_LINEAR (0 << 2)

static void rgb_write(uint8_t value, uint8_t addr) {
    rgb_addr_write(addr);
    rgb_dat_write(value);
}

void rgb_init(void) {
    // Turn on the RGB block and current enable, as well as enabling led control
    rgb_ctrl_write(
        (1 << CSR_RGB_CTRL_EXE_OFFSET) |
        (1 << CSR_RGB_CTRL_CURREN_OFFSET) |
        (1 << CSR_RGB_CTRL_RGBLEDEN_OFFSET)
        );

    // Enable the LED driver, and set 250 Hz mode.
    // Also set quick stop, which we'll use to switch patterns quickly.
    rgb_write(LEDDEN | FR250 | QUICK_STOP, LEDDCR0);

    // Set clock register to 12 MHz / 64 kHz - 1
    rgb_write((12000000/64000)-1, LEDDBR);

    rgb_write(BLINK_TIME_MS(32), LEDDONR);  // Amount of time to stay "on"
    rgb_write(BLINK_TIME_MS(0), LEDDOFR);   // Amount of time to stay "off"

    rgb_write(BREATHE_ENABLE | BREATHE_MODE_FIXED | BREATHE_RATE_MS(128), LEDDBCRR);
    rgb_write(BREATHE_ENABLE | BREATHE_MODE_FIXED | BREATHE_RATE_MS(128), LEDDBCFR);
}

void rgb_set(uint8_t r, uint8_t g, uint8_t b) {
    // Note: the LEDD control registers have arbitrary names that
    // do not match up with the LEDD pin outputs.  Hence this strange
    // mapping.
    rgb_write(r, LEDDPWRR); // Blue
    rgb_write(g, LEDDPWRG); // Red
    rgb_write(b, LEDDPWRB); // Green
}

void rgb_set_u32(uint32_t word) {
    rgb_set(
        (word >> 16)&0xff,
        (word >> 8)&0xff,
        (word)&0xff
        );
}

// The amount of time to stay off or on
void rgb_on_time(uint8_t ms) {
    rgb_write(BLINK_TIME_MS(ms), LEDDONR);  // Amount of time to stay "on"
}

void rgb_off_time(uint8_t ms) {
    rgb_write(BLINK_TIME_MS(ms), LEDDOFR);   // Amount of time to stay "off"
}

// The amount of time to breathe in/out
void rgb_in_time(uint8_t ms) {
    rgb_write(BREATHE_ENABLE| BREATHE_MODE_FIXED | BREATHE_RATE_MS(ms), LEDDBCRR);
}

void rgb_out_time(uint8_t ms) {
    rgb_write(BREATHE_ENABLE | BREATHE_MODE_FIXED | BREATHE_RATE_MS(ms), LEDDBCFR);
}

void rgb_fixed(void)
{
    rgb_write(0, LEDDBCFR);
    rgb_write(0, LEDDBCRR);
}

void rgb_blink_set_time(uint32_t time_on, uint32_t time_off) {
    // Sets the time the LED will be on, in milliseconds
    rgb_on_time( time_on );
    rgb_off_time( time_off );
    rgb_in_time( 50 );
    rgb_out_time( 50 );
}

void rgb_blink_set_color(uint8_t r, uint8_t g, uint8_t b) {
    rgb_set(r, g, b);
}

void rgb_blink(void) {
}
#endif
/* ----------- For the new pogobot prototype using NeoPixel LEDs ------------ */
#ifdef CSR_RGB_FIFO_ADDR // n neopixel LEDs

static union u3224 {
    uint8_t b[3];
    uint32_t w;
} GRB[RGB_LEDS];

void rgb_cycle(void); // used only in init

void rgb_init(void) {
    rgb_cycle();
    // Clear the LEDS
    for( uint8_t i=0; i<RGB_LEDS; i++)
    {
        GRB[i].w=0;
        rgb_data_write(GRB[i].w); 
    }
#ifdef CSR_RGB_ENABLE_ADDR
    rgb_enable_write(1);
#endif
}

void rgb_set(uint8_t r, uint8_t g, uint8_t b) { // Set color of main LED only, kept for compatibility
    rgb_set_led(r,g,b,0);
}

static void rgb_send(void);
static void rgb_send(void) {
    // Send the LED values to the FIFO
#ifdef CSR_RGB_FIFOEMPTY_ADDR
    while(rgb_fifoempty() == 0);    // Wait until FIFO is empty
#else
    msleep(1);
#endif
    for(uint8_t i=0; i<RGB_LEDS; i++) {
        rgb_data_write(GRB[i].w); 
    }
#ifdef CSR_RGB_ENABLE_ADDR
    rgb_enable_write(1);
#endif
}

void rgb_set_led(uint8_t r, uint8_t g, uint8_t b, uint8_t id) {
    if ( id < RGB_LEDS )
    {
        // Fill the FIFO with one LED data
        GRB[id].b[0] = b;
        GRB[id].b[1] = r;
        GRB[id].b[2] = g;
    }
    rgb_send();
}

void rgb_set_hex(uint32_t val, uint8_t id) {
    // Set LED value by hex code 0xRRGGBB
    if ( id < RGB_LEDS ) {
		GRB[id].w = ((val&0xff0000)>>8) | ((val&0xff00)<<8) | (val&0xff);
	}
}

static time_reference_t rgb_timer;
static uint32_t rgb_time_on, rgb_time_off;
static bool rgb_on;
static uint8_t blink_r, blink_g, blink_b;

void rgb_blink_set_time(uint32_t time_on, uint32_t time_off) {
    // Sets the time the LED will be on, in milliseconds
    pogobot_stopwatch_reset(&rgb_timer);
    rgb_time_on = time_on*1000;
    rgb_time_off = time_off*1000;
    rgb_on = false;
}

void rgb_blink_set_color(uint8_t r, uint8_t g, uint8_t b) {
    blink_r = r;
    blink_g = g;
    blink_b = b;
}

void rgb_blink(void) {
    // No interrupt, just call this in a loop
    // blinks first LED only
    if(rgb_on) {
        if(pogobot_stopwatch_get_elapsed_microseconds(&rgb_timer) > rgb_time_off+rgb_time_on) {
            rgb_set(0, 0, 0);
            rgb_on = false;
            pogobot_stopwatch_reset(&rgb_timer);
        }
    }
    else {
        if(pogobot_stopwatch_get_elapsed_microseconds(&rgb_timer) > rgb_time_off) {
            rgb_set(blink_r, blink_g, blink_b);
            rgb_on = true;
        }
    }
}

void rgb_cycle(void) {
    uint32_t color=0x20;
    for(uint8_t t=0; t<3; t++) {
        for(uint8_t i=1; i<5; i++) {        // Cycle through the 4 belly LEDs only
            rgb_set_hex(color,0);           // Set the color on head LED too
            for(uint8_t j=1; j<5; j++) {    // Treat all the 5 LEDs
                rgb_set_hex((i==j)?color:0,j);
            }
            rgb_send();
            msleep(80);
        }
		color<<=8;	// display blue, green, red
    }
}
#endif //RGB_LEDS

