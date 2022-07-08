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

#include <sfl.h>
#include <boot.h>
#include <ir_boot.h>
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

/* Returns 1 if other boot methods should be tried */
int serialboot(void)
{
	struct sfl_frame frame;
	int failures;
	static const char str[] = SFL_MAGIC_REQ;
	const char *c;
	int ack_status;
    uintptr_t ptr=0;    // last flashed mem address
    time_reference_t my_timer;
#ifdef REMOCON
    uint32_t crc_ptr=4; // Add later the size of the program to flash
    uint8_t crc_log [32768]; // 32K enough for gateware+bios (~3072 blocks of 64 bytes, x8 : address(4), size(1), CRC(2) + 1 0xFF)
#endif

/*#ifdef REMOCON
    // Send IR_MAGIC_REQ first
    static const char ir_str[] = IR_MAGIC_REQ;
    IRn_tx_write_msg(0xF, (ir_uart_word_t *)ir_str, strlen(ir_str));
    printf("Sending IR_MAGIC_REQ : %s, of length: %d\n",ir_str, strlen(ir_str));
#else */
	printf("Booting from serial...\n");
	printf("Press Q or ESC to abort boot completely.\n");

//#endif //REMOCON
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
		int i;
		int timeout;
		int computed_crc;
		int received_crc;
        uintptr_t addr;
#ifndef REMOCON
        uint32_t jump_addr;
#endif

		/* Get one Frame */
		i = 0;
		timeout = 1;
        pogobot_timer_init( &my_timer, CMD_TIMEOUT_DELAY);
		while((i == 0) || !pogobot_timer_has_expired(&my_timer)) {
			if (uart_read_nonblock()) {
				if (i == 0) {
						frame.payload_length = uart_read();
				}
				if (i == 1) frame.crc[0] = uart_read();
				if (i == 2) frame.crc[1] = uart_read();
				if (i == 3) frame.cmd    = uart_read();
				if (i >= 4) {
					frame.payload[i-4] = uart_read();
					if (i == (frame.payload_length + 4 - 1)) {
						timeout = 0;
						break;
					}
				}
				i++;
			}
		}

		/* Check Timeout */
		if (timeout) {
			/* Acknowledge the Timeout and continue with a new frame */
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

/* #ifdef REMOCON
        // When you're a remote control, you flash nothing but instead forward the orders to IR
        IRn_tx_write_msg(0xF, (ir_uart_word_t *)&frame, frame.payload_length +4 );    // TODO: Fix the mask
#endif */
		/* Execute Frame CMD */
		switch(frame.cmd) {
			/* On SFL_CMD_ABORT ... */
			case SFL_CMD_ABORT:
				/* Reset failures */
				failures = 0;
				/* Acknowledge and exit */
				uart_write(SFL_ACK_SUCCESS);
				return 1;

			/* On SFL_CMD_LOAD... */
			case SFL_CMD_LOAD:

				/* Reset failures */
				failures = 0;

                addr = get_uint32(&frame.payload[0]);
//#ifndef REMOCON
#ifdef CSR_SPIFLASH_BASE
                /* Write to SPI flash instead of memcpy if address is inside mapped flash area */
                if( (addr >= SPIFLASH_BASE) & (addr < SPIFLASH_BASE+SPIFLASH_SIZE) ) {
                    addr -= SPIFLASH_BASE;        // addr is now just the offset in flash
                    // Only erase if address is on a sector boundary.
                    if ((addr & ~(SPIFLASH_SECTOR_SIZE - 1) ) == addr) {
                        erase_flash_sector(addr);
                        ptr = addr;               // Could be a jump to a new sector
#ifdef REMOCON
                        if( addr == 0x60000 ) {   // Start of user prog
                            erase_flash_sector(USRPRG_CRC);     // Erase prog CRCs
                        }
#endif // REMOCON
                    }
                    if(ptr != addr) {
                        printf("Error : Non contiguous flashing, expected: %" PRIxPTR ", actual address: %" PRIxPTR "\n", ptr, addr);
                        return 0;
                    }
                    write_to_flash(addr, (unsigned char *)&frame.payload[4], frame.payload_length - 4);
                    ptr = addr + frame.payload_length - 4;    // Should be the next address to flash
#ifdef REMOCON
                    // Record metadata to flash
                    if( addr >= 0x60000 ) {   // user prog
                        crc_log[crc_ptr++] = frame.payload[3];
                        crc_log[crc_ptr++] = frame.payload[2];
                        crc_log[crc_ptr++] = frame.payload[1];
                        crc_log[crc_ptr++] = frame.payload[0];
                        crc_log[crc_ptr++] = frame.payload_length-4;
                        crc_log[crc_ptr++] = frame.crc[1];
                        crc_log[crc_ptr++] = frame.crc[0];
                        crc_log[crc_ptr++] = 0xff;
                    }
#endif // REMOCON
                }
                else
				/* Copy payload to RAM */
#endif //CSR_SPIFLASH_BASE
                {
                    char *load_addr;
                    load_addr = (char *)addr;
                    memcpy(load_addr, &frame.payload[4], frame.payload_length - 4);
                }
//#endif // not REMOCON
				/* Acknowledge */
                uart_write(SFL_ACK_SUCCESS);
				break;

			/* On SFL_CMD_JUMP... */
			case SFL_CMD_JUMP:
#ifdef CSR_SPIFLASH_BASE
#ifdef REMOCON
                addr = get_uint32(&frame.payload[0]);
                addr -= SPIFLASH_BASE;          // addr is now just the offset in flash
                ptr -= 0x60000;                 // ptr is now the size of program in flash
                if( addr == 0x60000 ) {
                    crc_log[3]=((ptr >> 24) & 0xff);
                    crc_log[2]=((ptr >> 16) & 0xff);
                    crc_log[1]=((ptr >> 8) & 0xff);
                    crc_log[0]=(ptr & 0xff);
                    write_to_flash(USRPRG_CRC+SPIFLASH_BASE, crc_log, crc_ptr);
                }
#endif // REMOCON

				/* Reset failures */
				failures = 0;
                
                /* Serial flash successful, write magic value to flash */
                char * flash_ok = FLASH_IS_OK;
                write_to_flash(FLASH_OK_OFFSET, (unsigned char *)flash_ok, strlen(flash_ok));
#endif //CSR_SPIFLASH_BASE

				/* Acknowledge and jump */
				uart_write(SFL_ACK_SUCCESS);

#ifndef REMOCON
				jump_addr = get_uint32(&frame.payload[0]);
                if(jump_addr <= (SPIFLASH_BASE + 0x40000) ) {
                    printf("Rebooting to bootloader\n");
                    reboot_ctrl_write(0xac);
                }
                else {
                    printf("Rebooting to user image\n");
                    reboot_ctrl_write(0xac | 1);
                }
#else
                if( addr == 0x60000 ) {
                    printf("ptr = 0x%08x\n", ptr);
                    printf("addr = 0x%08x\n", addr);
                    printf("crc_ptr = %ld\n", crc_ptr);
                    printf("Log written to flash : \n");
                    for( uint16_t i=0; i<256; i++) { 
                        printf("0x%02x ", crc_log[i]);
                        if(!((i+1)%8)) printf("\n");
                    }
                }
#endif // REMOCON 
                return 0;
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

uint8_t check_flash_state(const char * data, uint32_t address_in_flash) {
    // Check if string is present at address in flash or not
    spiFree();                                                      // Enable memory-mapped mode
    if(data == 0) return 0;
    char *flash_check = (char *)(SPIFLASH_BASE + address_in_flash);  // Address of OK message
    for(uint8_t i=0; i<strlen(data); i++) {
        if( data[i] != *(flash_check+i) ) {
            return 0;
        }
    }
	return 1;
}

