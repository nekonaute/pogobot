/* Exercise production library code against byte FIFOs, SPI pins, and a timer. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Supply the public data types while replacing target-only CSR dependencies. */
#define __POGOBOT_H__
#define CSR_IR_RX0_BASE 1
#define IR_RX_COUNT 4
#define CONFIG_CLOCK_FREQUENCY 21000000u
#define SPIFLASH_BASE 0x200000u
#define MAX_PAYLOAD_SIZE_BYTES 382
#define MAX_NUMBER_OF_WORD 20
#define NUMEL 20
#define pogobot_infrared_emitter_width_bits 2
#define pogobot_infrared_emitter_power_oneThird 1
#define pogobot_infrared_emitter_power_max 3
#define pogobot_infrared_emitter_power_null 0
typedef enum { ir_front, ir_right, ir_back, ir_left, ir_all } ir_direction;
enum { ir_t_cmd = 1, ir_t_flash = 2, ir_t_short = 3, ir_t_user = 16 };
typedef struct {
    uint8_t _packet_type, _emitting_power_list;
    uint16_t _sender_id;
    uint8_t _sender_ir_index, _receiver_ir_index;
    uint16_t payload_length;
} message_header_t;
typedef struct { message_header_t header; uint8_t payload[382]; } message_t;
typedef struct { uint8_t _packet_type; uint16_t payload_length; } message_short_header_t;
typedef struct { message_short_header_t header; uint8_t payload[382]; } short_message_t;
typedef struct { uint32_t hardware_value_at_time_origin; } time_reference_t;
#include "../pogolib/FifoBuffer.h"
#include "../pogolib/slip.h"
#include "../pogolib/spi.h"
FifoBuffer_typedef(message_t, Messagefifo);

static unsigned int tx_level, tx_count, tx_drops, tx_status_reads;
static uint8_t tx_bytes[32768];
static uint32_t mock_ticks;
static unsigned int spi_previous = 1u << PIN_CS, spi_bits, spi_count;
static uint8_t spi_byte, spi_frame[512], spi_program[512];
static unsigned int spi_program_count, spi_transactions;

static uint16_t pogobot_helper_getid(void) { return 0x1234; }
static uint8_t pogobot_infrared_emitting_power_list(int a, int b, int c, int d)
{ return a | (b << 2) | (c << 4) | (d << 6); }
static uint8_t ir_init(void) { return 0; }
static void IRn_conf_tx_power_write(int channel, int power) { (void)channel; (void)power; }
static void ir_tx_conf_tx_mask_write(unsigned int value) { (void)value; }
static void ir_tx_conf_tx_trig_write(unsigned int value) { (void)value; }
static unsigned int ir_tx_txfull_read(void)
{
    tx_status_reads++;
    if (tx_level == 512) {
        tx_level--;  // Model the transmitter making space while the CPU polls.
        return 1;
    }
    return 0;
}
static void ir_tx_tx_write(unsigned int value)
{
    if (tx_level == 512) { tx_drops++; return; }
    assert(tx_count < sizeof(tx_bytes));
    tx_bytes[tx_count++] = value;
    tx_level++;
}
static int ir_uart_read_nonblock(uint8_t channel) { (void)channel; return 0; }
static char ir_uart_read(uint8_t channel) { (void)channel; return 0; }
static void reboot_ctrl_write(unsigned int value) { (void)value; }
static void timer0_en_write(unsigned int value) { (void)value; }
static void timer0_load_write(unsigned int value) { (void)value; }
static void timer0_reload_write(unsigned int value) { (void)value; }
static void timer0_update_value_write(unsigned int value) { (void)value; }
static uint32_t timer0_value_read(void) { return mock_ticks; }
static void spiflash_bitbang_en_write(unsigned int value) { (void)value; }
static unsigned int spiflash_miso_read(void) { return 0; }
static void spiflash_bitbang_write(unsigned int value)
{
    unsigned int cs = 1u << PIN_CS, clk = 1u << PIN_CLK;
    if ((spi_previous & cs) && !(value & cs))
        spi_bits = spi_count = spi_byte = 0;
    if (!(value & cs) && !(spi_previous & clk) && (value & clk)) {
        spi_byte = (spi_byte << 1) | ((value >> PIN_MOSI) & 1u);
        if (++spi_bits == 8) {
            assert(spi_count < sizeof(spi_frame));
            spi_frame[spi_count++] = spi_byte;
            spi_bits = spi_byte = 0;
        }
    }
    if (!(spi_previous & cs) && (value & cs)) {
        spi_transactions++;
        if (spi_count && spi_frame[0] == 0x42) {
            memcpy(spi_program, spi_frame, spi_count);
            spi_program_count = spi_count;
        }
    }
    spi_previous = value;
}

#include "../pogolib/slip.c"
#include "../pogolib/pogolib_infrared.c"
#include "../pogolib/spi.c"
#include "../pogolib/pogobot_timer_sleep_stopwatch.c"

