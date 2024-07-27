#include "lib_pagerank.h"
#include "lib_supp.h"
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
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
    puts("-s\t\t enable sending signals SIGUSR1 (print top node at iteration x)");
}

inline void printGraphInfo(graph *g,FILE *stream,bool comment){
    if(!comment)
        fprintf(stream, "Number of nodes: %d \nNumber of dead-end nodes: %d\nNumber of valid arcs: %d\n", g->nodes, g->dead_count, g->edges);
    else
        fprintf(stream, "%%Number of nodes: %d \n%%Number of dead-end nodes: %d\n%%Number of valid arcs: %d\n", g->nodes, g->dead_count, g->edges);
}

void *calculate_pagerank(void *a){
    pagerank_thread *arg = (pagerank_thread *)a;
    int mio_index = -1;
    int mio_index2 = -1;
    double teleport = (1.0 - arg->dumping_factor) / ((double)arg->grafo->nodes);
    
    //variabile che uso per scambiare X_t con X_t+1
    double *temp;
    while(arg->exit == false){

        // === CALCOLO Y_t ===
        while(true){
            xpthread_mutex_lock(arg->vector_lock, HERE);
                mio_index = arg->index;
                arg->index += 1;
            xpthread_mutex_unlock(arg->vector_lock, HERE);

            if(mio_index >= arg->grafo->nodes)
                break;
            
            if(arg->grafo->out[mio_index] > 0)
                arg->Y[mio_index] = (*(arg->X_prec))[mio_index] / (double) arg->grafo->out[mio_index];

        }

        // === ATTENDO FINE CALCOLO Y_t ===
        xpthread_mutex_lock(arg->vector_lock, HERE);
            if(arg->sospesi_Y == arg->N_thread - 1){
                //printArrayd(arg->Y,arg->grafo->N);
                arg->index = 0;
                xpthread_cond_broadcast(arg->vector_cond, HERE);
            } 
            else {
                arg->sospesi_Y += 1;
                while (arg->index >= arg->grafo->nodes)
                    xpthread_cond_wait(arg->vector_cond, arg->vector_lock, HERE);
                arg->sospesi_Y -= 1;
            }
        xpthread_mutex_unlock(arg->vector_lock, HERE);

        // === FASE CALCOLO VETTORE X_t+1 ===

        while (true){
            
            xpthread_mutex_lock(arg->vector_lock, HERE);
                mio_index2 = arg->index2;
                arg->index2 += 1;
            xpthread_mutex_unlock(arg->vector_lock, HERE);
            
            if(mio_index2 >= arg->grafo->nodes)
                break;
            
            double somma = 0.0;
            inmap *obj = ((arg->grafo)->in)[mio_index2];
            
            if(obj != NULL){
                for(int i = 0; i < obj->length; i++){
                    int node = obj->vector[i];
                    somma += arg->Y[node];
                }
            }             
            (*(arg->X_vector))[mio_index2] = teleport + (arg->dumping_factor * somma) + (arg->dumping_factor / (double)(arg->grafo->nodes)) * arg->S_t;     
        }
        
        // === ATTENDO CALCOLO COMPONENTI X_t+1 ===
        double error = 0.0;
        xpthread_mutex_lock(arg->vector_lock, HERE);

            if(arg->sospesi_X == arg->N_thread - 1){
                // == verifica ERRORE ==

                for(int i = 0; i < arg->grafo->nodes; i++){
                    error+=fabs((*(arg->X_prec))[i] - (*(arg->X_vector))[i]);
                }
                
                if(error < arg->epsilon || *(arg->N_iter) == arg->maxiter - 1){
                    arg->exit = true;
                    arg->index2 = 0;
                }                 
                else {
                    // == ricalcolo S_t per nuova iterazione == 
                    arg->S_t = 0.0;
                    for(int i = 0; i < arg->grafo->dead_count; i++){ 
                        int nodo = arg->grafo->dead_end[i];
                        arg->S_t += (*(arg->X_vector))[nodo];
                    }
                    //printf("S_t= %f\n",arg->S_t);
                }
                double somma_Y;
                for(int i = 0; i < arg->grafo->nodes; i++){
                    somma_Y += arg->Y[i];
                }
                
                xpthread_mutex_lock(arg->signal_mutex, HERE);
                    temp = *(arg->X_prec);
                    *(arg->X_prec) = *(arg->X_vector);
                    *(arg->X_vector) = temp;

                    if(arg->exit){
                        free(*(arg->X_prec));
                        *(arg->X_prec) = NULL;
                    }
                    *(arg->N_iter) += 1;
                    arg->index2 = 0;
                xpthread_mutex_unlock(arg->signal_mutex, HERE);
                xpthread_cond_broadcast(arg->vector_cond, HERE);
            }
            else {
                arg->sospesi_X += 1;
                while(arg->index2 >= arg->grafo->nodes)
                    xpthread_cond_wait(arg->vector_cond, arg->vector_lock, HERE);
                arg->sospesi_X -= 1;
            }

        xpthread_mutex_unlock(arg->vector_lock, HERE);
    }
    pthread_exit(NULL);
}

