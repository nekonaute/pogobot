/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


// This file is Copyright (c) 2014-2021 Florent Kermarrec <florent@enjoy-digital.fr>
// This file is Copyright (c) 2013-2014 Sebastien Bourdeauducq <sb@m-labs.hk>
// This file is Copyright (c) 2018 Ewen McNeill <ewen@naos.co.nz>
// This file is Copyright (c) 2018 Felix Held <felix-github@felixheld.de>
// This file is Copyright (c) 2019 Gabriel L. Somlo <gsomlo@gmail.com>
// This file is Copyright (c) 2017 Tim 'mithro' Ansell <mithro@mithis.com>
// This file is Copyright (c) 2018 William D. Jones <thor0505@comcast.net>
// License: BSD

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <system.h>
#include <string.h>
#include <irq.h>

#include <generated/mem.h>
#include <generated/csr.h>
#include <generated/soc.h>

#include <helpers.h> //get_params
#include <sfl.h>
#include <boot.h>
#include <ir_boot.h>
#include <ir_upload_v2.h>
#include <pogobot.h>

#include <libbase/uart.h>

#include <libbase/console.h>
#include <libbase/crc.h>
#include <libbase/progress.h>
#include <libbase/spiflash.h>
#include <libbase/jsmn.h>

/*-----------------------------------------------------------------------*/
/* Helpers                                                               */
/*-----------------------------------------------------------------------*/

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

/*-----------------------------------------------------------------------*/
/* Boot                                                                  */
/*-----------------------------------------------------------------------*/

void __attribute__((noreturn)) boot(unsigned long r1, unsigned long r2, unsigned long r3, unsigned long addr)
{
    while(1);
}

enum {
	ACK_TIMEOUT,
	ACK_CANCELLED,
	ACK_OK
};

/*-----------------------------------------------------------------------*/
/* Serial Boot/Flash                                                     */
/*-----------------------------------------------------------------------*/

#ifdef CSR_UART_BASE

#define ACK_TIMEOUT_DELAY CONFIG_CLOCK_FREQUENCY/4
#define CMD_TIMEOUT_DELAY CONFIG_CLOCK_FREQUENCY/16

static int check_ack(void)
{
	int recognized;
	static const char str[] = SFL_MAGIC_ACK;
    time_reference_t my_timer;

    pogobot_timer_init( &my_timer, ACK_TIMEOUT_DELAY);
	recognized = 0;
	while(!pogobot_timer_has_expired( &my_timer )) {
		if(uart_read_nonblock()) {
			char c;
			c = uart_read();
			if((c == 'Q') || (c == '\e'))
				return ACK_CANCELLED;
			if(c == str[recognized]) {
				recognized++;
				if(recognized == strlen(str))
					return ACK_OK;
			} else {
				if(c == str[0])
					recognized = 1;
				else
					recognized = 0;
			}
		}
	}
	return ACK_TIMEOUT;
}

uint32_t get_uint32(unsigned char* data)
{
	return ((uint32_t) data[0] << 24) |
			 ((uint32_t) data[1] << 16) |
			 ((uint32_t) data[2] << 8) |
			  (uint32_t) data[3];
}

#define MAX_FAILURES 256

#ifdef REMOCON
extern slip_send_descriptor_s slip_send_descriptor;
static void send_flash_message( char* mPayload, uint32_t size) 
{
    message_t message;
    message.header._emitting_power_list =
        pogobot_infrared_emitting_power_list(3, 3, 3, 3);
    message.header.payload_length = size;
    message.header._sender_ir_index = ir_all;
    message.header._receiver_ir_index = 0;  // The robot fills in the actual receiver on arrival.
    memcpy( message.payload, mPayload, size );
    message.header._packet_type = ir_t_flash; 
    message.header._sender_id = pogobot_helper_getid();

	slip_send_message( &( slip_send_descriptor ), (uint8_t *)&message,
							sizeof( message_header_t ) +
							message.header.payload_length, message.header.payload_length );

}
#endif

/* Receive a bounded header and payload, including a valid empty ABORT frame.
 * The deadline starts at the first byte so an idle link cannot truncate it. */
static int receive_sfl_frame(struct sfl_frame *frame)
{
    time_reference_t timer;
    unsigned int received = 0;
    while (received == 0 || !pogobot_timer_has_expired(&timer)) {
        if (!uart_read_nonblock())
            continue;
        uint8_t value = uart_read();
        if (received == 0) {
            frame->payload_length = value;
            pogobot_timer_init(&timer, CMD_TIMEOUT_DELAY);
        } else if (received == 1) {
            frame->crc[0] = value;
        } else if (received == 2) {
            frame->crc[1] = value;
        } else if (received == 3) {
            frame->cmd = value;
        } else {
            if (received - 4 >= sizeof(frame->payload))
                return 0;
            frame->payload[received - 4] = value;
        }
        if (++received == (unsigned int)frame->payload_length + 4u)
            return 1;
    }
    return 0;
}

