#include "Ringbuffer.h"

// push data vao buffer
uint8_t Ring_buffer_push (Ring_buffer_t*p , uint8_t data)
{
    uint16_t next = p->pHead + 1;
    
    if ( next >= p->max_len)  {next =0;} // vong nguoc lai

    if ( next == p->pTail)  return -1 ; // full buffer
       
    p->buffer[p->pHead] = data;
    p->pHead = next;
    return 0;
}

// pop data ra khoi buffer
uint8_t Ring_buffer_pop (Ring_buffer_t*p , uint8_t *data)
{
    if (p->pHead == p->pTail) return -1 ;  // buffer empty

    uint16_t next = p->pTail +1;

    if ( next >= p->max_len) {next =0;} // vong nguoc lai

    *data = p->buffer[p->pTail] ; // lay du lieu ra
    p->pTail = next;
    return 0 ;
}


// lay so luong du lieu trong buffer
uint16_t Ring_buffer_available (Ring_buffer_t*p)
{
    if (p->pHead >= p->pTail) return (p->pHead - p->pTail);


    else return (p->max_len - (p->pTail - p->pHead));
} 


// config ring buffer
void Ring_buffer_init (Ring_buffer_t*ring_buffer, uint8_t* buffer, uint16_t len)
{
    ring_buffer->buffer = buffer;
    ring_buffer->max_len = len;
    ring_buffer->pHead = 0;
    ring_buffer->pTail = 0;
}