#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#include "lib_graph.h"
#include "lib_pagerank.h"
#include "lib_supp.h"

/**
 * NOTES
 * ----------------------------------------
 * The following are defined in header file
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
    return g;
}

void graph_destroy(graph *g){
    free(g->out);
    for(int i = 0; i<g->nodes; i++)
        inmap_free(g->in[i]);
    
    free(g->in);
    free(g);
}

/**
 * inmap_push()
 * ------------
 * param 
 *      inmap **obj: pointer to inmap struct inside "in" vector
 *      int    elem: node to push in the array
 *      int   *size: pointer to int variable, useful to realloc the vector
 */
inline void inmap_push(inmap **obj, int elem, int *size){
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

inline void inmap_free(inmap *ptr){
    if(ptr == NULL)return;

    free(ptr->vector);
    free(ptr);
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
graph *graph_parse(const char *pathname, int thread_count,bool take_time){
    
    struct timeval start,end,alloc_start,alloc_end,file_start,file_end,sort_start,sort_end;
    xgettimeofday(&start,take_time,HERE);
    xgettimeofday(&alloc_start,take_time,HERE);

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

    int interval_length = g->nodes / thread_count;

    int *pc_buffer[thread_count];
    int     pc_index[thread_count];
    sem_t   free_slots_parser[thread_count];
    sem_t   data_items_parser[thread_count];

    /*init of components */
    for(int i = 0; i<thread_count; i++){
        pc_buffer[i] = xmalloc(sizeof(int) * BUF_SIZE,HERE);
        pc_index[i] = 0;
        xsem_init(&(free_slots_parser[i]),0,BUF_SIZE/2,HERE);
        xsem_init(&(data_items_parser[i]),0,0,HERE);
    }

    pthread_t   tid[thread_count];
    parser_attr arg[thread_count];  

    for(int i = 0; i<thread_count; i++){
        arg[i].id = i;
        arg[i].buffer = pc_buffer[i];
        arg[i].index = 0; 
        arg[i].dyn_size = dynamic_size;
        arg[i].free_slots = &(free_slots_parser[i]);
        arg[i].data_items = &(data_items_parser[i]);
        arg[i].in         = g->in;

        xpthread_create(&tid[i],parser_routine,&arg[i],HERE);
    }

    xgettimeofday(&alloc_end,take_time,HERE);
    xgettimeofday(&file_start,take_time,HERE);

    int ori,dest;
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

        //insert edge on pc_buffer of one thread
        /**
         * to insert the edge, the in[dest] must be locked
         * so i insert the edge in just one buffer, selection
         * the index using the modulo operator
         */

        int j = ((dest-1) / interval_length) % thread_count;

        //printf("ori %d dest %d\n",ori-1,dest-1);

        xsem_wait(&(free_slots_parser[j]),HERE);
            pc_buffer[j][pc_index[j]]                   = ori -1;
            pc_buffer[j][(pc_index[j] + 1)  % BUF_SIZE] = dest-1;
            pc_index[j] = (pc_index[j] + 2) % BUF_SIZE;
        xsem_post(&(data_items_parser[j]),HERE); 
        (g->out[ori-1])+=1;

    }

    xgettimeofday(&file_end,take_time,HERE);

    //insert termination value for threads
    for(int i = 0; i<thread_count; i++){
        xsem_wait(&(free_slots_parser[i]),HERE);
            pc_buffer[i][pc_index[i]]                   = THREAD_TERM;
            pc_buffer[i][(pc_index[i] + 1)  % BUF_SIZE] = THREAD_TERM;
            pc_index[i] = (pc_index[i] + 2) % BUF_SIZE;
        xsem_post(&(data_items_parser[i]),HERE); 
    }

    for(int i = 0; i<thread_count; i++){
        xpthread_join(tid[i],NULL,HERE);
    }

    //deallocs struct needed no more
    xfclose(file,HERE);
    free(dynamic_size);
    free(getline_buff);

    for(int i = 0; i<thread_count; i++){
        free(pc_buffer[i]);
        xsem_destroy(&(free_slots_parser[i]),HERE);
        xsem_destroy(&(data_items_parser[i]),HERE);
    }
    
    int *sorter_buffer = xmalloc(BUF_SIZE * sizeof(int),HERE);
    sem_t free_slots_sorter,data_items_sorter;
    pthread_mutex_t buffer_mux;
    xsem_init(&free_slots_sorter,0,BUF_SIZE,HERE);
    xsem_init(&data_items_sorter,0,0,HERE);
    xpthread_mutex_init(&buffer_mux,HERE);
    
    /**
     * Create a new group of threads which sorts all "in[i]" in their
     * interval inserting duplicates in a pc_buffer that main
     * thread reads to decrement "out[i]"
     */

    //struct shared between threads
    sorter_attr_shared sorter_shared;
    sorter_shared.pc_index    = 0;
    sorter_shared.pc_buffer   = sorter_buffer;
    sorter_shared.graph       = g;
    sorter_shared.buffer_mux  = &buffer_mux;
    sorter_shared.free_slots  = &free_slots_sorter;
    sorter_shared.data_items  = &data_items_sorter;

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
    xgettimeofday(&sort_start,take_time,HERE);
    //puts("SORTER STARTED");

    //read from buffer
    int ready_for_join = 0;
    int index = 0;
    int duplicate;

    do{
        xsem_wait(&data_items_sorter,HERE);
            duplicate = sorter_buffer[index];
            index = index + 1 % BUF_SIZE;
        xsem_post(&free_slots_sorter,HERE);

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

    xgettimeofday(&sort_end,take_time,HERE);

    int dead_count = 0;
    for(int i = 0; i<g->nodes; i++){
        if(g->out[i] == 0) dead_count++;
    }
    g->dead_count = dead_count;

    free(sorter_buffer);
    xsem_destroy(&free_slots_sorter,HERE);
    xsem_destroy(&data_items_sorter,HERE);
    xpthread_mutex_destroy(&buffer_mux,HERE);

    xgettimeofday(&end,take_time,HERE);

    if(take_time){
        fprintf(stderr,"\n======\tTime Stats\t======\n");
        fprintf(stderr,"alloc time\t\t%.6f sec\n",exctract_time(alloc_start,alloc_end,take_time));
        fprintf(stderr,"read time\t\t%.6f sec\n",exctract_time(file_start,file_end,take_time));
        fprintf(stderr,"sort time\t\t%.6f sec\n",exctract_time(sort_start,sort_end,take_time));
        fprintf(stderr,"total time\t\t%.6f sec\n",exctract_time(start,end,take_time));
        fprintf(stderr,"\n=========================\n");
    }

    return g;

}
void *parser_routine(void *attr){
    
    parser_attr *arg = (parser_attr *)attr;
    int ori,dest;

    while(true){
        xsem_wait(arg->data_items,HERE);

                ori     = arg->buffer[arg->index];
                dest    = arg->buffer[(arg->index + 1) % BUF_SIZE];
                arg->index = (arg->index + 2) % BUF_SIZE;

        xsem_post(arg->free_slots,HERE);

        if(ori == THREAD_TERM || dest == THREAD_TERM){
            pthread_exit(NULL);
        }
        
        inmap_push(&(((arg)->in)[dest]), ori, &((arg->dyn_size)[dest]));

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
    //puts("SORTER EXITING");
    pthread_exit(NULL);
}
