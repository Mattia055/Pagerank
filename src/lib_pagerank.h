#ifndef LIBPGRK
#define LIBPGRK

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>

#include "lib_graph.h"

extern double *X_previous;
extern pthread_mutex_t signal_mux;

void graph_save(char *path, graph *grph);

void graph_cmp(char *path1,char *path2);

void printHelp(const char *name);

void printGraphInfo(graph *g, FILE *stream,bool comment);

void *calculate_pagerank(void *arg);

double *pagerank(graph *g, double d, double eps, int maxiter, int taux, int *numiter);

int *find_K_Max(double *ranks, int length,int k);

void printStats(double *ranks,int length,int iter_count,int max_iter,int k, FILE *stream);

typedef struct pagerank_shared_attr {
    //doppi puntatori per i vettori delle iterazioni per fare lo swap
    double          **X_current;
    double          **X_previous;
    double          *Y;
    double          S_t;
    double          S_t_shared;
    double          epsilon;
    double          error;
    double          dumping_factor;
    graph           *grph;
    int             max_iter;
    bool            do_X;
    bool            do_Y;
    bool            exit;
    int             waiting_on_Y;
    int             waiting_on_X;
    int             *curr_iter;
    int             thread_count;
    pthread_mutex_t *cond_mux;
    pthread_mutex_t *shared_mux;
    pthread_cond_t  *cond;
} pagerank_shared_attr;

typedef struct pagerank_thread_attr{
    int interval_start;
    int interval_end;
    pagerank_shared_attr *shared;
}pagerank_thread_attr;

double *pagerank(graph *grph, double dumping, double eps, int max_iter, int thread_count, int *iter_count);

void *pagerank_routine(void *);

typedef struct sig_handler_attr{
    int             *nodes;
    int             *iter_count;
    double          **X_previous;
    pthread_mutex_t *shared_mux;
    FILE            *signal_stream;

}sig_handler_attr;

double find_max(double *arr,int length);

void *signal_handler_routine(void *);

/**
 * DEBUG ONLY
 */

void graph_save(char *path, graph *grph);

void graph_cmp(char *path1,char *path2);

void xgettimeofday(struct timeval *t, bool check_time, char *file, int line);

double exctract_time(struct timeval start,struct timeval end,bool check_time);

#endif