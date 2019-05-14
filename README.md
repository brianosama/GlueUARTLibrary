# GlueUARTLibrary
A replacement ADUCM3029 UART library for ADI implemented UART Library. Uses interrupts and circular buffers to receive and transmit data through UART. 

# Library API

bool UART_Init();
int UART_Putch(uint8_t buff);
int UART_Puts(uint8_t *buff);
uint8_t UART_Getch();
uint8_t UART_Gets(uint8_t *buff);
bool UART_Rxbuff_Avail();
void UART_Flush_Rx();
void UART_Flush_Tx();

# Code Reference
The library is a port and reimplementation of the Interrupt-based Serial Routines for PIC Microcontroller by Regulus Berdin (aka zer0w1ng). 

Discussion and sample code about the PIC implementation:
http://www.elab.ph/forum/index.php?topic=3270.0
