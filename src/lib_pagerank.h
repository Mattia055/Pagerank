#ifndef LIBPGRK
#define LIBPGRK

#include "lib_graph.h"
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

extern double *global_X_prec;
extern pthread_mutex_t global_mutex;

void graph_save(char *path, graph *grph);

void graph_cmp(char *path1,char *path2);


typedef struct pagerank_thread {
    //doppi puntatori per i vettori delle iterazioni per fare lo swap
    double **X_vector;
    double **X_prec;
    double *Y;
    double S_t;
    int sospesi_Y;
    int sospesi_X;
    int index;
    int index2;
    double epsilon;
    double dumping_factor;
    int maxiter;
    int *N_iter;
    int N_thread;
    bool exit;
    graph *grafo;
    pthread_mutex_t *vector_lock;
    pthread_mutex_t *signal_mutex;
    pthread_cond_t *vector_cond;
} pagerank_thread;


void printHelp();

void printGraphInfo(graph *g, FILE *stream,bool comment);

void *calculate_pagerank(void *arg);

double *pagerank(graph *g, double d, double eps, int maxiter, int taux, int *numiter);


int *find_K_Max(double *ranks, int length,int k);

void printStats(double *ranks,int length,int iter_count,int max_iter,int k, FILE *stream);

/**
 * DEBUG ONLY
 */

void graph_save(char *path, graph *grph);

void graph_cmp(char *path1,char *path2);

#endif