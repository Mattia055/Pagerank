#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <bits/sigaction.h>

#include "./src/lib_supp.h"
#include "./src/lib_graph.h"
#include "./src/lib_pagerank.h"

#define _GNU_SOURCE

#ifndef SIGNAL_STREAM
#define SIGNAL_STREAM stderr
#endif

#ifndef INFO_STREAM
#define INFO_STREAM stdout
#endif

#ifndef CHECK_TIME
#define CHECK_TIME false
#endif

/**
 * global variables shared with signal handler
 */
double *X_previous;
pthread_mutex_t signal_mux;

int main(int argc, char *argv[])
{
    struct timeval start,end,parse_start,parse_end,page_start,page_end;
    /**Time measure struct */
    xgettimeofday(&start,CHECK_TIME,HERE);

    int k = 3;
    int m = 100;
    double d = 0.9;
    double e = 1e-7;
    int threads = 3;
    // stored true
    bool signal = false;

    /**
     * Parameter parsing from argv
     */
    int opt;
    while ((opt = getopt(argc, argv, "shk:m:d:e:t:")) != -1)
    {
        switch (opt)
        {
        case 'k':
            k = atoi(optarg);
            break;
        case 'm':
            m = atoi(optarg);
            break;
        case 'd':
            d = atof(optarg);
            break;
        case 'e':
            e = atof(optarg);
            break;
        case 't':
            threads = atoi(optarg);
            break;
        case 's':
            signal = false;
            break;
        case 'h':
            printHelp(argv[0]);
            exit(EXIT_SUCCESS);
        default: /* '?' */
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc)
    {
        puts("[pagerank] no input file");
        puts("usage: ./pagerank [-h] [-k K] [-m M] [-d D] [-e E] [-t T] <infile>");
        return -1;
    }

    char *infile = argv[optind];

    int iter_count = 0;

    int graph_nodes = -1;

    pthread_t signal_tid;

    /**
     * Setup thread that handles all signals
     *
     * Masks signals of main thread and delegates
     * their handling (via sigwait) to an auxiliary thread;
     */
    if (signal)
    {
        sigset_t local_mask;
        sigfillset(&local_mask);
        sig_handler_attr handler_attr;
        handler_attr.signal_stream  = SIGNAL_STREAM;
        handler_attr.X_previous     = &X_previous;
        handler_attr.shared_mux     = &signal_mux;
        handler_attr.nodes          = &graph_nodes;
        handler_attr.iter_count     = &iter_count;
        pthread_sigmask(SIG_SETMASK,&local_mask,NULL);
        xpthread_create(&signal_tid,signal_handler_routine,&handler_attr,HERE);
    }

    /**
     * start multithread
     * parsing of mtx file to graph
     * ----------------------------
     * calls graph_create(char *) that returns
     * a graph struct;
     */

    xgettimeofday(&parse_start,CHECK_TIME,HERE);
    graph *g    = graph_parse(infile, threads,CHECK_TIME);
    xgettimeofday(&parse_end,CHECK_TIME,HERE);

    printGraphInfo(g,INFO_STREAM, false);

    xgettimeofday(&page_start,CHECK_TIME,HERE);
    double *ranks = pagerank(g, d, e, m, threads, &iter_count);
    xgettimeofday(&page_end,CHECK_TIME,HERE);

    printStats(ranks, g->nodes, iter_count, m, k, INFO_STREAM);

    graph_destroy(g);
    free(ranks);

    if(signal){
        pthread_kill(signal_tid,SIGUSR2);
        xpthread_join(signal_tid,NULL,HERE);
    }
    xgettimeofday(&end,CHECK_TIME,HERE);

    /**
     * Print Time Stats
     */
    if(CHECK_TIME){
        fprintf(stderr,"\n--------------------\nTime Stats: %s\n--------------------\n",infile);
        fprintf(stderr,"parsing\ttime\t\t%.6f sec\n",exctract_time(parse_start,parse_end,CHECK_TIME));
        fprintf(stderr,"compute\ttime\t\t%.6f sec\n",exctract_time(page_start,page_end,CHECK_TIME));
        fprintf(stderr,"total\ttime\t\t%.6f sec\n",exctract_time(start,end,CHECK_TIME));
    }
    return 0;
}