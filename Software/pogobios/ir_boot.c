/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


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
#include <ir_uart.h>
#include <libbase/crc.h>
#include <libbase/spiflash.h>
#include <pogobot.h>

#define NB_MISSING_ADDR 10

static unsigned int success_addr, ptr;
const char * ir_magic_req = IR_MAGIC_REQ;
static int missing_packet = 0;
static unsigned int recovered_packet = 0;

static unsigned int l_missing_packet[NB_MISSING_ADDR];
static uint8_t ptr_l_missing_packet = 0;

static int flash_state_partial = 0;

static void add_to_missing_list(unsigned int addr) {

    if (ptr_l_missing_packet >= NB_MISSING_ADDR-1) {
        return;
    }
    printf("adding %x at ptr %d\n", addr, ptr_l_missing_packet);
    l_missing_packet[ptr_l_missing_packet] = addr;
    ptr_l_missing_packet += 1;

    return;
}

static uint8_t in_missing_list (unsigned int addr) {

    for (int i = 0; i < ptr_l_missing_packet; i++)
    {
        if (addr == l_missing_packet[i])
        {
            printf("found missing addr : %x\n", addr);
            return 1;
        }
    }

    return 0;

}

static void print_missing_list (void) {

    printf("mssing list addr : ");

    for (int i = 0; i < ptr_l_missing_packet; i++)
    {
        printf("%x\t", l_missing_packet[i]);
    }

    printf("\n");

    return;

}

uint8_t check_crc(struct sfl_frame* frame) {
	/* Check Frame CRC */
    int computed_crc;
    int received_crc;
	received_crc = ((int)frame->crc[0] << 8)|(int)frame->crc[1];
	computed_crc = crc16(&frame->cmd, frame->payload_length+1);
	return(computed_crc == received_crc);
}

void print_frame(struct sfl_frame* frame) {
    int i;
    uint32_t addr;
    printf("size: 0x%02x\n", frame->payload_length);
    printf("crc: 0x%04x\n", ((int)frame->crc[0] << 8)|(int)frame->crc[1]);
    printf("cmd: 0x%02x\ndata:", frame->cmd);
    addr = get_uint32(&frame->payload[0]);
    for( i=4; i<frame->payload_length; i++ ) {
        if( (i-4)%16 == 0 ) {
            printf("\n%08lx ", addr+i-4);
        }
        else {
            if( (i-4)%8 == 0 ) {
                printf(" ");
            }
        }
        printf(" %02x", frame->payload[i]);
    }
    printf("\n");
}

