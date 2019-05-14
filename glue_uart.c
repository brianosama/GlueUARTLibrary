/*
 * glue_uart.c
 *
 *  Created on: 12 May 2019
 *      Author: t0rp35t
 */

#include "glue_uart.h"
#include <stdlib.h>

static uint8_t UART_RX_Buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t UART_RX_Buffer_headPtr, UART_RX_Buffer_tailPnt;

static uint8_t UART_TX_Buffer[UART_TX_BUFFER_SIZE];
static volatile uint16_t UART_TX_Buffer_headPnt, UART_TX_Buffer_tailPnt;

#define UART0_TX_PORTP0_MUX  ((uint32_t) ((uint32_t) 1<<20))
#define UART0_RX_PORTP0_MUX  ((uint32_t) ((uint32_t) 1<<22))

extern void UART_Int_Handler(void);

bool UART_Init() {
  // pin mux
  uint32_t gpio0_cfg = *pREG_GPIO0_CFG;
  gpio0_cfg &= ~(BITM_GPIO_CFG_PIN10 | BITM_GPIO_CFG_PIN11);
  gpio0_cfg |= UART0_TX_PORTP0_MUX | UART0_RX_PORTP0_MUX;
  *pREG_GPIO0_CFG = gpio0_cfg;

  // set 8n1
  *pREG_UART0_LCR = (3 << BITP_UART_LCR_WLS) | (0 << BITP_UART_LCR_STOP)
      | (0 << BITP_UART_LCR_PEN);

  // set baud rate
  *pREG_UART0_DIV = UART0_DIV;
  *pREG_UART0_FBR = (1 << BITP_UART_COMFBR_FBEN)
      | (UART0_DIVM << BITP_UART_COMFBR_DIVM)
      | (UART0_DIVN << BITP_UART_COMFBR_DIVN);

  *pREG_UART0_LCR2 = (UART0_OSR << BITP_UART_LCR2_OSR);

  // set FIFO settings
  *pREG_UART0_FCR = (1 << BITP_UART_FCR_FIFOEN) | (3 << BITP_UART_FCR_RFTRIG)
      | (3 << BITP_UART_FCR_RFCLR);

  //set uart interrupt
  *pREG_UART0_IEN = (1 << BITP_UART_IEN_ERBFI) | (1 << BITP_UART_IEN_ELSI);

  NVIC_ClearPendingIRQ(UART_EVT_IRQn);
  NVIC_EnableIRQ(UART_EVT_IRQn);

  UART_RX_Buffer_headPtr = 0;
  UART_RX_Buffer_tailPnt = 0;

  UART_TX_Buffer_headPnt = 0;
  UART_TX_Buffer_tailPnt = 0;

  return true;
}

int UART_Putch(uint8_t buff) {
  while (((UART_TX_Buffer_headPnt + 1) & (UART_TX_BUFFER_SIZE - 1))
      == UART_TX_Buffer_tailPnt)
    __WFI();

  UART_TX_Buffer[UART_TX_Buffer_headPnt] = buff;
  UART_TX_Buffer_headPnt = (UART_TX_Buffer_headPnt + 1) & (UART_TX_BUFFER_SIZE - 1);

  *pREG_UART0_IEN = *pREG_UART0_IEN | (1 << BITP_UART_IEN_ETBEI);

  return 0;
}

int UART_Puts(uint8_t *buff) {
  while (((UART_TX_Buffer_headPnt + 1) & (UART_TX_BUFFER_SIZE - 1))
      == UART_TX_Buffer_tailPnt)
    __WFI();

  while (*buff) {
    UART_TX_Buffer[UART_TX_Buffer_headPnt] = *buff++;
    UART_TX_Buffer_headPnt = (UART_TX_Buffer_headPnt + 1) & (UART_TX_BUFFER_SIZE - 1);
  }

  if ((*pREG_UART0_IEN & BITM_UART_IEN_ETBEI) == 0) {
    *pREG_UART0_IEN = *pREG_UART0_IEN | (1 << BITP_UART_IEN_ETBEI);
  }

  return 0;
}

uint8_t UART_Getch() {
  while (!UART_Rxbuff_Avail()) {
    __WFI();
  }

  uint8_t buff = UART_RX_Buffer[UART_RX_Buffer_tailPnt];
  UART_RX_Buffer_tailPnt = (UART_RX_Buffer_tailPnt + 1) & (UART_RX_BUFFER_SIZE - 1);

  return buff;
}

uint8_t UART_Gets(uint8_t *buff) {
  uint8_t count = 0, tmp;

  while (!UART_Rxbuff_Avail()) {
    __WFI();
  }

  do {
    tmp = UART_RX_Buffer[UART_RX_Buffer_tailPnt];
    UART_RX_Buffer[UART_RX_Buffer_tailPnt++] = 0;
    UART_RX_Buffer_tailPnt &= (UART_RX_BUFFER_SIZE - 1);

    *buff++ = tmp;
    ++count;
  } while (UART_Rxbuff_Avail() | tmp != 0);

  return count;
}

bool UART_Rxbuff_Avail() {
  return UART_RX_Buffer_headPtr != UART_RX_Buffer_tailPnt;
}

void UART_Flush_Rx() {
  memset(UART_RX_Buffer, 0, sizeof(UART_RX_Buffer));

  UART_RX_Buffer_headPtr = 0;
  UART_RX_Buffer_tailPnt = 0;
}

void UART_Flush_Tx() {
  memset(UART_TX_Buffer, 0, sizeof(UART_TX_Buffer));

  UART_TX_Buffer_headPnt = 0;
  UART_TX_Buffer_tailPnt = 0;
}

void UART_Int_Handler(void) {
  uint16_t int_status = *pREG_UART0_IIR;
  uint8_t count = 0;

  if ((int_status & BITM_UART_IIR_NIRQ) == 0) {   // uart interrupt
    int_status = (int_status & BITM_UART_IIR_STAT) >> BITP_UART_IIR_STAT;

    if ((int_status & 1) == 1) {                  // tx buffer empty
      if (UART_TX_Buffer_headPnt != UART_TX_Buffer_tailPnt) {
        count = 14;                               // TX FIFO depth
        while (count--) {
          *pREG_UART0_TX = UART_TX_Buffer[UART_TX_Buffer_tailPnt];

          UART_TX_Buffer_tailPnt = (UART_TX_Buffer_tailPnt + 1)
              & (UART_TX_BUFFER_SIZE - 1);

          if (UART_TX_Buffer_headPnt == UART_TX_Buffer_tailPnt) {
            break;
          }
        }

      } else {
        *pREG_UART0_IEN = *pREG_UART0_IEN & ~(1 << BITP_UART_IEN_ETBEI);
      }
    } else if ((int_status & 2) == 2) {          // rx buffer full or rx timeout
      count = *pREG_UART0_RFC;
      while (count--) {
        UART_RX_Buffer[UART_RX_Buffer_headPtr] = *pREG_UART0_RX;
        UART_RX_Buffer_headPtr = (UART_RX_Buffer_headPtr + 1) & (UART_RX_BUFFER_SIZE - 1);
      }
    } else {

    }
  }
}

