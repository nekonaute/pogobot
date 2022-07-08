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
//#include <sysctl.h>
#include "ts4231.h"
#include <stdio.h>
#include <stdbool.h>

//IMPORTANT NOTES:
//1)  If porting the TS4231 library code to a non-Arduino architecture,
//    be sure that the INPUT ports assigned to the E and D signals are configured as
//    floating inputs with NO pull-up or pull-down function.  Using a pull-up or
//    pull-down function on the inputs will cause the TS4231 to operate incorrectly.
//2)  The TS4231 library omits delays between E and D signal transitions when going
//    from S3_STATE to WATCH_STATE or SLEEP_STATE to WATCH_STATE in function
//    goToWatch() for the purpose of transitioning into WATCH_STATE as quickly as
//    possible.  If a microcontroller is being used that can change states on
//    the E and D outputs faster than approximately 100ns, the TS4231 datasheet
//    must be consulted to verify timing parameters are not being violated to
//    assure proper TS4231 operation.  A suitable solution would be to include
//    a short delay in function ts_digitalWrite() to allow enough time between
//    output pin signal changes to meet the TS4231 timing parameters as stated
//    in the datasheet.  See the ts_digitalWrite() function for more information.

#include <generated/csr.h>

#ifdef CSR_IR_RX0_BASE

#include <pogobot_ir.h>

#include <sleep.h>

//bool configured_state[IR_CONF_COUNT];

bool ts_goToWatch(uint8_t index) {
    bool watch_success;
    uint32_t oldconf = 0;
    // Read register value :
    oldconf = csr_read_simple(CSR_IR_RX0_CONF_ADDR + index * IR_CONF_ADDR_DELTA );
    switch (ts_checkBus(index)) {
        case S0_STATE:
            watch_success = false;
            break;
        case SLEEP_STATE:
            IRn_conf_d_w(index, 1, &oldconf);     // D HIGH
            IRn_conf_d_oe(index, 1, &oldconf);    // D Output
            //usleep(BUS_DRV_DLY);
            IRn_conf_e_w(index, 0, &oldconf);     // E LOW
            IRn_conf_e_oe(index, 1, &oldconf);    // E Output
            //usleep(BUS_DRV_DLY);
            IRn_conf_d_w(index, 0, &oldconf);     // D LOW
            //usleep(BUS_DRV_DLY);
            IRn_conf_d_oe(index, 0, &oldconf);    // D Hi-Z
            //usleep(BUS_DRV_DLY);
            IRn_conf_e_w(index, 1, &oldconf);     // E HIGH
            //usleep(BUS_DRV_DLY);
            IRn_conf_e_oe(index, 0, &oldconf);    // E Hi-Z
            usleep(SLEEP_RECOVERY);
            if (ts_checkBus(index) == WATCH_STATE) watch_success = true;
            else watch_success = false;
            break;
        case WATCH_STATE:
            watch_success = true;
            break;
        case S3_STATE:
            IRn_conf_e_w(index, 1, &oldconf);     // E HIGH
            IRn_conf_e_oe(index, 1, &oldconf);    // E Output
            //usleep(BUS_DRV_DLY);
            IRn_conf_d_w(index, 1, &oldconf);     // D HIGH
            IRn_conf_d_oe(index, 1, &oldconf);    // D Output
            //usleep(BUS_DRV_DLY);
            IRn_conf_e_w(index, 0, &oldconf);     // E LOW
            //usleep(BUS_DRV_DLY);
            IRn_conf_d_w(index, 0, &oldconf);     // D LOW
            //usleep(BUS_DRV_DLY);
            IRn_conf_e_w(index, 1, &oldconf);     // E HIGH
            IRn_conf_d_oe(index, 0, &oldconf);    // D Hi-Z
            IRn_conf_e_oe(index, 0, &oldconf);    // E Hi-Z
            //usleep(SLEEP_RECOVERY);
            if (ts_checkBus(index) == WATCH_STATE) watch_success = true;
            else watch_success = false;
            break;
        default:
            watch_success = false;
            break;
    }

    return watch_success;
}


uint16_t ts_readConfig(uint8_t index) {
    uint16_t readback;
    uint32_t oldconf = 0;

    // Read register value :
    oldconf = csr_read_simple(CSR_IR_RX0_CONF_ADDR + index * IR_CONF_ADDR_DELTA );

    readback = 0x0000;
    IRn_conf_e_w(index, 1, &oldconf);     // E HIGH
    IRn_conf_d_w(index, 1, &oldconf);     // D HIGH
    IRn_conf_e_oe(index, 1, &oldconf);    // E Output
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_oe(index, 1, &oldconf);    // D Output
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_w(index, 0, &oldconf);     // D LOW
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 0, &oldconf);     // E LOW
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_w(index, 1, &oldconf);     // D HIGH
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 1, &oldconf);     // E HIGH
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_oe(index, 0, &oldconf);    // D Hi-Z
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 0, &oldconf);     // E LOW
    for (uint8_t i = 0; i < 14; i++) {
        //usleep(BUS_DRV_DLY);
        IRn_conf_e_w(index, 1, &oldconf);   // E HIGH
        readback = (readback << 1) | ir_rx0_in_d_r_extract(IRn_rx_in_read(index));
        //usleep(BUS_DRV_DLY);
        IRn_conf_e_w(index, 0, &oldconf);   // E LOW
    }
    IRn_conf_d_w(index, 0, &oldconf);   // D LOW
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_oe(index, 1, &oldconf);  // D Output
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 1, &oldconf);   // E HIGH
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_w(index, 1, &oldconf);   // D HIGH
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_oe(index, 0, &oldconf);   // E Hi-Z
    IRn_conf_d_oe(index, 0, &oldconf);   // D Hi-Z
    return readback;
}


