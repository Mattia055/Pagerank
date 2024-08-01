#ifndef LIBGRPH
#define LIBGRPH

#include <semaphore.h>

#define HERE __FILE__,__LINE__

#ifndef BUF_SIZE
#define BUF_SIZE 2048
#endif
#ifndef DYN_DEF
#define DYN_DEF 100
#endif

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
    int dead_count;
}graph;

void inmap_push(inmap **obj, int elem, int *size)__attribute__((always_inline));

void inmap_free(inmap *ptr)__attribute__((always_inline));

graph *graph_alloc(int nodes, int edges);

void graph_destroy(graph *);

typedef struct parser_new_attr{
    int id;
    int *buffer;
    int index;
    sem_t *free_slots;
    sem_t *data_items;
    int *dyn_size;
    inmap **in;
    
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

graph *graph_parse(const char *,int ,bool);

void *parser_routine(void *);

int cmp(const void *a, const void *b);

void *sorter_routine(void *);

#endif
