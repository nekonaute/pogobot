/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <console.h>
#include <string.h>
#include <uart.h>
#include <system.h>
#include <irq.h>
#include <init.h>

#include <command.h>
#include <helpers.h> // get_param
#include <readline.h>
#include <memtest.h>
#include <ir_boot.h>
#include <boot.h>
#include <sfl.h>
#include <spiflash.h>

#include <pogobot.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#ifdef REMOCON
#define POGOPROMPT "\e[92;1mRemoteControl\e[0m> "
#else
#if( ROM_BASE == (SPIFLASH_BASE + 0x20000) )
#define POGOPROMPT "\e[92;1mPogoboot\e[0m> "
#else
#define POGOPROMPT "\e[92;1mPogobot\e[0m> "
#endif
#endif

int main(void) {
    char buffer[CMD_LINE_BUFFER_SIZE];
    char *params[MAX_PARAM];
    char *command;
    struct command_struct *cmd;
    int nb_params;
    extern uint64_t unique_id, serial_number;

    pogobot_init(); // Needed for printf() (interrupts)

    printf("\n");
    printf("\e[1m        __   _ __      _  __\e[0m\n");
    printf("\e[1m       / /  (_) /____ | |/_/\e[0m\n");
    printf("\e[1m      / /__/ / __/ -_)>  <\e[0m\n");
    printf("\e[1m     /____/_/\\__/\\__/_/|_|\e[0m\n");
    printf("\e[1m   Build your hardware, easily!\e[0m\n");
    printf("\n");
    printf(" (c) Copyright 2012-2022 Enjoy-Digital\n");
    printf(" (c) Copyright 2007-2015 M-Labs\n");
    printf("\n");
#ifdef CONFIG_WITH_BUILD_TIME
    printf(" BIOS built on "__DATE__" "__TIME__"\n");
#endif
	//crcbios();
    printf(" LiteX git sha1: "LITEX_GIT_SHA1"\n");
    printf(" PogoSoC git sha1: "POGOSOC_GIT_SHA1"\n");
    if(unique_id != 0) printf(" Unique ID: 0x%08lx%08lx\n", (uint32_t)(unique_id>>32), (uint32_t)(unique_id & 0xffffffff));
    if(serial_number != 0) { printf(" Serial #: 0x%08lx%08lx, ", (uint32_t)(serial_number>>32), (uint32_t)(serial_number & 0xffffffff));
    printSerial(); }
    printf("\n");
#ifdef REMOCON
    printf(" Remote control for pogobots\n");
#else
    printf(" Battery voltage : %ld mV\n", (ADC_Read(3)*6445)/1000);
#endif
    printf("\n--=============== \e[1mSoC\e[0m ==================--\n");
	printf("\e[1mCPU\e[0m:\t\t%s @ %dMHz\n",
        CONFIG_CPU_HUMAN_NAME,
        CONFIG_CLOCK_FREQUENCY/1000000);
    printf("\e[1mBUS\e[0m:\t\t%s %d-bit @ %dGiB\n",
        CONFIG_BUS_STANDARD,
        CONFIG_BUS_DATA_WIDTH,
        (1 << (CONFIG_BUS_ADDRESS_WIDTH - 30)));
    printf("\e[1mCSR\e[0m:\t\t%d-bit data\n",
        CONFIG_CSR_DATA_WIDTH);
    printf("\e[1mROM\e[0m:\t\t%dKiB\n", ROM_SIZE/1024);
    printf("\e[1mSRAM\e[0m:\t\t%dKiB\n", SRAM_SIZE/1024);
#ifdef CONFIG_L2_SIZE
    printf("\e[1mL2\e[0m:\t\t%dKiB\n", CONFIG_L2_SIZE/1024);
#endif

#if !defined(TERM_MINI) && !defined(TERM_NO_HIST)
        hist_init();
#endif

    uint8_t wheelpos = 0;
    time_reference_t my_autostart_timer;
    pogobot_stopwatch_reset( &my_autostart_timer );

    uint8_t debug_mode = 0;     // debug_mode at 1 if uart connected

#ifndef REMOCON
    uint8_t ir_i;
    uint8_t char_index[IR_NUMBER];
    uint8_t recognized_word[IR_NUMBER];
    char ir_buf[IR_NUMBER][CMD_LINE_BUFFER_SIZE];
    for(ir_i=0; ir_i<IR_NUMBER; ir_i++) {
        char_index[ir_i]=0;
        recognized_word[ir_i]=0;
    }
    uint8_t flash_state = check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET);  // return 1 : programmed, or 0: probably erased

