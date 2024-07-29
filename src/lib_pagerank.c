#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include "lib_pagerank.h"
#include "lib_supp.h"

#define HERE __FILE__,__LINE__

void printHelp(){
    puts("usage: pagerank [-h] [-s] [-k K] [-m M] [-d D] [-e E] [-t T] infile");
    puts("");
    puts("Compute pagerank for a directed graph represented by the list of its edges");
    puts("following the Matrix Market format: https://math.nist.gov/MatrixMarket/formats.html#MMformat");
    puts("Uses teleporting and the damping factor as in the original PageRank paper");
    puts("");
    puts("Multithread Process, with threads executing CPU intensive tasks. It's suggested to use a number");
    puts("of threads that matches (or is close) to the CPU cores count");
    puts("");
    puts("Display the k highest ranked nodes (default k=3)");
    puts("");
    puts("positional arguments:");
    puts("infile:\t\tinput file");
    puts("");
    puts("options:");
    puts("-h\t\tshow this help message and exit");
    puts("-k K\t\tshow top K nodes (default 3)");
    puts("-m M\t\tmaximum number of iterations (default 100)");
    puts("-d D\t\tdamping factor (default 0.9)");
    puts("-e E\t\tmax error (default 1.0e7)");
    puts("-t T\t\tthreads count (default 3)");
    puts("-s\t\tDisable signal handler (SIGUSR1 to print current max node)");
}

inline void printGraphInfo(graph *g,FILE *stream,bool comment){
    if(!comment)
        fprintf(stream, "Number of nodes: %d \nNumber of dead-end nodes: %d\nNumber of valid arcs: %d\n", g->nodes, g->dead_count, g->edges);
    else
        fprintf(stream, "%%Number of nodes: %d \n%%Number of dead-end nodes: %d\n%%Number of valid arcs: %d\n", g->nodes, g->dead_count, g->edges);
}

/*
 * Returns the list of indexes: one for every k max
 * ------------------------------------------------
 * Implemented as a repeated search for max, still
 * asymptotically better than heapsort, at worst (k == n)
 * is Theta(n^2)
*/
int *find_K_Max(double *ranks, int length,int k){
    int     *index      = xmalloc(k * sizeof(int),      HERE);
    double  *max_ranks  = xmalloc(k * sizeof(double),   HERE);

    double  max;
    int     max_idx;

    //loop for each max;
    for(int i = 0; i<k; i++){
        max     = ranks[0];
        max_idx = 0;

        for(int j = 1; j<length; j++){
            if(ranks[j] > max){
                max     = ranks[j];
                max_idx = j;
            }
        }

        max_ranks[i]    = max;
        index[i]        = max_idx;
        ranks[max_idx]  = -1;
    }

    //clean the ranks array (rewrite all -1)
    for(int i = 0; i<k; i++){
        ranks[index[i]] = max_ranks[i];
    }

    free(max_ranks);
    return index;
}

void printStats(double *ranks,int length,int iter_count,int max_iter,int k, FILE *stream){
    double sum_ranks = 0;
    for(int i = 0; i<length; i++){
        sum_ranks += ranks[i];
    }

    int *max_index = find_K_Max(ranks,length,k);
    if(iter_count == max_iter)
        fprintf(stream,"Did not converge after %d iterations\n", iter_count);
    else
        fprintf(stream,"Converged after %d iterations\n", iter_count);

    fprintf(stream,"Sum of ranks: %f (should be 1)\n",sum_ranks);
    fprintf(stream, "Top %d nodes:\n",k);

    for(int i = 0; i<k; i++){
        fprintf(stream,"\t%d\t%f\n",max_index[i],ranks[max_index[i]]);
    }

    free(max_index);

    return;
}

