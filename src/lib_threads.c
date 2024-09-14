#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
/*
Implementation of single producer multiple consumer lock free
bounded buffer. For a single producer single consumer bounded 
buffer i can use atomic variables to control the state of the
buffer (head and tail) and use custom functions to handle 
insertion and extraction of an element from the buffer.
I can allocate the buffer on the heap or on the stack, since 
the size is bounded.

Must think of something different for multiple producer single 
consumer, 

synchronize the multiple end of the queue using spinlock (implemented
with atomic variables)
*/

#define BUFFER_SIZE 16 // Must be a power of 2 for simplicity

typedef struct {
    int origin;
    int destination;
} node;

typedef struct {
    atomic_size_t head;   // Index of the head (consumer)
    atomic_size_t tail;   // Index of the tail (producer)
    int buffer[BUFFER_SIZE]; // Ring buffer storage
} LockFreeBuffer;