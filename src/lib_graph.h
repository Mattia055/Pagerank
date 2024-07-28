#ifndef LIBGRPH
#define LIBGRPH

#include <semaphore.h>

#define HERE __FILE__,__LINE__
#define BUF_SIZE    4096
#define DYN_DEF     300
#define MUX_DEF     1009    //should be a prime number
#define THREAD_TERM -1      //should be negative or bigger than the highest graph node index

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

void inmap_push(inmap **obj, int elem, int *size);

void inmap_free(inmap *ptr);

graph *graph_alloc(int nodes, int edges);

void graph_destroy(graph *);

typedef struct parser_attr{
    int             index;
    int             *pc_buffer;
    int             *dyn_size;
    pthread_mutex_t *buffer_mux;
    pthread_mutex_t *graph_mux;
    sem_t           *free_slots;
    sem_t           *data_items;
    graph           *graph;
}parser_attr;

typedef struct sorter_attr_shared{
    int              pc_index;
    int             *pc_buffer;
    graph           *graph;
    pthread_mutex_t *buffer_mux;
    sem_t           *free_slots;
    sem_t           *data_items;
}sorter_attr_shared;

typedef struct sorter_attr{
    sorter_attr_shared  *shared;
    int                 interval_start;
    int                 interval_end;
}sorter_attr;

graph *graph_parse(const char *, int);

void *parser_routine(void *);

int cmp(const void *a, const void *b);

void *sorter_routine(void *);


#endif
