/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#undef INTERFACE
#if defined(CSR_IR_TX_BASE)
void ir_uart_sync(void);
void ir_uart_init(void);
void ir_uart_write(char c);
int ir_uart_read_nonblock(uint8_t);
char ir_uart_read(uint8_t);
void ir_uart_tx_isr(void);
void ir_uart_rx_isr(void);
#endif

#ifdef CSR_IR_RX3_BASE
#define IR_NUMBER 4
#elif defined CSR_IR_RX2_BASE
#define IR_NUMBER 3
#elif defined CSR_IR_RX1_BASE
#define IR_NUMBER 2
#elif defined CSR_IR_RX0_BASE
#define IR_NUMBER 1
#else
#define IR_NUMBER 0
#endif

#ifndef IR_RX1_INTERRUPT
#define IR_RX1_INTERRUPT 0
#endif
#ifndef IR_RX2_INTERRUPT
#define IR_RX2_INTERRUPT 0
#endif
#ifndef IR_RX3_INTERRUPT
#define IR_RX3_INTERRUPT 0
#endif


