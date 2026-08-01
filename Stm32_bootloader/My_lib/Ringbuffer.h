#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <stdint.h>
typedef struct 
{
    uint8_t *  buffer;
    uint16_t pHead;
    uint16_t pTail;
    uint16_t max_len;
} Ring_buffer_t;

void Ring_buffer_init (Ring_buffer_t*ring_buffer, uint8_t* buffer, uint16_t len);
uint8_t Ring_buffer_pop (Ring_buffer_t*p , uint8_t *data);
uint8_t Ring_buffer_push (Ring_buffer_t*p , uint8_t data);
uint16_t Ring_buffer_available (Ring_buffer_t*p);
#endif