#ifdef RGB_LEDS
    rgb_blink_set_time(5, 995); // 5ms flash every second
    if(flash_state) {
        rgb_blink_set_color(0,40,0);
    }
    else {
        rgb_blink_set_color(0,0,40);
    }
#else
    rgb_set(255,0,0);
#endif
#else //REMOCON
    IRn_conf_tx_power_write( 0, 3 ); // Set power to max if remocon
#ifdef RGB_LEDS
    //rgb_blink_set_time(5, 495); // 5ms flash every 500ms
    rgb_set(0,10,10);
#endif
#endif

    printf("\n%s", POGOPROMPT);

    while(1) {
#ifndef REMOCON
        if( debug_mode == 0 ) rgb_blink(); // Blink when not debugging 
#endif
		if(uart_read_nonblock() != 0)
		{
            debug_mode = 1;
			readline(buffer, CMD_LINE_BUFFER_SIZE);
			wheelpos+=100;
			color_wheel(wheelpos);
			if (buffer[0] != 0) {
				printf("\n");
				nb_params = get_param(buffer, &command, params);
				cmd = command_dispatcher(command, nb_params, params);
				if (!cmd)
						printf("Command not found");
			}
			printf("\n%s", POGOPROMPT);
		}

#ifndef REMOCON
        //if( ROM_BASE == (SPIFLASH_BASE + 0x20000) ) {   // Bootloader mode
            static const char str[] = IR_MAGIC_REQ;
            for( ir_i=0; ir_i<IR_NUMBER; ir_i++) {
                if((ir_uart_read_nonblock(ir_i) != 0) ) {
                    char c;
                    c = ir_uart_read(ir_i);
                    if(recognized_word[ir_i] == 0) {
                        if(c == str[char_index[ir_i]]) {
                            char_index[ir_i]++;
                            if(char_index[ir_i] == strlen(str))
                            {
                                recognized_word[ir_i]++;
                                char_index[ir_i]=0;
                            }
                        }
                        else {
                            if(c == str[0])
                                char_index[ir_i] = 1;
                            else {
                                char_index[ir_i] = 0;
                            }
                        }
                    }
                    else { // Magic word already received
                        if(c == '\n') { // End of command received
                            if (ir_buf[ir_i][0] != 0) {
                                nb_params = get_param(ir_buf[ir_i], &command, params);
                                cmd = command_dispatcher(command, nb_params, params);
                                if (cmd) {
                                    flash_state = check_flash_state(FLASH_IS_OK, FLASH_OK_OFFSET);  // Update flash state after each command
                                }
                            }
                            for(uint8_t i=0; i<IR_NUMBER; i++) {    // Forget about other messages received on a different IR
                                recognized_word[i]=0;
                                char_index[i]=0;
                            }
                        }
                        else {
                            ir_buf[ir_i][char_index[ir_i]] = c;
                            char_index[ir_i]++;
                            if( char_index[ir_i] == CMD_LINE_BUFFER_SIZE ) {    // Something went wrong...
                                char_index[ir_i]=0;
                                recognized_word[ir_i]=0;
                            }
                        }
                    }
				}
			}
		//}
#endif //REMOCON
    }

    return 0;
}
