#include "lib_graph.h"
#include "lib_supp.h"
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>

/**
 * NOTES
 * ----------------------------------------
 * The following are defined in header file
 * - inmap_free()
 * - inmap_push()
 * - HERE
 * - BUF_SIZE
 * - DYN_DEF
 * - MUX_DEF
 * - THREAD_TERM
 */

graph *graph_alloc(int nodes, int edges){
    graph *g = xmalloc(sizeof(graph),HERE);
    g->nodes = nodes;
    g->edges = edges;
    g->out  = xcalloc(nodes,sizeof(int),    HERE);
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

graph *graph_parse(const char *pathname, int thread_count){
    char    *getline_buff = NULL;
    size_t  getline_size = 0;
    int     r,c,edges_count;
    int     lines = 0;

    FILE    *file = xfopen(pathname,"r",HERE);

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

    graph   *g = graph_alloc(r,edges_count);

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
    pthread_mutex_t *graph_mux = xmalloc(MUX_DEF * sizeof(pthread_mutex_t),HERE);
    for(int i = 0; i<MUX_DEF; i++){
        xpthread_mutex_init(&(graph_mux[i]),HERE);
    }

    pthread_t tid[thread_count];
    parser_attr parser_t_attr;
    parser_t_attr.graph       = g;  
    parser_t_attr.buffer_mux  = &buffer_mutex;
    parser_t_attr.graph_mux   = graph_mux;
    parser_t_attr.pc_buffer   = pc_buffer;
    parser_t_attr.index       = 0;
    parser_t_attr.free_slots  = &free_slots;
    parser_t_attr.data_items  = &data_items;
    parser_t_attr.dyn_size    = dynamic_size;

    for(int i = 0; i < thread_count; i++){
        xpthread_create(&tid[i],parser_routine,&parser_t_attr,HERE);
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
        if( ori == dest || ori<=0 || dest <=0 || ori>g->nodes || dest>g->nodes){
            g->edges -=1;
            continue;
        }

        //insert edge on pc_buffer
        xsem_wait(&free_slots,HERE);
            pc_buffer[index]                = ori -1;
            pc_buffer[index + 1 % BUF_SIZE] = dest-1;
            index = (index + 2) % BUF_SIZE;
            (g->out[ori-1])+=1;
        xsem_post(&data_items,HERE); 

    }

    //insert termination value for threads
    for(int i = 0; i<thread_count; i++){
        xsem_wait(&free_slots,HERE);
            pc_buffer[index]                = THREAD_TERM;
            pc_buffer[index + 1 % BUF_SIZE] = THREAD_TERM;
            index = (index + 2) % BUF_SIZE;
        xsem_post(&data_items,HERE); 
    }
    for(int i = 0; i<thread_count; i++){
        xpthread_join(tid[i],NULL,HERE);
    }

    //deallocs struct needed no more
    xfclose(file,HERE);
    for(int i = 0; i<MUX_DEF; i++){
        xpthread_mutex_destroy(&graph_mux[i],HERE);
    }
    free(graph_mux);
    free(dynamic_size);
    free(getline_buff);
    xsem_destroy(&free_slots,HERE);

    //i'm gonna reuse buffer_mux and the semaphores (same for the buffer)
    xsem_init(&free_slots,0,BUF_SIZE,HERE);
    
    /**
     * Create a new group of threads which sorts all "in[i]" in their
     * interval inserting duplicates in a pc_buffer that main
     * thread reads to decrement "out[i]"
     */

    //struct shared between threads
    sorter_attr_shared sorter_shared;
    sorter_shared.pc_index    = 0;
    sorter_shared.pc_buffer   = pc_buffer;
    sorter_shared.graph       = g;
    sorter_shared.buffer_mux  = &buffer_mutex;
    sorter_shared.free_slots  = &free_slots;
    sorter_shared.data_items  = &data_items;

    //calculate interval length
    /**
     * DEBUG: Add error support
     * When 0 lists are assigned to a thread 
     */
    
    int int_start   = 0;
    const int int_length  = (int)(g->nodes / thread_count);
    sorter_attr thread_attr[thread_count];
    for(int i = 0; i<thread_count; i++){
        thread_attr[i].interval_start   = int_start;
        thread_attr[i].interval_end     = i == thread_count - 1? g->nodes -1 : int_start + int_length;
        thread_attr[i].shared           = &sorter_shared;
        xpthread_create(&tid[i],sorter_routine,&(thread_attr[i]),HERE);

        int_start += int_length +1;
        
    }

    //read from buffer
    int ready_for_join = 0;
    index = 0;
    int duplicate;

    do{
        xsem_wait(&data_items,HERE);
            duplicate = pc_buffer[index];
            index = index + 1 % BUF_SIZE;
        xsem_post(&free_slots,HERE);

        if(duplicate == THREAD_TERM){
            ready_for_join++;
        }
        else{
            g->out[duplicate]   -= 1;
            g->edges            -= 1;
        }

    }while(ready_for_join < thread_count);

    for(int i = 0; i<thread_count; i++){
        xpthread_join(tid[i],NULL,HERE);
    }

    free(pc_buffer);
    xsem_destroy(&free_slots,HERE);
    xsem_destroy(&data_items,HERE);
    xpthread_mutex_destroy(&buffer_mutex,HERE);

    /**
     * Find add dead end nodes (out[node] = 0)
     */

    int size    = DYN_DEF;
    int length  = 0;
    int *arr    = xmalloc(size * sizeof(int),HERE);

    for(int i = 0; i<g->nodes; i++){
        if(g->out[i] == 0){
            //realloc array
            if(size == length){
                size   *= 2;
                arr     = xreallocarray(arr,size,sizeof(int),HERE); 
            }

            arr[length++] = i;
        }
    } 

    //final realloc
    arr = xrealloc(arr,length*sizeof(int),HERE);
    g->dead_end     = arr;
    g->dead_count   = length;

    return g;

}

void *parser_routine(void *attr){

    /**
     * NOTE: Maybe adding internal bufferization and an entry point could
     * increase performance.
     */

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

        if(ori == THREAD_TERM || dest == THREAD_TERM){
            pthread_exit(NULL);
        }

        /**
         * insert the node ori in in[dest].
         * Uses static inline method inmap_push
         * 
         * takes mutex from mutex_vector
         * 
         * Actually using an array of mutexes is a problem
         * still dont know why but it seems that it creates
         * a race condition between threads
         * ------------------------------------------------
         * Debug: now i use 1 mutex so sequencial writes,
         * change that to a bitmap that identifies each cell
         * of "in", and a try-lock mechanism that does at such:
         * 1. if the cell is empty occupy and write 
         * 2. else write the node to an interal buffer and try the write
         * in the next cycle. The buffer should be implemented as a queue
         * so that the last node inserted will be tried forward in time
         */
        xpthread_mutex_lock(&((arg->graph_mux[dest % MUX_DEF])),HERE);
            inmap_push(&(((arg->graph)->in)[dest]), ori, &((arg->dyn_size)[dest]));
        xpthread_mutex_unlock(&(arg->graph_mux[dest % MUX_DEF]),HERE);

    }
}

