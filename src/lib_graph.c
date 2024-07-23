#include "./src/lib_graph.h"
#include "./src/lib_supp.h"
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

/**
 * NOTES
 * ----------------------------------------
 * The following are defined in header file
 * as "static inline" methods
 * 
 * - inmap_free()
 * - inmap_push()
 * - BUF_SIZE
 * - DYN_DEF
 * - MUX_DEF
 * - THREAD_TERM
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
void *parser_routine(void *);
void *sorter_routine(void *);

graph *graph_parse(char *pathname, int thread_count){
    char *getline_buff = NULL;
    size_t getline_size = 0;
    int r,c,edges_count;
    int lines = 0;

    FILE *file = xfopen(pathname,"r",HERE);

    do{
        if(getline(&getline_buff,&getline_size,file)==-1){
            error("[getline] comments",HERE);
        }
        lines ++;
    }while(getline_buff[0] == '%');

    if(sscanf(getline_buff,"%d %d %d",&r,&c,&edges_count)!=3){
        error("[sscanf] parsing first significant line",HERE);
    }

    if(r!=c || r < 1 || edges_count<0){
        error("[graph_parse] Bad file",HERE);
    }

    graph *g = graph_alloc(r,edges_count);

    /**
     * Alloc and Init of structures needed by threads
     */

    int *dynamic_size   = xmalloc(r * sizeof(int),HERE);
    int *pc_buffer      = xmalloc(BUF_SIZE * sizeof(int),HERE);

    sem_t free_slots, data_items;
    xsem_init(&free_slots,0,BUF_SIZE / 2,HERE);
    xsem_init(&data_items,0,0,HERE);

    pthread_mutex_t buffer_mutex;
    xpthread_mutex_init(&buffer_mutex,HERE);

    /**
     * Uses an array of mutex to synch each thread on the corresp.
     * struct.
     * 
     * The choice on which mutex to use is done using the '%' operator
     * so that it minimizes collisions;
     */
    pthread_mutex_t **graph_mux = xmalloc(MUX_DEF * sizeof(pthread_mutex_t *),HERE);
    for(int i = 0; i<MUX_DEF; i++){
        xpthread_mutex_init(graph_mux[i],HERE);
    }

    pthread_t tid[thread_count];
    parser_attr thread_attr;
    thread_attr.buffer_mux  = &buffer_mutex;
    thread_attr.graph_mux   = graph_mux;
    thread_attr.pc_buffer   = pc_buffer;
    thread_attr.index       = 0;
    thread_attr.free_slots  = &free_slots;
    thread_attr.data_items  = &data_items;
    thread_attr.dyn_size    = dynamic_size;

    for(int i = 0; i < thread_count; i++){
        xpthread_create(&tid[i],parser_routine,&thread_attr,HERE);
    }

    //START READING EDGES FROM FILE

    int ori,dest;
    int index = 0;
    while(getline(&getline_buff,&getline_size,file) != -1){
        lines ++;

        if(sscanf(getline_buff,"%d %d",&ori,&dest)!=2){
            char *err_mess;
            if(asprintf(&err_mess, "[sscanf] error parsing edge at line %d",lines)<0){
                error("[sscanf] error parsing edge",HERE);
            }
            error(err_mess,HERE);
        }

        //discard not valid edges
        if(ori == dest || ori<0 || dest <0 || ori>g->nodes || dest>g->nodes){
            g->edges -=1;
            continue;
        }

        //insert edge on pc_buffer
        xsem_wait(&free_slots,HERE);
            pc_buffer[index] = ori;
            pc_buffer[index + 1 % BUF_SIZE] = dest;
            index = (index + 2) % BUF_SIZE;
        xsem_post(&data_items,HERE); 

    }

    //insert termination value for threads
    xsem_wait(&free_slots,HERE);
        pc_buffer[index] = THREAD_TERM;
        pc_buffer[index + 1 % BUF_SIZE] = THREAD_TERM;
        index = (index + 2) % BUF_SIZE;
    xsem_post(&data_items,HERE); 

    for(int i = 0; i<tid[thread_count]; i++){
        xpthread_join(tid[i],NULL,HERE);
    }

    //deallocs struct needed no more
    for(int i = 0; i<MUX_DEF; i++){
        xpthread_mutex_destroy(graph_mux[i],HERE);
    }
    free(graph_mux);
    free(dynamic_size);
    free(getline_buff);
    xsem_destroy(&free_slots,HERE);

    //i'm gonna reuse buffer_mux and the semaphores (same for the buffer)
    xsem_init(&free_slots,0,BUF_SIZE,HERE);
    pthread_mutex_t index_mux;
    xpthread_mutex_init(&index_mux,HERE);
    

    /**
     * Create a new group of threads that work with a shared index sorting
     * the "in" vectors and inserting duplicates in a pc_buffer that main
     * thread reads to decrement "out[i]"
     */

}

void *parser_routine(void *attr){
    parser_attr *arg = (parser_attr *)attr;

    int ori,dest;

    while(true){
        xsem_wait(arg->data_items,HERE);
            xpthread_mutex_lock(arg->buffer_mux,HERE);

                ori     = arg->pc_buffer[arg->index];
                dest    = arg->pc_buffer[(arg->index + 1) % BUF_SIZE];
                arg->index = (arg->index + 2) % BUF_SIZE;

            xpthread_mutex_unlock(arg->buffer_mux,HERE);
        xsem_post(arg->free_slots,HERE);

        if(ori == THREAD_TERM && dest == THREAD_TERM){
            pthread_exit(NULL);
        }

        /**
         * insert the node ori in in[dest].
         * Uses static inline method inmap_push
         * 
         * takes mutex from mutex_vector
         */
        xpthread_mutex_unlock(arg->graph_mux[dest % MUX_DEF],HERE);
            inmap_push(&(arg->graph->in[dest]), ori, &(arg->dyn_size[dest]));
            (arg->graph->out)[ori] += 1;
        xpthread_mutex_unlock(arg->graph_mux[dest % MUX_DEF],HERE);

    }
}