#ifndef SERIAL_BOOT_RAM_WRITE
#define SERIAL_BOOT_RAM_WRITE(address, data, length) \
    memcpy((void *)(uintptr_t)(address), (data), (length))
#endif

/* Returns 1 if other boot methods should be tried */
int serialboot(void)
{
#ifdef REMOCON
    printf("Serialboot is desactivate.\n");
    printf("To programm the robots use rc_flash_robot.\n");
    return 1;
#else
    struct sfl_frame frame;
    int failures = 0;
#ifdef CSR_SPIFLASH_BASE
    uint32_t next_address = 0;
    uint8_t erased[(SPIFLASH_SIZE / SPIFLASH_SECTOR_SIZE + 7u) / 8u] = {0};
    uint8_t image_changed = 0, image_failed = 0;
#endif

    printf("Booting from serial...\n");
    printf("Press Q or ESC to abort boot completely.\n");
    for (const char *c = SFL_MAGIC_REQ; *c; c++)
        uart_write(*c);
    int ack_status = check_ack();
    if (ack_status != ACK_OK) {
        printf(ack_status == ACK_TIMEOUT ? "Timeout\n" : "Cancelled\n");
        return 0;
    }

    while (1) {
        if (!receive_sfl_frame(&frame)) {
            uart_write(SFL_ACK_ERROR);
            continue;
        }
        uint16_t received_crc = ((uint16_t)frame.crc[0] << 8) | frame.crc[1];
        if (crc16(&frame.cmd, frame.payload_length + 1) != received_crc) {
            uart_write(SFL_ACK_CRCERROR);
            if (++failures == MAX_FAILURES)
                return 0;
            continue;
        }

        switch (frame.cmd) {
        case SFL_CMD_ABORT:
            if (frame.payload_length != 0) {
                uart_write(SFL_ACK_ERROR);
                break;
            }
            uart_write(SFL_ACK_SUCCESS);
            return 1;

        case SFL_CMD_LOAD: {
            // LOAD needs an address and at least one byte of actual data.
            if (frame.payload_length <= 4) {
#ifdef CSR_SPIFLASH_BASE
                image_failed = image_changed;
#endif
                uart_write(SFL_ACK_ERROR);
                break;
            }
            uint32_t address = get_uint32(frame.payload);
            uint32_t length = frame.payload_length - 4;
#ifdef CSR_SPIFLASH_BASE
            if (address >= SPIFLASH_BASE &&
                address - SPIFLASH_BASE < SPIFLASH_SIZE) {
                uint32_t offset = address - SPIFLASH_BASE;
                if (length > SPIFLASH_SIZE - offset ||
                    (offset != next_address &&
                     offset % SPIFLASH_SECTOR_SIZE != 0)) {
                    image_failed = image_changed;
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                // Any overlapping application update invalidates cached IR
                // progress and the boot marker before erasing or programming.
                if (offset < 0x80000u && offset + length > 0x40000u &&
                    !image_changed) {
                    ir_boot_reset_v2_upload();
                    if (!flash_invalidate_user_image()) {
                        uart_write(SFL_ACK_ERROR);
                        break;
                    }
                    image_changed = 1;
                    write_to_flash(FLASH_V2_PARTIAL_OFFSET,
                                   (unsigned char *)FLASH_IS_PARTIAL,
                                   strlen(FLASH_IS_PARTIAL));
                }
                // A valid SFL payload may cross a sector boundary. Erase each
                // touched sector once, including for multi-image cable uploads.
                uint32_t first = offset / SPIFLASH_SECTOR_SIZE;
                uint32_t last = (offset + length - 1) / SPIFLASH_SECTOR_SIZE;
                for (uint32_t sector = first; sector <= last; sector++) {
                    uint8_t bit = 1u << (sector & 7u);
                    if (!(erased[sector >> 3] & bit)) {
                        erase_flash_sector(sector * SPIFLASH_SECTOR_SIZE);
                        erased[sector >> 3] |= bit;
                    }
                }
                write_to_flash(offset, frame.payload + 4, length);
                uint8_t written = 1;
                for (uint32_t byte = 0; byte < length; byte++)
                    if (POGOBIOS_FLASH_READ(offset + byte) != frame.payload[byte + 4])
                        written = 0;
                if (!written) {
                    image_failed = image_changed;
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                next_address = offset + length;
                failures = 0;
                uart_write(SFL_ACK_SUCCESS);
                break;
            }
#endif
#if defined(SRAM_BASE) && defined(SRAM_SIZE)
            if (address - SRAM_BASE < SRAM_SIZE &&
                length <= SRAM_SIZE - (address - SRAM_BASE)) {
                SERIAL_BOOT_RAM_WRITE(address, frame.payload + 4, length);
                failures = 0;
                uart_write(SFL_ACK_SUCCESS);
                break;
            }
#endif
            // Addresses outside generated writable memory are never RAM pointers.
#ifdef CSR_SPIFLASH_BASE
            image_failed = image_changed;
#endif
            uart_write(SFL_ACK_ERROR);
            break;
        }

        case SFL_CMD_JUMP:
            if (frame.payload_length != 4) {
                uart_write(SFL_ACK_ERROR);
                break;
            }
#ifdef CSR_SPIFLASH_BASE
            if (image_changed) {
                if (image_failed) {
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                // A partially programmed marker needs a sector erase before
                // retry; verified image data stays untouched.
                if (!check_flash_state(FLASH_ERASED_MARKER, FLASH_OK_OFFSET) &&
                    !check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET)) {
                    if (!flash_invalidate_user_image()) {
                        uart_write(SFL_ACK_ERROR);
                        break;
                    }
                    write_to_flash(FLASH_V2_PARTIAL_OFFSET,
                                   (unsigned char *)FLASH_IS_PARTIAL,
                                   strlen(FLASH_IS_PARTIAL));
                }
                write_to_flash(FLASH_OK_OFFSET, (unsigned char *)FLASH_IS_OK,
                               strlen(FLASH_IS_OK));
                if (!check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET)) {
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
            }
#endif
            uart_write(SFL_ACK_SUCCESS);
            printf(" execute 'run' to start the programm \n ");
            return 0;

        default:
            uart_write(SFL_ACK_UNKNOWN);
            if (++failures == MAX_FAILURES)
                return 0;
            break;
        }
    }
#endif
}

#ifdef REMOCON

/* Returns 1 if other boot methods should be tried */
int flash_robot(void)
{
	struct sfl_frame frame;
	int failures;
	uint8_t v2_mode = 0;
	static const char str[] = SFL_MAGIC_REQ;
	const char *c;
	int ack_status;
    printf("Sending from serial...\n");

	// sending 10x the command to start the reception 
	uint8_t i,j;
    uint16_t count = 0;
    extern const char * ir_magic_req;
	char * cmd = "ir_flash";
	message_t message;
    message.header._emitting_power_list =
        pogobot_infrared_emitting_power_list(3, 3, 3, 3);
    message.header._sender_ir_index = ir_all;
    message.header._packet_type = ir_t_cmd; 
    message.header._sender_id = pogobot_helper_getid();

    //First send the magic 
    for(i=0; i<strlen(ir_magic_req); i++)
    {
        message.payload[count++] = ir_magic_req[i];
    }
	for (j=0; j < strlen(cmd); j++)
	{
		message.payload[count++]=cmd[j];
	}
	message.payload[count++]=0;
	message.header.payload_length = count;
	for ( i = 0; i < 10; i++)
	{
		slip_send_message( &( slip_send_descriptor ), (uint8_t *)&message,
							sizeof( message_header_t ) +
							message.header.payload_length, message.header.payload_length );
		msleep(200);
	}
	
	/* Announce versioned IR support before the existing magic request. */
	/* Older terminals ignore this text and still recognize SFL_MAGIC_REQ. */
	c = IR_V2_CAPABILITY_BANNER;
	while(*c) {
		uart_write(*c);
		c++;
	}
	/* Send the serialboot "magic" request to Host and wait for ACK_OK */
	c = str;
	while(*c) {
		uart_write(*c);
		c++;
	}
	ack_status = check_ack();
	if(ack_status == ACK_TIMEOUT) {
		printf("Timeout\n");
		return 0;
	}
	if(ack_status == ACK_CANCELLED) {
		printf("Cancelled\n");
		return 0;
	}

	/* Assume ACK_OK */
	failures = 0;
	while(1) {
		int computed_crc;
		int received_crc;
        if (!receive_sfl_frame(&frame)) {
            uart_write(SFL_ACK_ERROR);
            continue;
        }

		/* Check Frame CRC */
		received_crc = ((int)frame.crc[0] << 8)|(int)frame.crc[1];
		computed_crc = crc16(&frame.cmd, frame.payload_length+1);
		if(computed_crc != received_crc) {
			/* Acknowledge the CRC error */
			uart_write(SFL_ACK_CRCERROR);

			/* Increment failures and exit when max is reached */
			failures++;
			if(failures == MAX_FAILURES) {
				printf("Too many consecutive errors, aborting");
				return 0;
			}
			continue;
		}
    
        /* Execute Frame CMD */
        switch(frame.cmd) {
            // JUMP only closes the PC connection after a versioned upload.
            // Legacy LOAD/ABORT is rejected by default and never broadcast.
            case SFL_CMD_JUMP:
                if (frame.payload_length != 4) {
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                failures = 0;
                uart_write(SFL_ACK_SUCCESS);
                return 0;

            case IR_V2_CMD_START:
                if (frame.payload_length != IR_V2_START_LENGTH) {
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                failures = 0;
                v2_mode = 1;
                // Repeated START is safe and improves discovery; pause for sector erases.
                for (uint8_t copy = 0; copy < 3; copy++) {
                    send_flash_message((char *)&frame, frame.payload_length + 4);
                    msleep(200);
                }
                msleep(3000);  // Conservative broadcast preparation for two 64 KiB erases.
                uart_write(SFL_ACK_SUCCESS);
                break;

            case IR_V2_CMD_DATA:
                if (!v2_mode || frame.payload_length < IR_V2_DATA_HEADER_LENGTH + 1 ||
                    frame.payload_length > IR_V2_DATA_HEADER_LENGTH + IR_V2_CHUNK_SIZE) {
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                failures = 0;
                send_flash_message((char *)&frame, frame.payload_length + 4);
                msleep(200);
                uart_write(SFL_ACK_SUCCESS);
                break;

            case IR_V2_CMD_PARITY:
                /* The PC computes parity; the remote only validates size and broadcasts it. */
                if (!v2_mode || frame.payload_length != IR_V2_PARITY_LENGTH) {
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                failures = 0;
                send_flash_message((char *)&frame, frame.payload_length + 4);
                msleep(200);
                uart_write(SFL_ACK_SUCCESS);
                break;

            case IR_V2_CMD_END:
            case IR_V2_CMD_ABORT:
                if (!v2_mode || frame.payload_length != IR_V2_ID_LENGTH) {
                    uart_write(SFL_ACK_ERROR);
                    break;
                }
                failures = 0;
                send_flash_message((char *)&frame, frame.payload_length + 4);
                msleep(200);
                uart_write(SFL_ACK_SUCCESS);
                if (frame.cmd == IR_V2_CMD_ABORT)
                    return 1;
                break;

			default:
				/* Increment failures */
				failures++;

				/* Acknowledge the UNKNOWN cmd */
				uart_write(SFL_ACK_UNKNOWN);

				/* Increment failures and exit when max is reached */
				if(failures == MAX_FAILURES) {
					printf("Too many consecutive errors, aborting");
					return 0;
				}

				break;
		}

	}
	return 1;
}

#endif

#endif

/* check if a string (data) is present at this adress (address_in_flash) */
uint8_t check_flash_state(const char * data, uint32_t address_in_flash) 
{
    // Check if string is present at address in flash or not
    spiFree();                                                      // Enable memory-mapped mode
    if(data == 0) return 0;
    for(uint8_t i=0; i<strlen(data); i++) {
        if( (uint8_t)data[i] != POGOBIOS_FLASH_READ(address_in_flash + i) ) {
            return 0;
        }
    }
	return 1;
}

// A failed validity-sector erase must not be followed by code modification.
uint8_t flash_invalidate_user_image(void)
{
    spiBeginErase4(FLASH_OK_OFFSET);
    return check_flash_state(FLASH_ERASED_MARKER, FLASH_OK_OFFSET) &&
           check_flash_state(FLASH_ERASED_MARKER, FLASH_V2_PARTIAL_OFFSET);
}

/* 
 * update the blink color depending of the flash status
 * green : ready
 * blue  : empty
 * orange: partial
 */
void update_led_status(void) 
{
	// check is ok
	int flash_state = check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET);  
	if(flash_state) {
        rgb_blink_set_color(0,40,0);
		return;
    }

	// check is partial
	flash_state = check_flash_state(FLASH_IS_PARTIAL, FLASH_OK_OFFSET) ||
                  check_flash_state(FLASH_IS_PARTIAL, FLASH_V2_PARTIAL_OFFSET);
	if(flash_state) {
        rgb_blink_set_color(40,15,0);
    }
    else {
        rgb_blink_set_color(0,0,40);
    }

	return;
}