static void test_send_bounds_and_fifo_backpressure(void)
{
    tx_level = 511;
    for (unsigned int i = 1; i <= 4; i++)
        assert(write_byte_via_a_four_byte_word_channel(i) == 1);
    assert(tx_count == 4 && tx_drops == 0 && tx_status_reads > 4);
    for (unsigned int i = 0; i < 4; i++) assert(tx_bytes[i] == i + 1);

    uint8_t payload[382];
    memset(payload, 0xdb, sizeof(payload));  // Exercise worst-case SLIP escaping.
    for (unsigned int length = 383; length <= UINT16_MAX; length += UINT16_MAX - 383) {
        unsigned int before = tx_count;
        assert(pogobot_infrared_sendLongMessage_uniSpe(ir_front, NULL, length) == 1);
        assert(pogobot_infrared_sendLongMessage_omniGen(NULL, length) == 1);
        assert(pogobot_infrared_sendLongMessage_omniSpe(NULL, length) == 1);
        assert(pogobot_infrared_sendShortMessage_uni(ir_front, NULL, length) == 1);
        assert(pogobot_infrared_sendShortMessage_omni(NULL, length) == 1);
        assert(tx_count == before);
    }
    assert(pogobot_infrared_sendLongMessage_uniSpe(ir_front, payload, sizeof(payload)) == 0);
    assert(pogobot_infrared_sendLongMessage_omniGen(payload, sizeof(payload)) == 0);
    assert(pogobot_infrared_sendLongMessage_omniSpe(payload, sizeof(payload)) == 0);
    assert(pogobot_infrared_sendShortMessage_uni(ir_front, payload, sizeof(payload)) == 0);
    assert(pogobot_infrared_sendShortMessage_omni(payload, sizeof(payload)) == 0);
    assert(pogobot_infrared_sendLongMessage_omniGen(payload, 0) == 0);
    assert(tx_drops == 0);
}

static void test_security_register_bounds_and_full_write(void)
{
    uint8_t data[256], readback[256];
    for (unsigned int i = 0; i < sizeof(data); i++) data[i] = i;
    assert(spiWriteSecurityRegister(1, 0, data, sizeof(data)) == 0);
    assert(spi_program_count == 260 && spi_program[1] == 0 && spi_program[2] == 0x10);
    assert(memcmp(spi_program + 4, data, sizeof(data)) == 0);
    assert(spiReadSecurityRegister(1, 0, sizeof(readback), readback) == 0);
    for (unsigned int i = 0; i < sizeof(readback); i++) assert(readback[i] == 0);
    assert(spiWriteSecurityRegister(3, 255, data, 1) == 0);
    unsigned int before = spi_transactions;
    assert(spiWriteSecurityRegister(0, 0, data, 1) == -1);
    assert(spiWriteSecurityRegister(4, 0, data, 1) == -1);
    assert(spiWriteSecurityRegister(1, 255, data, 2) == -1);
    assert(spiWriteSecurityRegister(1, 1, data, UINT32_MAX) == -1);
    assert(spiReadSecurityRegister(0, 0, 1, readback) == -1);
    assert(spiReadSecurityRegister(1, 255, 2, readback) == -1);
    assert(spiReadSecurityRegister(1, 1, UINT32_MAX, readback) == -1);
    spiEraseSecurityRegister(0);
    assert(spi_transactions == before);
}

static void test_signed_timers_and_counter_wrap(void)
{
    time_reference_t timer = {1000};
    mock_ticks = 979;
    assert(pogobot_timer_get_remaining_microseconds(&timer) == -1);
    assert(pogobot_timer_has_expired(&timer));
    mock_ticks = 1021;
    assert(pogobot_stopwatch_get_elapsed_microseconds(&timer) == -1);
    assert(pogobot_stopwatch_lap(&timer) == -1);
    assert(timer.hardware_value_at_time_origin == mock_ticks);
    timer.hardware_value_at_time_origin = 1000000;
    pogobot_timer_offset_origin_microseconds(&timer, -1000);
    assert(timer.hardware_value_at_time_origin == 1021000);
    pogobot_stopwatch_offset_origin_microseconds(&timer, 1000);
    assert(timer.hardware_value_at_time_origin == 1000000);
    timer.hardware_value_at_time_origin = 10;
    mock_ticks = UINT32_MAX - 10;
    assert(pogobot_stopwatch_get_elapsed_microseconds(&timer) == 1);
    assert(pogobot_timer_get_remaining_microseconds(&timer) == -1);
    timer.hardware_value_at_time_origin = UINT32_MAX - 10;
    mock_ticks = 10;
    assert(pogobot_timer_get_remaining_microseconds(&timer) == 1);
    assert(!pogobot_timer_has_expired(&timer));
    assert(ticks_to_microseconds(INT32_MIN) == -102261126);
    assert(ticks_to_microseconds(INT32_MAX) == 102261126);
}

int main(void)
{
    test_send_bounds_and_fifo_backpressure();
    test_security_register_bounds_and_full_write();
    test_signed_timers_and_counter_wrap();
    puts("Library API fault injection: PASS");
    return 0;
}
