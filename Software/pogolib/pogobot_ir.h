/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#ifndef __IR_MULTI_H__
#define __IR_MULTI_H__

/* clang-format-ok */

#include <generated/csr.h>

#ifdef CSR_IR_RX0_BASE

#include <stdint.h>
#include <stdio.h> // For size_t

extern uint8_t target_ir_bit_mask;

uint8_t ir_init( void );

#define UART_EV_TX 0x1
#define UART_EV_RX 0x2

//#define FIFO_SIZE 128 // Size of IR_UART RX FIFO (defined in litex)

// 8-bit packets: uint8_t and 2 nibbles
// typedef uint8_t uart_word_t;
//#define UART_WORD_NIBBLES 2

// 20-bit packets: uint32_t and 5 nibbles
//typedef uint32_t uart_word_t;
typedef uint8_t ir_uart_word_t;
#define UART_WORD_NIBBLES 6

#define UART_WORD_BITS ( UART_WORD_NIBBLES << 2 )
#define UART_WORD_MASK ( ( 1 << UART_WORD_BITS ) - 1 )

#ifdef CSR_IR_RX0_BASE
#define CSR_IR0_PRESENT 1
#else
#define CSR_IR0_PRESENT 0
#endif

#ifdef CSR_IR_RX1_BASE
#define CSR_IR1_PRESENT 1

#define IR_RX_BASE_DELTA ( (CSR_IR_RX1_BASE) - (CSR_IR_RX0_BASE) )
#define IR_CONF_ADDR_DELTA ( (CSR_IR_RX1_CONF_ADDR) - (CSR_IR_RX0_CONF_ADDR) )

#else
#define CSR_IR1_PRESENT    0
#define IR_RX_BASE_DELTA 0
#define IR_CONF_ADDR_DELTA 0
#endif

#ifdef CSR_IR_RX2_BASE
#define CSR_IR2_PRESENT 1

_Static_assert( (CSR_IR_RX2_BASE) - (CSR_IR_RX1_BASE) == IR_RX_BASE_DELTA,
                "IR2 base address not aligned" );

#else
#define CSR_IR2_PRESENT 0
#endif

#ifdef CSR_IR_RX3_BASE
#define CSR_IR3_PRESENT 1

_Static_assert( (CSR_IR_RX3_BASE) - (CSR_IR_RX2_BASE) == IR_RX_BASE_DELTA,
                "IR3 base address not aligned" );

#else
#define CSR_IR3_PRESENT 0
#endif

#define IR_RX_COUNT                                                          \
    ( CSR_IR0_PRESENT + CSR_IR1_PRESENT + CSR_IR2_PRESENT + CSR_IR3_PRESENT )

#define IR_RX_BITMASK_ALL ( ( 1 << ( IR_RX_COUNT ) ) - 1 )

void IRn_conf_d_oe( uint8_t id, uint8_t val, uint32_t *p_oldconf );
void IRn_conf_e_oe( uint8_t id, uint8_t val, uint32_t *p_oldconf );
void IRn_conf_d_w( uint8_t id, uint8_t val, uint32_t *p_oldconf );
void IRn_conf_e_w( uint8_t id, uint8_t val, uint32_t *p_oldconf );
uint8_t IRn_tx_write_msg( uint8_t mask, ir_uart_word_t * msg, size_t length);
void IRn_conf_tx_power_write( int index, int power );
#define IRn_uart_ev_pending_write( index, value ) csr_write_simple( value,  CSR_IR_RX0_EV_PENDING_ADDR + index * IR_RX_BASE_DELTA)
#define IRn_rx_conf_write( index, value ) csr_write_simple( value, CSR_IR_RX0_CONF_ADDR + index * ( IR_RX_BASE_DELTA ) )
#define IRn_rx_conf_read( index ) csr_read_simple( CSR_IR_RX0_CONF_ADDR + index * ( IR_RX_BASE_DELTA ) )
#define IRn_rx_in_read( index ) csr_read_simple( CSR_IR_RX0_IN_ADDR + index * ( IR_RX_BASE_DELTA ) )
#define IRn_rx_rxempty_read( index ) csr_read_simple( CSR_IR_RX0_RXEMPTY_ADDR + index * IR_RX_BASE_DELTA )
#define IRn_rx_read( index ) csr_read_simple( CSR_IR_RX0_BASE + index * IR_RX_BASE_DELTA  )

#else /* CSR_IR_RX0_BASE */

#warning no IR, skipping IR software

#endif /* CSR_IR_RX0_BASE */

#endif /* __IR_MULTI_H__ */
