/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include <generated/csr.h>
#include <irq.h>
#include <pogobot_ir.h>

#ifdef CSR_IR_TX_BASE
#include <ir_uart.h>

#undef UART_EV_RX
#define UART_EV_RX 1

/*
 * Buffer sizes must be a power of 2 so that modulos can be computed
 * with logical AND.
 */

//#define UART_POLLING

#ifndef UART_POLLING

#define UART_RINGBUFFER_SIZE_RX 512
#define UART_RINGBUFFER_MASK_RX (UART_RINGBUFFER_SIZE_RX-1)

static char rx_buf[IR_NUMBER][UART_RINGBUFFER_SIZE_RX];
static volatile unsigned int rx_produce[IR_NUMBER];
static unsigned int rx_consume[IR_NUMBER];

#define UART_RINGBUFFER_SIZE_TX 512
#define UART_RINGBUFFER_MASK_TX (UART_RINGBUFFER_SIZE_TX-1)

/*static char tx_buf[UART_RINGBUFFER_SIZE_TX];
static unsigned int tx_produce;
static volatile unsigned int tx_consume; */

/*
 * static const char * REBOOT_KEY = "REBOOT_ME_PLEASE";
 * #define KEYLEN 16
 * static uint8_t reboot_ptr[IR_NUMBER];
*/
void ir_uart_rx_isr(void)
{
	unsigned int stat[IR_NUMBER] , rx_produce_next[IR_NUMBER];

    stat[0] = ir_rx0_ev_pending_read();
	if(stat[0] & UART_EV_RX) {
		while(!ir_rx0_rxempty_read()) {
			rx_produce_next[0] = (rx_produce[0] + 1) & UART_RINGBUFFER_MASK_RX;
			if(rx_produce_next[0] != rx_consume[0]) {
				rx_buf[0][rx_produce[0]] = ir_rx0_rx_read();
                /* reboot block */
                /*if( rx_buf[0][rx_produce[0]] == REBOOT_KEY[reboot_ptr[0]]) {
                    reboot_ptr[0]++;
                    if (reboot_ptr[0] == KEYLEN) {
                        reboot_ctrl_write(0xac);
                    }
                }
                else {
                    reboot_ptr[0] = 0;
                }*/ /* end reboot block */
				rx_produce[0] = rx_produce_next[0];

			}
			ir_rx0_ev_pending_write(UART_EV_RX);
		}
	}
#ifdef CSR_IR_RX1_BASE
    stat[1] = ir_rx1_ev_pending_read();
	if(stat[1] & UART_EV_RX) {
		while(!ir_rx1_rxempty_read()) {
			rx_produce_next[1] = (rx_produce[1] + 1) & UART_RINGBUFFER_MASK_RX;
			if(rx_produce_next[1] != rx_consume[1]) {
				rx_buf[1][rx_produce[1]] = ir_rx1_rx_read();
                /* reboot block */
                /*if( rx_buf[1][rx_produce[1]] == REBOOT_KEY[reboot_ptr[1]]) {
                    reboot_ptr[1]++;
                    if (reboot_ptr[1] == KEYLEN) {
                        reboot_ctrl_write(0xac);
                    }
                }
                else {
                    reboot_ptr[1] = 0;
                }*/ /* end reboot block */
				rx_produce[1] = rx_produce_next[1];
			}
			ir_rx1_ev_pending_write(UART_EV_RX);
		}
	}
#endif
#ifdef CSR_IR_RX2_BASE
    stat[2] = ir_rx2_ev_pending_read();
	if(stat[2] & UART_EV_RX) {
		while(!ir_rx2_rxempty_read()) {
			rx_produce_next[2] = (rx_produce[2] + 1) & UART_RINGBUFFER_MASK_RX;
			if(rx_produce_next[2] != rx_consume[2]) {
				rx_buf[2][rx_produce[2]] = ir_rx2_rx_read();
                /* reboot block */
                /*if( rx_buf[2][rx_produce[2]] == REBOOT_KEY[reboot_ptr[2]]) {
                    reboot_ptr[2]++;
                    if (reboot_ptr[2] == KEYLEN) {
                        reboot_ctrl_write(0xac);
                    }
                }
                else {
                    reboot_ptr[2] = 0;
                }*/ /* end reboot block */
				rx_produce[2] = rx_produce_next[2];
			}
			ir_rx2_ev_pending_write(UART_EV_RX);
		}
	}
#endif
#ifdef CSR_IR_RX3_BASE
    stat[3] = ir_rx3_ev_pending_read();
	if(stat[3] & UART_EV_RX) {
		while(!ir_rx3_rxempty_read()) {
			rx_produce_next[3] = (rx_produce[3] + 1) & UART_RINGBUFFER_MASK_RX;
			if(rx_produce_next[3] != rx_consume[3]) {
				rx_buf[3][rx_produce[3]] = ir_rx3_rx_read();
                /* reboot block */
                /*if( rx_buf[3][rx_produce[3]] == REBOOT_KEY[reboot_ptr[3]]) {
                    reboot_ptr[3]++;
                    if (reboot_ptr[3] == KEYLEN) {
                        reboot_ctrl_write(0xac);
                    }
                }
                else {
                    reboot_ptr[3] = 0;
                }*/ /* end reboot block */
				rx_produce[3] = rx_produce_next[3];
			}
			ir_rx3_ev_pending_write(UART_EV_RX);
		}
	}
#endif
}

