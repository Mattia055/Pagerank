#include "lib_pagerank.h"
#include "lib_supp.h"
#include "stdio.h"

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

void printGraphInfo(graph *g,FILE *stream){
    fprintf(stream, "Number of nodes: %d \nNumber of dead-end nodes: %d\nNumber of valid arcs: %d\n", g->nodes, g->dead_count, g->edges);
}