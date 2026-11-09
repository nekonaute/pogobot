/* Host serial-to-IR checks for the production remote upload command. */
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define REMOCON 1
#define CSR_UART_BASE 1
#define CONFIG_CLOCK_FREQUENCY 21000000u
#define SPIFLASH_BASE 0x200000u
#define SFL_MAGIC_REQ "sL5DdSMmkekro"
#define SFL_MAGIC_ACK "z6vGf7Rvp"
#define SFL_CMD_ABORT 0
#define SFL_CMD_LOAD 1
#define SFL_CMD_JUMP 2
#define SFL_ACK_SUCCESS 'K'
#define SFL_ACK_CRCERROR 'C'
#define SFL_ACK_ERROR 'E'
#define SFL_ACK_UNKNOWN 'U'
#define ir_t_cmd 1
#define ir_t_flash 2
#define ir_all 4

typedef struct { uint32_t deadline; } time_reference_t;
typedef struct {
    uint8_t _packet_type, _emitting_power_list;
    uint16_t _sender_id;
    uint8_t _sender_ir_index, _receiver_ir_index;
    uint16_t payload_length;
} message_header_t;
typedef struct { message_header_t header; uint8_t payload[382]; } message_t;
typedef struct { unsigned int unused; } slip_send_descriptor_s;
struct sfl_frame { uint8_t payload_length, crc[2], cmd, payload[255]; };

static uint8_t serial_input[1024], serial_output[256], forwarded[16];
static unsigned int input_count, input_read, output_count, forwarded_count;
static uint32_t clock_us;
slip_send_descriptor_s slip_send_descriptor;

static uint16_t crc16(const unsigned char *data, unsigned int length)
{
    uint16_t crc = 0;
    for (unsigned int i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (unsigned int bit = 0; bit < 8; bit++)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0));
    }
    return crc;
}

static int uart_read_nonblock(void) { return input_read < input_count; }
static char uart_read(void) { assert(input_read < input_count); return serial_input[input_read++]; }
static void uart_write(char value)
{ assert(output_count < sizeof(serial_output)); serial_output[output_count++] = value; }
static void pogobot_timer_init(time_reference_t *timer, uint32_t timeout)
{ timer->deadline = clock_us + timeout; }
static int pogobot_timer_has_expired(time_reference_t *timer)
{ return ++clock_us >= timer->deadline; }
static void msleep(int milliseconds) { clock_us += (uint32_t)milliseconds * 1000u; }
static void spiFree(void) {}
static int spiBeginErase4(uint32_t address)
{ (void)address; assert(!"remote test must not erase local flash"); return 0; }
static void rgb_blink_set_color(int r, int g, int b) { (void)r; (void)g; (void)b; }
static uint16_t pogobot_helper_getid(void) { return 0x1234; }
static uint8_t pogobot_infrared_emitting_power_list(int a, int b, int c, int d)
{ return a | (b << 2) | (c << 4) | (d << 6); }
static void slip_send_message(slip_send_descriptor_s *descriptor, uint8_t *data,
                              uint32_t length, uint16_t payload_length)
{
    const message_t *message = (const message_t *)data;
    (void)descriptor;
    assert(length == sizeof(message_header_t) + payload_length);
    if (message->header._packet_type == ir_t_flash) {
        const struct sfl_frame *frame = (const struct sfl_frame *)message->payload;
        assert(forwarded_count < sizeof(forwarded));
        forwarded[forwarded_count++] = frame->cmd;
    }
}

#include "../pogobios/boot.c"

const char *ir_magic_req = IR_MAGIC_REQ;

static void reset_serial(void)
{
    input_count = strlen(SFL_MAGIC_ACK);
    memcpy(serial_input, SFL_MAGIC_ACK, input_count);
    input_read = output_count = forwarded_count = clock_us = 0;
}

static void append_serial_frame(uint8_t command, unsigned int length)
{
    struct sfl_frame frame = {0};
    assert(length <= sizeof(frame.payload));
    assert(input_count + length + 4 <= sizeof(serial_input));
    frame.payload_length = length;
    frame.cmd = command;
    uint16_t crc = crc16(&frame.cmd, length + 1);
    frame.crc[0] = crc >> 8;
    frame.crc[1] = crc;
    memcpy(serial_input + input_count, &frame, length + 4);
    input_count += length + 4;
}

int main(void)
{
    // Empty legacy frames and maximum-size unknown frames must not consume
    // bytes from the next frame or overrun the serial frame buffer.
    reset_serial();
    append_serial_frame(SFL_CMD_ABORT, 0);
    append_serial_frame(0x7f, 255);
    append_serial_frame(SFL_CMD_JUMP, 3);
    append_serial_frame(SFL_CMD_JUMP, 4);
    assert(flash_robot() == 0 && forwarded_count == 0);
    assert(serial_output[output_count - 4] == SFL_ACK_UNKNOWN);
    assert(serial_output[output_count - 3] == SFL_ACK_UNKNOWN);
    assert(serial_output[output_count - 2] == SFL_ACK_ERROR);
    assert(serial_output[output_count - 1] == SFL_ACK_SUCCESS);

    reset_serial();
    append_serial_frame(SFL_CMD_LOAD, 68);
    append_serial_frame(SFL_CMD_JUMP, 4);
    assert(flash_robot() == 0);
    assert(forwarded_count == 0);
    assert(serial_output[output_count - 2] == SFL_ACK_UNKNOWN);
    assert(serial_output[output_count - 1] == SFL_ACK_SUCCESS);

    reset_serial();
    append_serial_frame(IR_V2_CMD_START, IR_V2_START_LENGTH);
    append_serial_frame(IR_V2_CMD_DATA, IR_V2_DATA_HEADER_LENGTH + 1);
    append_serial_frame(IR_V2_CMD_PARITY, IR_V2_PARITY_LENGTH);
    append_serial_frame(IR_V2_CMD_END, IR_V2_ID_LENGTH);
    append_serial_frame(SFL_CMD_JUMP, 4);
    assert(flash_robot() == 0);
    const uint8_t expected[] = {IR_V2_CMD_START, IR_V2_CMD_START, IR_V2_CMD_START,
                               IR_V2_CMD_DATA, IR_V2_CMD_PARITY, IR_V2_CMD_END};
    assert(forwarded_count == sizeof(expected));
    assert(memcmp(forwarded, expected, sizeof(expected)) == 0);
    for (unsigned int i = output_count - 5; i < output_count; i++)
        assert(serial_output[i] == SFL_ACK_SUCCESS);

    reset_serial();
    append_serial_frame(IR_V2_CMD_START, IR_V2_START_LENGTH);
    append_serial_frame(IR_V2_CMD_ABORT, IR_V2_ID_LENGTH);
    assert(flash_robot() == 1);
    assert(forwarded_count == 4 && forwarded[3] == IR_V2_CMD_ABORT);
    puts("IR remote forwarding: PASS");
    return 0;
}
