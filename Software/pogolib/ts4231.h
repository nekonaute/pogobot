/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


/*******************************************************************
    Copyright (C) 2017 Triad Semiconductor

    ts4231.h - Library for configuring the Triad Semiconductor TS4231 Light
               to Digital converter.
    Created by: John Seibel
*******************************************************************/

#ifndef ts4231_h
#define ts4231_h

#include <stdint.h>
#include <stdbool.h>

#define BUS_DRV_DLY     1       //delay in microseconds between bus level changes
#define BUS_CHECK_DLY   500     //delay in microseconds for the checkBus() function
#define SLEEP_RECOVERY  100     //delay in microseconds for analog wake-up after exiting SLEEP mode
#define UNKNOWN_STATE   0x04    //checkBus() function state
#define S3_STATE        0x03    //checkBus() function state
#define WATCH_STATE     0x02    //checkBus() function state
#define SLEEP_STATE     0x01    //checkBus() function state
#define S0_STATE        0x00    //checkBus() function state
#define CFG_WORD        0x392B  //configuration value
#define BUS_FAIL        0x01    //ts_configDevice() function status return value
#define VERIFY_FAIL     0x02    //ts_configDevice() function status return value
#define WATCH_FAIL      0x03    //ts_configDevice() function status return value
#define CONFIG_PASS     0x04    //ts_configDevice() function status return value

bool ts_waitForLight(uint8_t index, uint16_t light_timeout);  //timeout in milliseconds
void ts_delayUs(unsigned int delay_val);  //delay in microseconds
uint8_t ts_configDevice(uint8_t index, uint16_t config_val);
bool ts_goToWatch(uint8_t index);
void ts_goToSleep(uint8_t index);
void ts_wakeUp(uint8_t index);
void ts_writeConfig(uint8_t index, uint16_t config_val);
uint16_t ts_readConfig(uint8_t index);
uint8_t ts_checkBus(uint8_t index);

void IRn_conf_d_oe(uint8_t, uint8_t, uint32_t *);
void IRn_conf_e_oe(uint8_t, uint8_t, uint32_t *);
void IRn_conf_d_w(uint8_t, uint8_t, uint32_t *);
void IRn_conf_e_w(uint8_t, uint8_t, uint32_t *);

#include <generated/csr.h>

#define IR_CONF_DELTA ((CSR_IR_RX0_CONF_ADDR) - (CSR_IR_RX0_BASE))

#ifdef CSR_IR_RX1_BASE
#define CSR_IR1_PRESENT 1

#define IR_BASE_DELTA ((CSR_IR_RX1_BASE) - (CSR_IR_RX0_BASE))

_Static_assert((CSR_IR_RX1_CONF_ADDR) - (CSR_IR_RX1_BASE) == IR_CONF_DELTA,
                  "IR1 oe address not aligned");

#else
#define CSR_IR1_PRESENT 0
#define IR_BASE_DELTA 0
#endif


#ifdef CSR_IR_RX2_BASE
#define CSR_IR2_PRESENT 1

_Static_assert((CSR_IR_RX2_BASE) - (CSR_IR_RX1_BASE) == IR_BASE_DELTA,
                  "IR2 base address not aligned");

_Static_assert((CSR_IR_RX2_CONF_ADDR) - (CSR_IR_RX2_BASE) == IR_CONF_DELTA,
                  "IR2 oe address not aligned");

#else
#define CSR_IR2_PRESENT 0
#endif


#ifdef CSR_IR_RX3_BASE
#define CSR_IR3_PRESENT 1

_Static_assert((CSR_IR_RX3_BASE) - (CSR_IR_RX2_BASE) == IR_BASE_DELTA,
                  "IR3 base address not aligned");

_Static_assert((CSR_IR_RX3_CONF_ADDR) - (CSR_IR_RX3_BASE) == IR_CONF_DELTA,
                  "IR3 oe address not aligned");

#else
#define CSR_IR3_PRESENT 0
#endif

#define IR_COUNT 2 + CSR_IR2_PRESENT + CSR_IR3_PRESENT

#endif /* ts4231_h */
