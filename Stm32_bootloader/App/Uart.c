#include "Uart.h"


#define RING_BUFFER_SIZE 128
static Ring_buffer_t my_ring_buffer;
uint8_t buffer[RING_BUFFER_SIZE];


void Receive_rx (uint8_t data)
{
    Ring_buffer_push(&my_ring_buffer, data);
}


uint16_t Uart_available (void)
{
    return Ring_buffer_available(&my_ring_buffer);
}

uint8_t Uart_read ()
{
    uint8_t data;
    Ring_buffer_pop(&my_ring_buffer, &data);
    return data;
}
void Uart_init (void)
{
    Ring_buffer_init(&my_ring_buffer, buffer, RING_BUFFER_SIZE); 
}