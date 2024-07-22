#include "./src/lib_supp.h"

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

int main(int argc, char *argv[]){
    int k = 3;
    int m = 100;
    double d = 0.9;
    double e = 1e-7;
    int threads = 3;
    //turned true for testing
    bool signal = false;
    char *infile;
    
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
        puts("usage: ./pagerank [-h] [-k K] [-m M] [-d D] [-e E] [-t T] [[infile]]");
        return -1;
    }

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



}