/*void ir_uart_tx_isr(void)
{
	unsigned int stat;

	stat = ir_tx_ev_pending_read();
	if(stat & UART_EV_TX) {
		ir_tx_ev_pending_write(UART_EV_TX);
		while((tx_consume != tx_produce) && !ir_tx_txfull_read()) {
			ir_tx_tx_write(tx_buf[tx_consume]);
			tx_consume = (tx_consume + 1) & UART_RINGBUFFER_MASK_TX;
		}
	}
}*/

/* Do not use in interrupt handlers! */
char ir_uart_read(uint8_t channel)
{
	char c;
    if(channel > IR_NUMBER)
	{
        return 0;
	}

	if(irq_getie()) {
		while(rx_consume[channel] == rx_produce[channel]);
	} else if (rx_consume[channel] == rx_produce[channel]) {
		return 0;
	}

	c = rx_buf[channel][rx_consume[channel]];
	rx_consume[channel] = (rx_consume[channel] + 1) & UART_RINGBUFFER_MASK_RX;
	return c;
}

int ir_uart_read_nonblock(uint8_t channel)
{
    if(channel > IR_NUMBER)
	{
        return 0;
	} else {
		return (rx_consume[channel] != rx_produce[channel]);
	}
	
}

/*void ir_uart_write(char c)
{
	unsigned int oldmask;
	unsigned int tx_produce_next = (tx_produce + 1) & UART_RINGBUFFER_MASK_TX;

	if(irq_getie()) {
		while(tx_produce_next == tx_consume);
	} else if(tx_produce_next == tx_consume) {
		return;
	}

	oldmask = irq_getmask();
	irq_setmask(oldmask & ~(1 << IR_TX_INTERRUPT));
	if((tx_consume != tx_produce) || ir_tx_txfull_read()) {
		tx_buf[tx_produce] = c;
		tx_produce = tx_produce_next;
	} else {
		ir_tx_tx_write(c);
	}
	irq_setmask(oldmask);
}*/

void ir_uart_init(void)
{
    for( uint8_t i=0; i<IR_NUMBER ;i++ )
    {
        rx_produce[i] = 0;
        rx_consume[i] = 0;
        //reboot_ptr[i] = 0;
    }
	//tx_produce = 0;
	//tx_consume = 0;
#ifdef CSR_IR_RX0_BASE
	//ir_tx_ev_pending_write(ir_tx_ev_pending_read());
    //ir_tx_ev_enable_write(UART_EV_TX);
    ir_rx0_ev_pending_write(ir_rx0_ev_pending_read()); 
	ir_rx0_ev_enable_write(UART_EV_RX);
#endif
#ifdef CSR_IR_RX1_BASE
    ir_rx1_ev_pending_write(ir_rx1_ev_pending_read()); 
	ir_rx1_ev_enable_write(UART_EV_RX);
#endif
#ifdef CSR_IR_RX2_BASE
    ir_rx2_ev_pending_write(ir_rx2_ev_pending_read()); 
	ir_rx2_ev_enable_write(UART_EV_RX);
#endif
#ifdef CSR_IR_RX3_BASE
    ir_rx3_ev_pending_write(ir_rx3_ev_pending_read()); 
	ir_rx3_ev_enable_write(UART_EV_RX);
#endif
	irq_setmask(irq_getmask() | (1 << IR_RX0_INTERRUPT) | (1 << IR_RX1_INTERRUPT) | (1 << IR_RX2_INTERRUPT) | (1 << IR_RX3_INTERRUPT));
}

void ir_uart_sync(void)
{
}

#else // Polling mode
/* TODO */
void ir_uart_isr(void);
void ir_uart_isr(void)
{
}

char ir_uart_read(uint8_t channel)
{
	char c;
    while( IRn_rx_rxempty_read(channel));
    c = IRn_rx_read(channel);
    IRn_uart_ev_pending_write(channel,UART_EV_RX);
    return c;
}

int ir_uart_read_nonblock(uint8_t channel)
{
	return (IRn_rx_rxempty_read(channel) == 0);
}

void ir_uart_init(void)
{
#ifdef CSR_IR_RX0_BASE
    ir_rx0_ev_pending_write(ir_rx0_ev_pending_read()); 
	ir_rx0_ev_enable_write(UART_EV_RX);
#endif
#ifdef CSR_IR_RX1_BASE
    ir_rx1_ev_pending_write(ir_rx1_ev_pending_read()); 
	ir_rx1_ev_enable_write(UART_EV_RX);
#endif
#ifdef CSR_IR_RX2_BASE
    ir_rx2_ev_pending_write(ir_rx2_ev_pending_read()); 
	ir_rx2_ev_enable_write(UART_EV_RX);
#endif
#ifdef CSR_IR_RX3_BASE
    ir_rx3_ev_pending_write(ir_rx3_ev_pending_read()); 
	ir_rx3_ev_enable_write(UART_EV_RX);
#endif
}

void ir_uart_sync(void)
{
	while( ir_tx_txfull_read() );
}

#endif // UART_POLLING
#endif // CSR_IR_TX_BASE

