#ifndef LIBGRPH
#define LIBGRPH

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#define HERE __FILE__,__LINE__
#define BUF_SIZE    2048
#define DYN_DEF     10
#define MUX_DEF     500
#define THREAD_TERM -1  //should be negative or bigger than the highest graph node index

typedef struct{
    int *vector;
    int length;
}inmap;

typedef struct{
    int nodes;          //nodes count
    int edges;          //valid edge count
    inmap **in;         //vector of ptrs to inmap structs (if NULL dead end)
    int *out;           //vector (one per node) with the count of outer edges
    int dead_count;     //cound of dead nodes
    int *dead_end;      //vector of dead end nodes tag (length: dead_count)
}graph;

/**
 * inmap_push()
 * ------------
 * param 
 *      inmap **obj: pointer to inmap struct inside "in" vector
 *      int    elem: node to push in the array
 *      int   *size: pointer to int variable, useful to realloc the vector
 */

static inline void inmap_push(inmap **obj, int elem, int *size){
    if(*obj == NULL){
        *obj = xmalloc(sizeof(inmap), HERE);
        *size = DYN_DEF;
        (*obj)->vector = xmalloc(DYN_DEF * sizeof(int), HERE);
        (*obj)->length = 0;
    }
    else if((*obj)->length == *size){
        *size *= 2;
        (*obj)->vector = xrealloc((*obj)->vector, (*size) * sizeof(int), HERE);
    }

    (*obj)->vector[(*obj)->length] = elem;
    (*obj)->length += 1;

}

static inline void inmap_free(inmap *ptr){
    if(ptr == NULL)
        return;

    free(ptr->vector);
    free(ptr);
}

graph *graph_alloc(int nodes, int edges);

typedef struct parser_attr{
    int             index;
    int             *pc_buffer;
    int             *dyn_size;
    pthread_mutex_t *buffer_mux;
    pthread_mutex_t **graph_mux;
    sem_t           *free_slots;
    sem_t           *data_items;
    graph           *graph;
}parser_attr;

#endif
