/*
 * glue_uart.h
 *
 *  Created on: 12 May 2019
 *      Author: t0rp35t
 */

#ifndef GLUE_UART_H_
#define GLUE_UART_H_

#include <drivers/uart/adi_uart.h>

#define ADI_UART_MEMORY_SIZE    (ADI_UART_BIDIR_MEMORY_SIZE)
#define UART_DEVICE_NUM         0

#define UART_RX_BUFFER_SIZE     32
#define UART_RX_BUFFER_MASK     (UART_RX_BUFFER_SIZE-1)

#define UART_TX_BUFFER_SIZE     32
#define UART_TX_BUFFER_MASK     (UART_TX_BUFFER_SIZE-1)

// setting for 26MHz PCLK
#define UART0_DIV               7
#define UART0_DIVM              2
#define UART0_DIVN              31
#define UART0_OSR               2

bool UART_Init();

int UART_Putch(uint8_t buff);
int UART_Puts(uint8_t *buff);

uint8_t UART_Getch();
uint8_t UART_Gets(uint8_t *buff);

bool UART_Rxbuff_Avail();

void UART_Flush_Rx();
void UART_Flush_Tx();

#endif /* GLUE_UART_H_ */