//checkBus() performs a voting function where the bus is sampled 3 times
//to find 2 identical results.  This is necessary since light detection is
//asynchronous and can indicate a false state.
uint8_t ts_checkBus(uint8_t index) {
    uint8_t state;
    uint8_t E_state;
    uint8_t D_state;
    uint8_t S0_count = 0;
    uint8_t SLEEP_count = 0;
    uint8_t WATCH_count = 0;
    uint8_t S3_count = 0;
    uint32_t IRn_IN;

    for (uint8_t i=0; i<3; i++) {
        IRn_IN = IRn_rx_in_read(index);     // Read the pins' status
        E_state = ir_rx0_in_e_r_extract(IRn_IN);
        D_state = ir_rx0_in_d_r_extract(IRn_IN);
        if (D_state == 1) {
            if (E_state == 1) S3_count++;
            else SLEEP_count++;
        }
        else {
            if (E_state == 1) WATCH_count++;
            else S0_count++;
        }
        usleep(BUS_CHECK_DLY);
    }
    if (SLEEP_count >= 2) state = SLEEP_STATE;
    else if (WATCH_count >= 2) state = WATCH_STATE;
    else if (S3_count >= 2) state = S3_STATE;
    else if (S0_count >= 2) state = S0_STATE;
    else state = UNKNOWN_STATE;

    return state;
}


uint8_t ts_configDevice(uint8_t index, uint16_t config_val) {
    uint8_t config_success = 0x00;
    uint32_t oldconf = 0;
    uint16_t readback;

    // Read register value :
    oldconf = csr_read_simple(CSR_IR_RX0_CONF_ADDR + index * IR_CONF_ADDR_DELTA );
    IRn_conf_d_oe(index, 0, &oldconf);    // D Hi-Z
    IRn_conf_e_oe(index, 0, &oldconf);    // E Hi-Z
    IRn_conf_d_w(index, 0, &oldconf);     // D LOW (but still Hi-Z)
    IRn_conf_e_w(index, 1, &oldconf);     // E HIGH (but still Hi-Z)
    IRn_conf_e_oe(index, 1, &oldconf);    // E Really HIGH now
    usleep(BUS_DRV_DLY);
    /* Next commands useless? */
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 0, &oldconf);     // E LOW
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 1, &oldconf);     // E HIGH
    /* end useless block */
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_oe(index, 1, &oldconf);    // D really LOW
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_w(index, 1, &oldconf);     // D HIGH (E Still HIGH)
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_oe(index, 0, &oldconf);    // D Hi-Z
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_oe(index, 0, &oldconf);    // E Hi-Z too, Should be in config-mode = S3-State, both D and E high
    if (ts_checkBus(index) == S3_STATE) {
        ts_writeConfig(index, config_val);
        readback = ts_readConfig(index);
        if (readback == config_val) {
            if (ts_goToWatch(index)) config_success = CONFIG_PASS;
            else config_success = WATCH_FAIL;
        }
        else config_success = VERIFY_FAIL;
    }
    else config_success = BUS_FAIL;
    return config_success;
}

void ts_writeConfig(uint8_t index, uint16_t config_val) {
    uint32_t oldconf = 0;
    // Read register value :
    oldconf = csr_read_simple(CSR_IR_RX0_CONF_ADDR + index * IR_CONF_ADDR_DELTA );
    IRn_conf_d_w(index, 1, &oldconf);   // D high
    IRn_conf_e_w(index, 1, &oldconf);   // E high (but still in Hi-Z)
    IRn_conf_e_oe(index, 1, &oldconf);  // E Output enabled
    IRn_conf_d_oe(index, 1, &oldconf);  // D Output Enabled
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_w(index, 0, &oldconf);   // D Low // Write mode
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 0, &oldconf);   // E Low

    for (uint8_t i = 0; i < 15; i++) {
        config_val = config_val << 1;
        if ((config_val & 0x8000) > 0) IRn_conf_d_w(index, 1, &oldconf);  // D high
        else IRn_conf_d_w(index, 0, &oldconf);  // D low
        //usleep(BUS_DRV_DLY);
        IRn_conf_e_w(index, 1, &oldconf); // E HIGH
        //usleep(BUS_DRV_DLY);
        IRn_conf_e_w(index, 0, &oldconf); // E LOW
        //usleep(BUS_DRV_DLY);
    }
    //IRn_conf_e_w(index, 0, &oldconf); // E Low
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_w(index, 0, &oldconf); // D Low
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_w(index, 1, &oldconf); // E high
    //usleep(BUS_DRV_DLY);
    IRn_conf_d_w(index, 1, &oldconf); // D high 
    //usleep(BUS_DRV_DLY);
    IRn_conf_e_oe(index, 0, &oldconf);    // E Hi-Z 
    IRn_conf_d_oe(index, 0, &oldconf);    // D Hi-Z
}

#else /* CSR_IR_RX0_CONF_ADDR */

#warning no IR, skipping IR software

#endif /* CSR_IR_RX0_CONF_ADDR */
