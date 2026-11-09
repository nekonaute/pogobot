/* Reuse the production firmware receiver's strict NOR/radio device double.
 * Then exercise the actual onboard sender and PFFS sink against each other. */
#define main firmware_fixture_main
#include "ir_upload_fault_test.c"
#undef main

static message_t file_frames[30000];
static unsigned file_count;
static uint8_t file_content[POGO_FLASH_FILE_MAX_BYTES];
static bool radio_error;
#define ir_all 4
static uint8_t pogobot_infrared_emitting_power_list(unsigned a, unsigned b,
                                                   unsigned c, unsigned d) {
    return (uint8_t)(a | b << 2 | c << 4 | d << 6);
}

static void msleep(unsigned milliseconds) { clock_us += milliseconds * 1000u; }
uint32_t pogobot_infrared_sendRawLongMessage(message_t *data) {
    if (radio_error) return 1u;
    CHECK(file_count < sizeof(file_frames) / sizeof(file_frames[0]));
    message_t *m = &file_frames[file_count++];
    memset(m, 0, sizeof(*m));
    m->header._packet_type = ir_t_user;
    m->header._sender_id = device_id;
    CHECK(data->header._emitting_power_list == 0xffu &&
          data->header._sender_ir_index == ir_all);
    m->header.payload_length = data->header.payload_length;
    memcpy(m->payload, data->payload, data->header.payload_length);
    return 0;
}
#include "../pogobios/pffs/ir_file.c"

