#ifndef LIBGRPH
#define LIBGRPH

#include <stdlib.h>
#include <stdio.h>

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

static inline void inmap_free(inmap *ptr){
    if(ptr == NULL)
        return;

    free(ptr->vector);
    free(ptr);
}

graph *graph_alloc(int nodes, int edges);

#endif