/**
 * Pagerank Computation Utilities
 * ---------------------------------------------------------------------------
 *      typedef struct pagerank_shared_attr {
 *          //doppi puntatori per i vettori delle iterazioni per fare lo swap
 *          double          **X_current;
 *          double          **X_previous;
 *          double          *Y;
 *          double          S_t;
 *          double          S_t_shared;
 *          double          epsilon;
 *          double          error;
 *          double          dumping_factor;
 *          graph           *grph;
 *          int             max_iter;
 *          bool            do_X;
 *          bool            do_Y;
 *          bool            exit;
 *          int             waiting_on_Y;
 *          int             waiting_on_X;
 *          int             *curr_iter;
 *          int             thread_count;
 *          pthread_mutex_t *cond_mux;
 *          pthread_mutex_t *shared_mux;
 *          pthread_cond_t  *cond;
 *      } pagerank_shared_attr;
 *
 *      typedef struct pagerank_thread_attr{
 *          int interval_start;
 *          int interval_end;
 *          pagerank_shared_attr *shared;
 *      } pagerank_thread_attr;
 * -------------------------------------------------------------------
 */
void *pagerank_routine(void *attr){
    pagerank_thread_attr *arg = (pagerank_thread_attr *)attr;
    pagerank_shared_attr *shared = arg->shared;
    const double teleport = (1.0 - shared->dumping_factor) / ((double)(shared->grph->nodes));
    double my_S_t;
    double my_error;
    
    //swap variable for vectors;
    double *temp;
    do{
        // === Computation of Y components ===
        for(int i = arg->interval_start; i<=arg->interval_end; i++){
            if(shared->grph->out[i] > 0)
                shared->Y[i] = ((*(shared->X_previous))[i]) / ((double)(shared->grph->out)[i]);
        }

        // === Thread suspension ===
        xpthread_mutex_lock(shared->cond_mux, HERE);
            if((shared->waiting_on_Y) == (shared->thread_count - 1)){
                shared->do_X = true;
                shared->do_Y = false;
                xpthread_cond_broadcast(shared->cond, HERE);
            } 
            else {
                shared->waiting_on_Y += 1;
                    while (shared->do_X == false)
                        xpthread_cond_wait(shared->cond, shared->cond_mux, HERE);  
                shared->waiting_on_Y -= 1;
            }
        xpthread_mutex_unlock(shared->cond_mux, HERE);

        // === Computation of X components ===

        my_error    = 0.0;
        my_S_t      = 0.0;

        for(int i = arg->interval_start; i<=arg->interval_end; i++){
            double sum_dead_end = 0.0;
            inmap *obj = ((shared->grph)->in)[i];
            
            if(obj != NULL){
                for(int i = 0; i < obj->length; i++)
                    sum_dead_end += (shared->Y)[(obj->vector[i])];
            }     
            (*(shared->X_current))[i] = teleport + (shared->dumping_factor * sum_dead_end) + (shared->dumping_factor / (double)(shared->grph->nodes)) * shared->S_t;  

            //compute S_t for next iteration
            if(shared->grph->out[i] == 0)
                my_S_t += ((*(shared->X_current))[i]);

            //compute error for next iteration
            my_error += fabs((*(shared->X_current))[i] - (*(shared->X_previous))[i]);
        }
        
        // === Thread suspension ===
        xpthread_mutex_lock(shared->cond_mux, HERE);
            /**
             * Dump shared error and S_t for next iteration
             */
            shared->error       += my_error;
            shared->S_t_shared  += my_S_t;

            if((shared->waiting_on_X) == (shared->thread_count - 1)){
                /**
                 * 1. If error more than threshold (epsilon) exit
                 * 
                 * 2. Swap local S_t with global S_t
                 */
                if((shared->error < shared->epsilon) || (*(shared->curr_iter) == (shared->max_iter - 1)))
                    shared->exit = true;
                
                shared->error       = 0;              
                shared->S_t         = shared->S_t_shared;
                shared->S_t_shared  = 0.0;
                
                xpthread_mutex_lock(shared->shared_mux, HERE);

                    temp = *(shared->X_previous);
                    *(shared->X_previous) = *(shared->X_current);
                    *(shared->X_current) = temp;

                    if(shared->exit == true){
                        /**
                         * Setting previous vector as null
                         * for the signal handler
                         */
                        free(*(shared->X_previous));
                        *(shared->X_previous) = NULL;
                    }

                    *(shared->curr_iter) += 1;

                xpthread_mutex_unlock(shared->shared_mux, HERE);
                shared->do_Y = true;
                shared->do_X = false;
                xpthread_cond_broadcast(shared->cond, HERE);
            }
            else {
                shared->waiting_on_X += 1;

                while(shared->do_Y == false)
                    xpthread_cond_wait(shared->cond, shared->cond_mux , HERE);

                shared->waiting_on_X -= 1;
            }

        xpthread_mutex_unlock(shared->cond_mux, HERE);

    } while(shared->exit == false);

    pthread_exit(NULL);
}

