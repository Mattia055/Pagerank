#include "./src/lib_graph.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * NOTES
 * ----------------------------------------
 * The following are defined in header file
 * as "static inline" methods
 * 
 * - inmap_free()
 * - inmap_push()
 */

#define HERE __FILE__,__LINE__

graph *graph_alloc(int nodes, int edges){
    graph *g = xmalloc(sizeof(graph),HERE);
    g->nodes = nodes;
    g->edges = edges;
    g->out  = xcalloc(nodes,sizeof(int),HERE);
    g->in   = xcalloc(nodes,sizeof(inmap *),HERE);
    /**
     * following are just placeholders
     * they will be set later on
     */
    g->dead_count   = 0;
    g->dead_end     = NULL;
    return g;
}

void graph_destroy(graph *g){

    if(g->dead_end != NULL){
        free(g->dead_end);
    }
    free(g->out);

    for(int i = 0; i<g->nodes; i++){
        inmap_free(g->in[i]);
    }

    free(g->in);
    free(g);
}

/**
 * ------------------------------------------
 * Parses a graph as a multithread solution
 * ------------------------------------------
 * parameters:
 *      `pathname` = file `.mtx`
 *      `int thread_count`
 * returns:
 *      `* struct graph`
 * -------------------------------------
 * ##### BRIEF NOTES ON .mtx files #####
 * -------------------------------------
 * - lines starting with '%' are comments 
 *   and are ignored
 * - first significant line has the following 
 *   syntax "a b c" where a,b,c are integers: 
 *   a must be equal to b (number of nodes)
 *   c is the number of edges
 * - subsequent lines contain two integers "a b": origin - destination
 *   edge definition
 * - IMPORTANT : any line formatted different 
 *   than the previous defs. is threaded as 
 *   malformed line and makes the parsing invalid
 *   terminating the entire process
 * -------------------------------------
 * ##### IMPLEMENTATION KEY POINTS #####
 * -------------------------------------
 * 1. Reads from file using threads to insert
 * nodes in the "in" arrays. Doesn't count
 * invalid edges (malformed). Treats "in" arrays
 * as "dynamic arrays" pushing each edge in the end
 * of the corrisponding array
 * 
 * 2. When the read is completed, uses threads to
 * sort "in" arrays concurrently updating lists
 * discarding duplicate edges in O(n) time. Then
 * reallocs the "in" arrays to a fixed size
 * 
 * 3. The duplicates are inserted in a buffer read
 * from the main thread that updates the count on
 * the "out" array
 */

graph *graph_parse(char *pathname, int thread_count){
    FILE *file = xfopen(pathname,"r",HERE);
}