static void capture_passes(unsigned length, bool replace, unsigned passes) {
    reset_device(); device_id = 42u; file_count = 0u;
    CHECK(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
    CHECK(pogo_flash_file_create_bytes(20u, "payload", length, 1u, file_content) ==
          POGO_FLASH_FILE_OK);
    uint8_t scratch[256];
    CHECK(ir_file_send(20u, passes, replace, scratch));
    CHECK(file_count > 3u);
}
static void capture(unsigned length, bool replace) { capture_passes(length, replace, 1u); }

static void receiver(void) {
    reset_device(); device_id = 1u;
    /* Every file operation must preserve bootloader, gateware, controller,
     * CRC area and boot-validity markers byte for byte. */
    memset(flash_bytes, 0xa5, 0x90000u);
    CHECK(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
}
static void guard(void) {
    for (unsigned i = 0u; i < 0x90000u; ++i) CHECK(flash_bytes[i] == 0xa5u);
}
static uint8_t apply(message_t *m) {
    struct sfl_frame *f = (struct sfl_frame *)m->payload;
    CHECK(m->header.payload_length == f->payload_length + 4u && check_crc(f));
    rx_sender = m->header._sender_id;
    return exec_v2_frame(f);
}
static void replay(unsigned lost_mask, bool ends) {
    for (unsigned i = 0u; i < file_count; ++i) {
        struct sfl_frame *f = (struct sfl_frame *)file_frames[i].payload;
        if (f->cmd == IR_FILE_DATA) {
            unsigned index = ir_v2_read_u16(f->payload + 4);
            if (index < 16u && (lost_mask & (1u << index))) continue;
        }
        if (!ends && f->cmd == IR_FILE_END) continue;
        apply(&file_frames[i]);
    }
}
static void verified(unsigned length, unsigned expected_id) {
    pogo_flash_file_info_t info;
    uint8_t page[256];
    CHECK(!ir_file_busy());
    CHECK(pogo_flash_file_find_by_name("payload", &info) == POGO_FLASH_FILE_OK);
    CHECK(info.id == expected_id && info.size_bytes == length);
    CHECK(pogo_flash_file_read_page_secure(info.id, 0, page, NULL) == POGO_FLASH_FILE_OK);
    CHECK(memcmp(flash_bytes + 0x90000u + (uint32_t)info.first_page * 256u,
                 file_content, length) == 0);
    guard();
}

static void test_file_roundtrip_sizes_and_fec(void) {
    const unsigned lengths[] = {0u, 1u, 63u, 64u, 65u, 255u, 256u, 257u,
                                17u * 64u + 13u, 65537u, POGO_FLASH_FILE_MAX_BYTES};
    for (unsigned n = 0; n < sizeof(lengths) / sizeof(lengths[0]); ++n) {
        capture(lengths[n], false); receiver();
        replay(lengths[n] == 17u * 64u + 13u ? 0x8211u : 0u, true);
        verified(lengths[n], 1u);
        if (lengths[n] == 17u * 64u + 13u) CHECK(upload_stats.fec_recovered == 4u);
    }
}

static void test_file_resume_and_source_binding(void) {
    capture(1101u, false); receiver(); replay(0x1fu, true);
    CHECK(ir_file_busy() && v2_upload.received == v2_upload.chunks - 5u);
    pogo_flash_file_info_t info;
    CHECK(pogo_flash_file_find_by_name("payload", &info) == POGO_FLASH_FILE_NOT_FOUND);
    unsigned received = v2_upload.received;
    unsigned erased = all_erase4;
    apply(&file_frames[0]);
    CHECK(all_erase4 == erased && v2_upload.received == received);
    message_t wrong = file_frames[0]; wrong.header._sender_id = 77u;
    CHECK(!ir_boot_try_v2_start(&wrong));
    apply(&wrong); CHECK(v2_upload.received == received);
    CHECK(ir_file_busy());
    for (unsigned i = 0; i < file_count; ++i) {
        struct sfl_frame *f = (struct sfl_frame *)file_frames[i].payload;
        if (f->cmd == IR_FILE_DATA && ir_v2_read_u16(f->payload + 4) < 5u)
            apply(&file_frames[i]);
    }
    apply(&file_frames[file_count - 1u]); verified(1101u, 1u);
    /* Repeated START/DATA/END after success must not erase or rewrite data. */
    uint8_t snapshot[4096]; memcpy(snapshot, flash_bytes + 0xa0000u, sizeof(snapshot));
    unsigned programmed = all_programs; erased = all_erase4;
    replay(0u, true); verified(1101u, 1u);
    CHECK(all_programs == programmed && all_erase4 == erased);
    CHECK(memcmp(snapshot, flash_bytes + 0xa0000u, sizeof(snapshot)) == 0);
}

static void test_file_unpublished_abort_and_reboot(void) {
    capture(257u, false); receiver(); replay(0u, false);
    CHECK(ir_file_busy());
    pogo_flash_file_info_t info;
    CHECK(pogo_flash_file_find_by_name("payload", &info) == POGO_FLASH_FILE_NOT_FOUND);
    ir_file_abort(); CHECK(!ir_file_busy());
    replay(0u, false); CHECK(ir_file_busy());
    ir_boot_reset_v2_upload(); CHECK(!ir_file_busy());
    CHECK(pogo_flash_file_find_by_name("payload", &info) == POGO_FLASH_FILE_NOT_FOUND);
    replay(0u, true); verified(257u, 1u);
}

static void existing(unsigned length) {
    static uint8_t old[1280]; memset(old, 0x55, sizeof(old));
    CHECK(pogo_flash_file_create_bytes(7u, "payload", length, 1u, old) == POGO_FLASH_FILE_OK);
}
static void test_file_replacement_and_conflicts(void) {
    capture(1101u, false); receiver(); existing(1200u);
    replay(0u, true);
    uint8_t page[256]; pogo_flash_file_info_t info;
    CHECK(!ir_file_busy());
    CHECK(pogo_flash_file_read_page_secure(7u, 0u, page, &info) == POGO_FLASH_FILE_OK);
    CHECK(info.size_bytes == 1200u && page[0] == 0x55u); guard();
    capture(1101u, true); receiver(); existing(1200u);
    replay(0u, true); verified(1101u, 7u);
    CHECK(pogo_flash_file_find(7u, &info) == POGO_FLASH_FILE_OK && info.generation == 2u);
    /* --replace may create a missing name, but cannot resize an existing extent. */
    receiver(); replay(0u, true); verified(1101u, 1u);
    receiver(); existing(256u); replay(0u, true);
    CHECK(!ir_file_busy());
    CHECK(pogo_flash_file_read_page_secure(7u, 0u, page, &info) == POGO_FLASH_FILE_OK);
    CHECK(info.size_bytes == 256u && page[0] == 0x55u); guard();
    receiver(); existing(1200u); replay(0x1fu, false);
    CHECK(ir_file_busy()); ir_file_abort();
    CHECK(pogo_flash_file_read_page_secure(7u, 0u, page, NULL) == POGO_FLASH_FILE_BAD_CHECKSUM);
    guard();
}

static void test_file_erase_write_and_crc_failures(void) {
    capture(1101u, false); receiver();
    failed_erase_address = 0xa0000u; replay(0u, true);
    CHECK(ir_file_busy() && v2_upload.failed); guard();
    unsigned erased = all_erase4;
    for (unsigned i = 0; i < file_count; ++i) {
        struct sfl_frame *f = (struct sfl_frame *)file_frames[i].payload;
        if (f->cmd == IR_FILE_START && !(f->payload[1] & IR_FILE_RESTART))
            apply(&file_frames[i]);
    }
    CHECK(all_erase4 == erased && ir_file_busy());
    failed_erase_address = UINT32_MAX; replay(0u, true); verified(1101u, 1u);
    receiver(); apply(&file_frames[0]);
    failed_program_address = v2_upload.offset; replay(0u, true);
    CHECK(ir_file_busy() && v2_upload.failed); guard();
    failed_program_address = UINT32_MAX; replay(0u, true); verified(1101u, 1u);
    receiver(); replay(0u, false);
    flash_bytes[v2_upload.offset + 17u] ^= 1u;
    apply(&file_frames[file_count - 1u]);
    CHECK(ir_file_busy() && v2_upload.failed);
    pogo_flash_file_info_t info;
    CHECK(pogo_flash_file_find_by_name("payload", &info) == POGO_FLASH_FILE_NOT_FOUND);
    ir_file_abort(); guard();
    /* A failed publication is reported and locked, never silently formatted. */
    receiver(); replay(0u, false);
    failed_program_address = 0x90000u + 12u;
    apply(&file_frames[file_count - 1u]);
    CHECK(ir_file_busy() && v2_upload.failed);
    CHECK(pogo_flash_file_check() != POGO_FLASH_FILE_OK);
    guard(); ir_file_abort(); failed_program_address = UINT32_MAX;
    /* Invalid catalogs are never autoformatted by a radio START. */
    receiver(); flash_bytes[0x90000u] ^= 1u; replay(0u, true);
    CHECK(!ir_file_busy() && flash_bytes[0x90000u] != 'P'); guard();
}

static void test_file_sender_cancellation(void) {
    reset_device(); CHECK(pogo_flash_file_format() == POGO_FLASH_FILE_OK);
    CHECK(pogo_flash_file_create_bytes(2u, "payload", 100u, 1u, file_content) == POGO_FLASH_FILE_OK);
    uint8_t scratch[256]; file_count = 0;
    serial_input[0] = 'Q'; serial_count = 1u;
    CHECK(!ir_file_send(2u, 1u, false, scratch));
    CHECK(file_count == 2u);
    CHECK(((struct sfl_frame *)file_frames[0].payload)->cmd == IR_FILE_START);
    CHECK(((struct sfl_frame *)file_frames[1].payload)->cmd == IR_FILE_ABORT);
    serial_count = serial_read = 0u; radio_error = true;
    CHECK(!ir_file_send(2u, 1u, false, scratch)); radio_error = false;
}

static void test_file_repeated_pass_repair(void) {
    capture_passes(1101u, false, 3u); receiver();
    unsigned pass = 0u, ends = 0u;
    uint32_t identity = ir_v2_read_u32(((struct sfl_frame *)file_frames[0].payload)->payload + 2);
    for (unsigned i = 0; i < file_count; ++i) {
        struct sfl_frame *f = (struct sfl_frame *)file_frames[i].payload;
        if (f->cmd == IR_FILE_START)
            CHECK(ir_v2_read_u32(f->payload + 2) == identity);
        if (f->cmd == IR_FILE_DATA) {
            unsigned index = ir_v2_read_u16(f->payload + 4);
            if ((pass == 0u && index < 5u) || (pass == 1u && index != 0u)) continue;
        }
        apply(&file_frames[i]);
        if (f->cmd == IR_FILE_END && ++ends == 3u) { ends = 0u; ++pass; }
    }
    CHECK(pass == 3u && upload_stats.fec_recovered == 4u);
    verified(1101u, 1u);
    pogo_flash_file_info_t info;
    CHECK(pogo_flash_file_find(1u, &info) == POGO_FLASH_FILE_OK && info.generation == 1u);
}

static void test_file_discovery_and_malformed_packets(void) {
    capture(1101u, false); receiver();
    /* Real loop entry, outer packet classification and inner-CRC rejection.
     * One damaged DATA is repaired by the transmitted parity. */
    reset_messages(); CHECK(file_count < MAX_MESSAGES);
    memcpy(messages, file_frames + 1u, (file_count - 1u) * sizeof(message_t));
    message_count = file_count - 1u;
    /* Preparation completes during the initial frame handler. Real senders
     * keep DATA off the wire until after it; release queued frames afterward. */
    message_release_us = 2000u;
    for (unsigned i = 0; i < message_count; ++i) {
        struct sfl_frame *f = (struct sfl_frame *)messages[i].payload;
        if (f->cmd == IR_FILE_DATA) { f->crc[0] ^= 1u; break; }
    }
    CHECK(ir_boot_try_v2_start(&file_frames[0])); verified(1101u, 1u);
    CHECK(upload_stats.crc_errors == 1u && upload_stats.fec_recovered == 1u);
    receiver(); reset_messages();
    memcpy(messages, file_frames, 3u * sizeof(message_t)); message_count = 3u;
    apply(&file_frames[0]);
    CHECK(message_read == message_count && ir_file_busy());
    reset_messages(); replay(0u, true); verified(1101u, 1u);
    receiver();
    for (unsigned bad = 0; bad < 7u; ++bad) {
        message_t m = file_frames[0];
        struct sfl_frame *f = (struct sfl_frame *)m.payload;
        if (bad == 0) memset(f->payload + 2, 0, 4);
        if (bad == 1) f->payload[16] = 33u;
        if (bad == 2) put32(f->payload + 6, POGO_FLASH_FILE_MAX_BYTES + 1u);
        if (bad == 3) { f->payload[14] = 0x80u; f->payload[15] = 1u; }
        if (bad == 4) f->payload[1] = 0u;
        if (bad == 5) f->payload[0] = 2u;
        if (bad == 6) f->payload[48] = 1u;
        uint16_t crc = crc16(&f->cmd, f->payload_length + 1u);
        f->crc[0] = (uint8_t)(crc >> 8); f->crc[1] = (uint8_t)crc;
        unsigned erased = all_erase4, programmed = all_programs;
        apply(&m);
        CHECK(!ir_file_busy() && all_erase4 == erased && all_programs == programmed);
    }
    apply(&file_frames[0]); CHECK(ir_file_busy());
    message_t firmware = {0}; firmware.header._packet_type = ir_t_flash;
    firmware.header.payload_length = IR_V2_START_LENGTH + 4u;
    struct sfl_frame *f = (struct sfl_frame *)firmware.payload;
    f->payload_length = IR_V2_START_LENGTH; f->cmd = IR_V2_CMD_START;
    uint16_t crc = crc16(&f->cmd, f->payload_length + 1u);
    f->crc[0] = (uint8_t)(crc >> 8); f->crc[1] = (uint8_t)crc;
    CHECK(!ir_boot_try_v2_start(&firmware)); CHECK(ir_file_busy());
    ir_file_abort(); guard();
}

int main(void) {
    for (unsigned i = 0; i < sizeof(file_content); ++i)
        file_content[i] = (uint8_t)(i * 37u + i / 256u);
    test_file_roundtrip_sizes_and_fec();
    test_file_resume_and_source_binding();
    test_file_unpublished_abort_and_reboot();
    test_file_replacement_and_conflicts();
    test_file_erase_write_and_crc_failures();
    test_file_sender_cancellation();
    test_file_repeated_pass_repair();
    test_file_discovery_and_malformed_packets();
    puts("IR PFFS file sender/receiver/NOR fault injection: PASS");
    return 0;
}