double *pagerank(graph *grph, double dumping, double eps, int max_iter, int thread_count, int *iter_count){
     
    // iteration vectors allocation
    double *X_current   = xmalloc(grph->nodes * sizeof(double), HERE);
            X_previous  = xmalloc(grph->nodes * sizeof(double), HERE);
    double *Y           = xcalloc(grph->nodes , sizeof(double), HERE);

    // popolamento vettori iterazioni
    const double init = 1.0 /(double)grph->nodes;
    for(int i = 0; i < grph->nodes; i++){
        X_current [i]   = init;
        X_previous[i]   = init;
    }

    //Conto un iterazione fatta
    // (*numiter) += 1;

    pthread_t tid[thread_count];
    pagerank_shared_attr shared;

    pthread_mutex_t cond_mux;
    pthread_cond_t cond;
    xpthread_mutex_init(&cond_mux, HERE);
    xpthread_cond_init(&cond, HERE);

    shared.cond             = &cond;
    shared.cond_mux         = &cond_mux;
    shared.curr_iter        = iter_count;
    shared.do_X             = false;
    shared.do_Y             = true;
    shared.dumping_factor   = dumping;
    shared.epsilon          = eps;
    shared.error            = 0.0;
    shared.exit             = false;
    shared.grph             = grph;
    shared.max_iter         = max_iter;
    shared.S_t              = ((double)grph->dead_count) * init;
    shared.S_t_shared       = 0;
    shared.thread_count     = thread_count;
    shared.shared_mux       = &signal_mux;
    shared.waiting_on_X     = 0;
    shared.waiting_on_Y     = 0;
    shared.X_previous       = &X_previous;
    shared.X_current        = &X_current;
    shared.Y                = Y;
    
    pagerank_thread_attr thread_attr[thread_count];
    int int_start = 0;
    const int int_length = (int)(grph->nodes /thread_count);

    for(int i = 0; i < thread_count; i++){
        thread_attr[i].interval_start   = int_start;
        thread_attr[i].interval_end     = (i == thread_count - 1)? grph->nodes -1 : int_start + int_length;
        thread_attr[i].shared           = &shared;
        xpthread_create(&tid[i], pagerank_routine, &(thread_attr[i]), HERE);

        int_start += int_length +1 ;
    }

    for(int i = 0; i < thread_count; i++){
        xpthread_join(tid[i], NULL, HERE);
    }

    *iter_count = *(shared.curr_iter);

    free(Y);
    xpthread_mutex_destroy(&cond_mux, HERE);
    xpthread_cond_destroy(&cond, HERE);    

    return X_current;
}

inline double find_max(double *arr,int length){
    int index       = 0;
    double curr_max = arr[0];

    for(int i = 1; i<length; i++){
        if(arr[i]>curr_max){
            curr_max = arr[i];
            index = i;
        }
    }

    return index;
}

