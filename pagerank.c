#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "./src/lib_supp.h"
#include "./src/lib_graph.h"
#include "./src/lib_pagerank.h"

#define _GNU_SOURCE

#define HERE __FILE__,__LINE__

/**
 * global variables shared with signal handler
 */
double *X_previous;
pthread_mutex_t signal_mux;

int main(int argc, char *argv[]){
    int k = 3;
    int m = 100;
    double d = 0.9;
    double e = 1e-7;
    int threads = 3;
    //turned true for testing
    bool signal = false;

    /**
     * Parameter parsing from argv
     */
    int opt;
    while ((opt = getopt(argc, argv, "shk:m:d:e:t:")) != -1) {
        switch (opt) {
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
                signal = true;
                break;
            case 'h':
                printHelp();
                exit(EXIT_SUCCESS);
            default: /* '?' */
                exit(EXIT_FAILURE);
        }
    }

    if(optind >= argc){
        puts("[pagerank] no input file");
        puts("usage: ./pagerank [-h] [-k K] [-m M] [-d D] [-e E] [-t T] <infile>");
        return -1;
    }

    char *infile = argv[optind];

    int iter_count = 0;
    

    /**
     * Setup thread that handles all signals
     * 
     * Masks signals of main thread and delegates
     * their handling (via sigwait) to an auxiliary thread;
     */
    if(signal){
        sigset_t local_mask;
        sigfillset(&local_mask);
        //DEBUG: create thread;
    }

    /**
     * start multithread 
     * parsing of mtx file to graph
     * ----------------------------
     * calls graph_create(char *) that returns
     * a graph struct;
     */

    graph *g = graph_parse(infile,threads);

    printGraphInfo(g,stdout,false);
    double *ranks = pagerank(g, d, e, m, threads, &iter_count);

    printStats(ranks,g->nodes,iter_count,m,k,stdout);

    graph_destroy(g);
    free(ranks);

    return 0;

}