double *pagerank(graph *g, double d, double eps, int maxiter, int taux, int *numiter){
     
    // allocazione vettori per le iterazioni
    double  *X_vector = xmalloc(g->nodes * sizeof(double), HERE);
        global_X_prec = xmalloc(g->nodes * sizeof(double), HERE);
    double *Y = xcalloc(g->nodes, sizeof(double), HERE);

    // popolamento vettori iterazioni
   
    for(int i = 0; i < g->nodes; i++){
        X_vector[i] = (1.0 / (double)g->nodes);
        (global_X_prec)[i] = (1.0 / (double)g->nodes);
    }
    
    double S_t = 0.0;
    int node = -1;

    /**
     * Calcolo S_t
     * -----------
     * S_t = somma dei valori dell'iterazione precedente dei dead end nodes
     * 
     */

    for(int i = 0; i < g->dead_count; i++){
        node = g->dead_end[i];
        S_t += global_X_prec[node];
    }

    //Conto un iterazione fatta
    // (*numiter) += 1;

    pthread_t tid[taux];
    pagerank_thread dati_pagerank;

    pthread_mutex_t vector_lock;
    pthread_cond_t vector_cond;
    xpthread_mutex_init(&vector_lock, HERE);
    xpthread_cond_init(&vector_cond, HERE);
    dati_pagerank.X_vector =    &X_vector;
    dati_pagerank.X_prec =      &global_X_prec;
    dati_pagerank.Y = Y;
    dati_pagerank.S_t = S_t;
    dati_pagerank.sospesi_Y = 0;
    dati_pagerank.sospesi_X = 0;
    dati_pagerank.index = 0;
    dati_pagerank.index2 = 0;
    dati_pagerank.epsilon = eps;
    dati_pagerank.dumping_factor = d;
    dati_pagerank.maxiter = maxiter;
    dati_pagerank.N_iter = numiter;
    dati_pagerank.N_thread = taux;
    dati_pagerank.exit = false;
    dati_pagerank.grafo = g;
    dati_pagerank.signal_mutex = &global_mutex;
    dati_pagerank.vector_lock = &vector_lock;
    dati_pagerank.vector_cond = &vector_cond;
    
    for(int i = 0; i < taux; i++){
        xpthread_create(&tid[i], calculate_pagerank, &dati_pagerank, HERE);
    }

    for(int i = 0; i < taux; i++){
        xpthread_join(tid[i], NULL, HERE);
    }

    *numiter = *(dati_pagerank.N_iter);

    free(Y);
    xpthread_mutex_destroy(&vector_lock, HERE);
    xpthread_cond_destroy(&vector_cond, HERE);    

    return X_vector;
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
 * are eHEREvalent
 * ---------------------------------------------------------
 * The second function is useful to check if, between parsing
 * versions, the same output is always produced
 */
void graph_save(char *path, graph *grph){
    FILE *file = xfopen(path,"w",HERE);
    //print brief graph info
    printGraphInfo(grph,file,true);
    //print first line
    fprintf(file,"%d %d %d\n",grph->nodes,grph->nodes,grph->edges,grph->dead_count);
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

    }while(true);

    fprintf(stdout,"graphs not eHEREvalent\n");
    fprintf(stdout,"%s%s",buff_1,buff_2);

    free(buff_1);
    free(buff_2);
    xfclose(f1,HERE);
    xfclose(f2,HERE);
    return;
}