void *signal_handler_routine(void *attr){
    sigset_t local_mask;
    sigemptyset(&local_mask);
    sigaddset(&local_mask,SIGUSR1);
    sigaddset(&local_mask,SIGUSR2);
    pthread_sigmask(SIG_SETMASK,&local_mask,NULL);

    sig_handler_attr *arg = (sig_handler_attr *)attr;
    int sig, curr_index, iter_count;
    double curr_max;
    int action = -1;

    do{
        if(sigwait(&local_mask,&sig)!=0)
            error("[sigwait]",HERE);

        if(sig == SIGUSR1){
            xpthread_mutex_lock(arg->shared_mux,HERE);
                if(*(arg->iter_count)==0){
                    action = 0;
                }
                else if(*(arg->X_previous) == NULL){
                    action = 1;
                }
                else {
                    curr_index  = find_max(*(arg->X_previous),*(arg->nodes));
                    curr_max    = (*(arg->X_previous))[curr_index];
                    iter_count  = *(arg->iter_count);
                    action      = 2;
                }
            xpthread_mutex_unlock(arg->shared_mux,HERE);

            switch(action){
                case 0: fputs("Pagerank computation not began [parsing graph]\n",arg->signal_stream);
                break;
                case 1: fputs("Pagerank computation completed\n",arg->signal_stream);
                break;
                case 2: fprintf(arg->signal_stream,"Iteration count: %d\tMax node%d\tValue:%f\n",iter_count,curr_index,curr_max);
                break;
                default:
                exit(EXIT_FAILURE);
            }
        }
        else if(sig == SIGUSR2){
            break;
        }
    } while(1);

    pthread_exit(NULL);

}

/**
 * DEBUG ONLY
 * ----------
 * Definition of functions that can be used to save the
 * representation of a graph to a file, and compare two
 * representation.
 * ----------------------------------------------------
 * The file representation of a graph is a file formatted
 * like a mtx file, without the number of nodes (v1) or the
 * number of edges. This can be esaily added just like the
 * comments at the begin.
 * 
 * graph_save(graph *,const char *)
 * Saves the representation of the graph to a file. Since the
 * Adjacent Lists are sorted while parsing, the file output
 * is deterministic for each graph.
 * 
 * graph_cmp(const *char,const *char)
 * Compares two representations of graphs to determinate if they
 * are equivalent
 * ---------------------------------------------------------
 * The second function is useful to check if, between parsing
 * versions, the same output is always produced
 */
void graph_save(char *path, graph *grph){
    FILE *file = xfopen(path,"w",HERE);
    //print brief graph info
    printGraphInfo(grph,file,true);
    //print first line
    fprintf(file,"%d %d %d %d\n",grph->nodes,grph->nodes,grph->edges,grph->dead_count);
    //print all valid edges
    inmap *curr_obj;
    for(int i = 0; i<grph->nodes;i++){
        curr_obj = grph->in[i];
        if(curr_obj == NULL)
            continue;
        for(int j = 0; j<curr_obj->length;j++){
            fprintf(file,"%d %d\n",curr_obj->vector[j],i);
        }
    }
    
    //print all dead end nodes
    for(int i = 0; i<grph->dead_count;i++){
        fprintf(file,"%d\n",grph->dead_end[i]);
    }
    xfclose(file,HERE);
}

void graph_cmp(char *path1,char *path2){
    FILE *f1 = xfopen(path1,"r",HERE);
    FILE *f2 = xfopen(path2,"r",HERE);

    char *buff_1 = NULL;
    char *buff_2 = NULL;
    size_t get1 = 0;
    size_t get2 = 0;

    int e1,e2;

    do{
        e1 = getline(&buff_1,&get1,f1);
        e2 = getline(&buff_2,&get2,f2);

        if(e1 == -1 || e2 == -1){
            if(e1 == e2){
                fprintf(stdout,"graphs eHEREvalent\n");
            }
            else{
                break;
            }
            return;
        }

        if(strcmp(buff_1,buff_2)!=0)break;

    } while(true);

    fprintf(stdout,"graphs not equivalent\n");
    fprintf(stdout,"%s\n%s\n",buff_1,buff_2);

    free(buff_1);
    free(buff_2);
    xfclose(f1,HERE);
    xfclose(f2,HERE);
    return;
}

inline void xgettimeofday(struct timeval *t, bool check_time, char *file, int line){
    if(check_time == false)return;
    if(gettimeofday(t,NULL)!=0){
        error("[gettimeofday]",file,line);
    }
}

inline double exctract_time(struct timeval start,struct timeval end,bool check_time){
    if(check_time == false)return -1;
    return (end.tv_sec - start.tv_sec)+((end.tv_usec - start.tv_usec)*1e-6);

}