static unsigned int exec_frame_cmd(struct sfl_frame* frame)
{
    unsigned int addr=0;
    char * flash_ok = FLASH_IS_OK;
    char * flash_par = FLASH_IS_PARTIAL;

    //printf("m %d, r %d, p %d\n", missing_packet, recovered_packet, ptr_l_missing_packet);

	/* Execute Frame CMD and return flashed address (or reboot) */
	switch(frame->cmd) {
        case SFL_CMD_ABORT:
            printf("transfert abord!! \n");
            missing_packet = 0xFFFFFF;

		case SFL_CMD_LOAD:
			addr = get_uint32(&frame->payload[0]);
			// Check if we try to write to flash, and not overwrite the bootloader
			if( (addr >= SPIFLASH_BASE+0x40000) & (addr < SPIFLASH_BASE+SPIFLASH_SIZE) ) {
                addr -= SPIFLASH_BASE; // addr is now just the offset in flash
                if ((flash_state_partial && in_missing_list(addr)) || !flash_state_partial) {   
                    // Flash only if not already flashed before (duplicated messages)  
                    if(addr != success_addr) {
                        success_addr = addr;
                        // Only erase if address is on a sector boundary.
                        if ((addr & ~(SPIFLASH_SECTOR_SIZE - 1) ) == addr) {
                            printf("Erasing sector 0x%" PRIxPTR "\n", addr);
                            erase_flash_sector(addr);
                            ptr = addr;
                            if((addr == 0x40000) | ( addr == 0x60000 )) { 
                                printf("Erasing flashOK flag\n");
                                spiBeginErase4(FLASH_OK_OFFSET);
                            }
                        }
                        if( ptr != addr && !flash_state_partial) {
                            printf("Error : Non contiguous flashing, expected: %" PRIxPTR ", actual address: %" PRIxPTR "\n", ptr, addr);
                            for( unsigned int m_addr = ptr; m_addr < addr; m_addr += frame->payload_length - 4) {
                                add_to_missing_list(m_addr);
                                missing_packet++;
                            }
                        }
                        if (flash_state_partial)
                        {
                            printf("Recovered addr: %" PRIxPTR "\n", addr);
                            recovered_packet ++;
                        }
                        
                        write_to_flash(addr, (unsigned char *)&frame->payload[4], frame->payload_length - 4);
                        ptr=addr+frame->payload_length - 4;  // Should be the next address to flash
                        //printf("Flashed address %08x\n", addr);
                    }
                } 
			}
            break;
			
		case SFL_CMD_JUMP: 


            printf("jump m %d, r %d\n", missing_packet, recovered_packet);
            
            //erase the "flash is ok" token
    		spiBeginErase4(FLASH_OK_OFFSET);
            missing_packet -= recovered_packet;
            recovered_packet = 0;

            if (missing_packet > 0 && missing_packet < NB_MISSING_ADDR)
            {
                printf("missing %d packets\n", missing_packet);
                print_missing_list();
                /* Flash partially, write magic value */
                write_to_flash(FLASH_OK_OFFSET, (unsigned char *)flash_par, strlen(flash_par));
                return 1;
            } else if (missing_packet > NB_MISSING_ADDR) {
                printf(" too many missing packets (%d)\n", missing_packet);
                print_missing_list();
                return 1;
            }
            if (missing_packet < 0) {
                printf("something strange just happened !\n");
            }
            /* Flash successful, write magic value */
            write_to_flash(FLASH_OK_OFFSET, (unsigned char *)flash_ok, strlen(flash_ok));
            // no reboot on the code, we wait for a start command. 
            missing_packet = 0;
            ptr_l_missing_packet = 0;
			break;

		default:
			printf("CMD not recognized: %d\n", frame->cmd);
			break;
	}
    return 0;
}

void ir_boot_loop(void) {
    time_reference_t mytimer;
    uint32_t timeout = 10000000;  // in microseconds
    struct sfl_frame * frame;
    message_t msg;
    missing_packet = 0;
    success_addr=0;
    rgb_blink_set_time(5, 95);                      // Flash 5 ms every 100ms
    rgb_blink_set_color(0, 0, 50);                  // Tel the user data is being received

    pogobot_timer_init(&mytimer, timeout);          // Set a timeout

    flash_state_partial = check_flash_state(FLASH_IS_PARTIAL, FLASH_OK_OFFSET); 
    int flash_status_ok = check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET); 

    if (flash_state_partial) {
        printf (" partial programming ! \n ");
        missing_packet = ptr_l_missing_packet;
    } else {
        ptr_l_missing_packet = 0;
    }

    if (flash_status_ok)
    {
        // you need to erase the user prog before programming
        rgb_blink_set_time(5, 995);
        update_led_status();
        return;
    }
    

    //printf ("debut m %d, r %d, p %d \n", missing_packet, recovered_packet, ptr_l_missing_packet);

    while(!pogobot_timer_has_expired(&mytimer)) {
        pogobot_infrared_update();
        rgb_blink();
        /* Get one Frame */
        if( pogobot_infrared_message_available() ) {
            pogobot_timer_init(&mytimer, timeout);         // Reset timer each time we receive data
            pogobot_infrared_recover_next_message( &msg );
            if (msg.header._packet_type == ir_t_flash)
            {
            
                //printf("Received length : %d, [%s]\n", msg.header.payload_length, msg.payload);
                frame = (struct sfl_frame*) msg.payload;
                if(check_crc(frame)) {
                    if(exec_frame_cmd(frame) != 0) {
                        printf("exec_frame_cmd failed\n");
                        break;
                    }
                }
                else {
                    printf("CRC Error after address 0x%08x\n", success_addr);
                    print_frame(frame);
                }
                  
            }
        }
	}
    rgb_blink_set_time(5, 995);
    update_led_status();
}