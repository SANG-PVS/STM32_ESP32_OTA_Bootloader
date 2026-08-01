#ifndef UART_H
#define UART_H
#include "stdint.h"
#include "Ringbuffer.h"

void Receive_rx (uint8_t data);
void Uart_init (void);
uint16_t Uart_available (void);
uint8_t Uart_read ();




#endif // UART_H
