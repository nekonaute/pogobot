/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pogobot_ir.h"
#include "ts4231.h"
#include "ir_uart.h"

#ifdef CSR_IR_RX0_BASE

/* clang-format-ok */

uint8_t target_ir_bit_mask = IR_RX_BITMASK_ALL;

/** implicitly obeys target_ir_bit_mask */
uint8_t
ir_init( void )
{
    //printf( "ir_init:\n" );
    uint8_t bit_mask = 0;
    ir_uart_word_t msg = 42;

    for ( int ir_i = 0; ir_i < IR_RX_COUNT; ir_i++ )
    {
        if ( ( ( target_ir_bit_mask >> ir_i ) & 1 ) == 0 )
        {
            continue;
        }

        IRn_tx_write_msg( 1<<ir_i, &msg, 1);

        uint8_t result = ts_configDevice( ir_i, CFG_WORD );
        if ( result == 4 )
        {
            bit_mask |= 1 << ir_i;
        }
    }
    /*printf( "mask=0x%x ideal=0x%x %s\n", bit_mask, IR_RX_BITMASK_ALL,
            ( bit_mask == IR_RX_BITMASK_ALL )
                ? "PERFECT"
                : "WARN: some non-functional IR" );
	*/			
    return bit_mask;
}

#define TIMEOUT 10000
uint8_t
IRn_tx_write_msg( uint8_t mask, ir_uart_word_t * msg, size_t length)
{
    uint32_t i=0;
    ir_tx_conf_tx_mask_write(mask);
	while( ( ir_tx_txempty_read() != 1 ) && (ir_tx_rts_read() == 1)) {    // Wait until FIFO is empty and IR not receiving
        i++;
        if( i == TIMEOUT ) {
            return -1;
        }
    }
    for(i=0; i<length; i++)
    {
        ir_tx_tx_write(msg[i]);             // Directly write to the FIFO
    }
    ir_tx_conf_tx_trig_write(1);            // Trigger transmission
    return 0;
}

void
IRn_conf_d_oe( uint8_t id, uint8_t val, uint32_t *p_oldconf )
{
    uint32_t v = val << CSR_IR_RX0_CONF_D_OE_OFFSET;
    uint32_t mask = 1 << CSR_IR_RX0_CONF_D_OE_OFFSET;
    *p_oldconf = ( *p_oldconf & ~mask ) | v;
    csr_write_simple( *p_oldconf, CSR_IR_RX0_CONF_ADDR + id * IR_CONF_ADDR_DELTA );
}

void
IRn_conf_e_oe( uint8_t id, uint8_t val, uint32_t *p_oldconf )
{
    uint32_t v = val << CSR_IR_RX0_CONF_E_OE_OFFSET;
    uint32_t mask = 1 << CSR_IR_RX0_CONF_E_OE_OFFSET;
    *p_oldconf = ( *p_oldconf & ~mask ) | v;
    csr_write_simple( *p_oldconf, CSR_IR_RX0_CONF_ADDR + id * IR_CONF_ADDR_DELTA );
}

void
IRn_conf_d_w( uint8_t id, uint8_t val, uint32_t *p_oldconf )
{
    uint32_t v = val << CSR_IR_RX0_CONF_D_W_OFFSET;
    uint32_t mask = 1 << CSR_IR_RX0_CONF_D_W_OFFSET;
    *p_oldconf = ( *p_oldconf & ~mask ) | v;
    csr_write_simple( *p_oldconf, CSR_IR_RX0_CONF_ADDR + id * IR_CONF_ADDR_DELTA );
}

void
IRn_conf_e_w( uint8_t id, uint8_t val, uint32_t *p_oldconf )
{
    uint32_t v = val << CSR_IR_RX0_CONF_E_W_OFFSET;
    uint32_t mask = 1 << CSR_IR_RX0_CONF_E_W_OFFSET;
    *p_oldconf = ( *p_oldconf & ~mask ) | v;
    csr_write_simple( *p_oldconf, CSR_IR_RX0_CONF_ADDR + id * IR_CONF_ADDR_DELTA );
}

void
IRn_conf_tx_power_write( int index, int power )
{
    // TX power is stored for each IR, so it's in each RX module
    uint32_t oldword = IRn_rx_conf_read( index );
    uint32_t newword = ir_rx0_conf_tx_power_replace( oldword, power );
    IRn_rx_conf_write( index, newword );
}

#else /* CSR_IR_RX0_CONF_ADDR */

#warning no IR, skipping IR software

#endif /* CSR_IR_RX0_CONF_ADDR */