int cmp(const void *a, const void *b){
    return (*(int *)a - *(int *)b);
}

void *sorter_routine(void *attr){
    sorter_attr *arg = (sorter_attr *)attr;
    sorter_attr_shared *shared = arg->shared;

    inmap *curr_obj;
    int *arr;
    int k;
    //select vector in its interval 
    for(int j = arg->interval_start; j<=arg->interval_end; j++){
        curr_obj = shared->graph->in[j];

        //no vector to sort
        if(curr_obj == NULL){
            continue;
        }

        qsort(curr_obj->vector,curr_obj->length,sizeof(int),cmp);

        //start "deleting" duplicates
        k = 1;

        arr = curr_obj->vector;

        for(int i = 1; i<curr_obj->length; i++){
            //shift left no duplicate elements
            if(arr[i] != arr[i-1]){
                arr[k] = arr[i];
                k+=1;
            }
            //insert duplicates on buffer
            else{
                xsem_wait(shared->free_slots,HERE);
                    xpthread_mutex_lock(shared->buffer_mux,HERE);

                        shared->pc_buffer[shared->pc_index] = arr[i];
                        shared->pc_index = (shared->pc_index + 1) % BUF_SIZE;

                    xpthread_mutex_unlock(shared->buffer_mux,HERE);
                xsem_post(shared->data_items,HERE);
            }
        }

        //realloc vector
        arr = xrealloc(arr,k*sizeof(int),HERE);
        curr_obj->length = k;
        curr_obj->vector = arr;
    }

    //insert thread termination value
    xsem_wait(shared->free_slots,HERE);
        xpthread_mutex_lock(shared->buffer_mux,HERE);
            shared->pc_buffer[shared->pc_index] = THREAD_TERM;
            shared->pc_index = (shared->pc_index + 1) % BUF_SIZE;
        xpthread_mutex_unlock(shared->buffer_mux,HERE);
    xsem_post(shared->data_items,HERE);

    pthread_exit(NULL);
}

