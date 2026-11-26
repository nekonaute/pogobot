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
#include <ctype.h> 

#include <command.h>
#include <helpers.h> // get_param
#include <readline.h>
#include <memtest.h>
#include <ir_boot.h>
#include <boot.h>
#include <sfl.h>
#include <spiflash.h>

#include <pogobot.h>
#include "pffs.h"
#include "ir_file.h"
#include "console_input.h"

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

// Standby option status
uint8_t standby_status = 0; // 0 : OFF, 1 : ON LISTENING, 2 : ON SAVE ENERGY
uint8_t standby_reloaded = 0; // to check if the listening has begin (0 if is needed to reload the timer )
// Voltage status
uint8_t voltage_status = 0; // 0 : OFF, 1 : DISPLAY VOLTAGE
// Autotest status
uint8_t autotest_status = 0; // 0 : OFF, 1 : DISPLAY AUTOTEST RESULT
// Autotest done
uint8_t autotest_done = 0; // 0 : NO, 1 : YES

int main(void) {
    char buffer[CMD_LINE_BUFFER_SIZE];
    char *params[MAX_PARAM];
    char *command;
    struct command_struct *cmd;
    int nb_params;
    extern uint64_t unique_id, serial_number;

    pogobot_init(); // Needed for printf() (interrupts), slip, ...
    bios_pffs_init(); /* Never formats flash; filesystem lookup is lazy. */

    printf("\n");
    printf("\e[1m     ____                   _           _         \e[0m\n");
    printf("\e[1m    |  _ \\ ___   __ _  ___ | |__   ___ | |_      \e[0m\n");
    printf("\e[1m    | |_) / _ \\ / _` |/ _ \\| '_ \\ / _ \\| __|  \e[0m\n");
    printf("\e[1m    |  __/ (_) | (_| | (_) | |_) | (_) | |_       \e[0m\n");
    printf("\e[1m    |_|   \\___/ \\__, |\\___/|_.__/ \\___/ \\__| \e[0m\n");
    printf("\e[1m                 |___/                            \e[0m\n");
    printf("\e[1m    https://pogobot.github.io                     \e[0m\n");
    printf("\n");

    printf(" Licensed under the MIT License (also known as Expat License).\n");

    printf(" (c) Copyright 2022-2023 Sorbonne Université, Institut des Systèmes Intelligents et de Robotique (ISIR)\n");
    printf(" (c) Copyright 2012-2022 Enjoy-Digital\n");
    printf(" (c) Copyright 2007-2015 M-Labs\n");
    printf(" Work funded by the Agence Nationale pour la Recherche (ANR-18-CE33-0006).\n");
    printf("\n");

#ifdef CONFIG_WITH_BUILD_TIME
    printf(" BIOS built on "__DATE__" "__TIME__"\n");
#endif
	//crcbios();
    printf(" LiteX git sha1: "LITEX_GIT_SHA1"\n");
    printf(" PogoSoC git sha1: "POGOSOC_GIT_SHA1"\n");
    pogobot_helper_print_version();
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

    // color wheel memory
    uint8_t wheelpos = 0;

#ifndef REMOCON
    static const char str_magic[] = IR_MAGIC_REQ;
    //standby timer
    time_reference_t my_standby_timer;
    uint8_t debug_mode = 0;     // debug_mode at 1 if uart connected

#ifdef RGB_LEDS
    rgb_blink_set_time(5, 995); // 5ms flash every second
    update_led_status();
#else
    rgb_set(255,0,0);
#endif
#else //REMOCON
    // Set power to max if remocon
    IRn_conf_tx_power_write( 0, 3 ); 
#ifdef RGB_LEDS
    rgb_set(0,10,10);
#endif
#endif

    printf("\n%s", POGOPROMPT);

    // main loop
    while(1) {
        bios_pffs_service();
        if (bios_pffs_input_reset()) printf("\n%s", POGOPROMPT);

#ifndef REMOCON
        /* Background diagnostics and sleep cannot interrupt a file session. */
        if (!bios_pffs_busy()) {
        // standby control
        if (standby_status > 0)
        {
            if (standby_status == 2)
            {
                // wait 6 secondes deaf
                rgb_set(0, 0, 0);
                pogobot_timer_init( &my_standby_timer, 6000000 );
                while (!pogobot_timer_has_expired( &my_standby_timer )) 
                { 
                    //do nothing
                    __asm__( /* Assembly function body */
                    "  nop	\n"
                    "  nop	\n"
                    "  nop	\n"
                    "  nop	\n"
                    "  nop	\n"
                    "  nop	\n"
                    );
                }

                // ending hibernation
                standby_status = 1;
                standby_reloaded = 0;
                int i=0;
                for( i=0; i<IR_RX_COUNT; i++) {
                    ts_wakeUp(i);
                }

            } else if (standby_status == 1) {

                // listen during 3 seconds 
                if (!standby_reloaded)
                {
                    rgb_set(30, 30, 30);
                    msleep(5);
                    rgb_set(0, 0, 0);
                    pogobot_timer_init( &my_standby_timer, 3000000 );
                    standby_reloaded = 1;
                }
                if (pogobot_timer_has_expired( &my_standby_timer ))
                {
                    // go to hibernation
                    int i=0;
                    for( i=0; i<IR_RX_COUNT; i++) {
                        ts_goToSleep(i);
                    }
                    standby_status = 2;
                }

            }
            
        } else if (voltage_status == 1)
        {
            command_dispatcher("bat_life", 0, NULL);
            // no need to check the flash status

        } else if (autotest_status == 1)
        {
            if ( !autotest_done) {
                command_dispatcher("auto_test", 0, NULL);
                // no need to check the flash status
                autotest_done = 1;
            }

        } else {
            if( debug_mode == 0 ) rgb_blink(); // Blink when not debugging 
        }
        }
        
        
#endif
        // cable communication 
		if(uart_read_nonblock() != 0)
		{
#ifndef REMOCON
            debug_mode = 1;
#endif
			readline(buffer, CMD_LINE_BUFFER_SIZE);
			wheelpos+=100;
			color_wheel(wheelpos);
			if (buffer[0] != 0) {
				printf("\n");
				nb_params = bios_console_parse(buffer, &command, params);
				cmd = bios_console_dispatch(command, nb_params, params, true);
				if (cmd) {
                    /* load_run owns its failure LED; marker colors would
                     * hide a missing/corrupt file behind an older image. */
                    if (!bios_pffs_busy() && strcmp(cmd->name, "load_run") != 0)
                        update_led_status();
                } else {
                    printf("Command not found\n");
                }
						
			}
			printf("\n%s", POGOPROMPT);
		}

#ifndef REMOCON

        //IR communication
        //if( ROM_BASE == (SPIFLASH_BASE + 0x20000) ) {   // Bootloader mode
            pogobot_infrared_update();
            while ( pogobot_infrared_message_available() ) {
                message_t msg;
                pogobot_infrared_recover_next_message( &msg );
                /* A CRC-checked START enters reception directly. Pending IR
                 * files allow their matching START to resume after timeout;
                 * other filesystem sessions defer all incoming uploads. */
                if ((!bios_pffs_busy() || ir_file_busy()) &&
                    ir_boot_try_v2_start(&msg)) continue;
                if (bios_pffs_busy()) continue;
                /* Late ENDs and data for a refused file are protocol traffic,
                 * not application text to dump into the interactive console. */
                if (msg.header._packet_type == ir_t_user && msg.header.payload_length >= 4u) {
                    struct sfl_frame *f = (struct sfl_frame *)msg.payload;
                    if (f->cmd >= IR_FILE_START && f->cmd <= IR_FILE_PARITY &&
                        f->payload_length == msg.header.payload_length - 4u &&
                        check_crc(f)) continue;
                }
                if (msg.header.payload_length < strlen(str_magic) ||
                    memcmp(msg.payload, str_magic, strlen(str_magic)) != 0) {
                    printf("Message received : Destination on IR %d, sender %d on IR %d, length %d ",
                            msg.header._receiver_ir_index,
                            msg.header._sender_id, msg.header._sender_ir_index,
                            msg.header.payload_length);
                    for(uint16_t byte=0; byte<msg.header.payload_length; byte++) {
                        printf("%c", isprint(msg.payload[byte])?msg.payload[byte]:'_');
                    }
                    printf("\n");
                } else {
                    /* Tokenize a terminated, bounded copy; IR payloads do not
                     * have to contain the UART parser's trailing NUL. */
                    size_t prefix = strlen(str_magic);
                    size_t length = msg.header.payload_length - prefix;
                    if (length >= sizeof(buffer)) continue;
                    memcpy(buffer, msg.payload + prefix, length);
                    buffer[length] = '\0';
                    nb_params = bios_console_parse(buffer, &command, params);
                    printf("Command : %s\n", command);
                    cmd = bios_console_dispatch(command, nb_params, params, false);
                    if (cmd && strcmp(cmd->name, "load_run") != 0) {
                        update_led_status();
                    } else if (!cmd) {
                        printf("Command not found\n");
                    }
                }
            }
		//}
#endif //REMOCON
    }

    return 0;
}
