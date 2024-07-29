#!/usr/bin/env python3
import argparse,os,psutil

"""
Utility to testbench a set of files or a batch of files
.......................................................
Produces two output files:
1. testbench.txt        = results of tests on each files
2. testbench_avg.txt    = time average of each file
.......................................................
Uses argparse to 
"""

def main(pagerank_attr:list,runs:int,file_path:list[str],flags:dict)->None:
    pass

if __name__=="__main__":
    parser = argparse.ArgumentParser(description="testbench.py")
    parser.add_argument("-t","--threads",help="thread count",type = int,default = psutil.cpu_count(logical=False))
    parser.add_argument("-e","--epsilon",help="[epsilon] default error threshold",type = float,default = 1e-7)
    parser.add_argument("-d","--dumping",help="dumping factor",type = float,default = 0.9)
    parser.add_argument("-k","--top_k",help="Top k nodes",type=int,default = 3)
    parser.add_argument("-m","--maxiter",help="Max iterations",type=int,default = 200)
    parser.add_argument("-s","--signal",help="Allow signal handler",action="store_false")
    parser.add_argument("-r","--runs",help="number of runs per file",type=int,default = 10)
    # file management parameters
    parser.add_argument("input_files", nargs='+', help="paths to input files")
    parser.add_argument("-p","--print",action = "store_false",help="print each test result to file")
    parser.add_argument("-v","--verbose",action="store_true",help="print all test results to stdout")
    parser.add_argument("-l","--log",type = str,help="custon testbench file",default="testbench")
    parser.add_argument("-V","--valgrind",help="execute tests with valgrind, 1 run, no time stats",action="store_true")
    parser.add_argument("--time",help="include time statistics in report",action="store_false")

    # Parse arguments
    args = parser.parse_args()

    flags = {
        "print":    args.print,
        "verbose":  args.verbose,
        "time":     args.time,
        "valgrind": args.valgrind,
        "log":      args.log
    }

    # Validate numeric arguments
    for arg in [args.threads, args.epsilon, args.dumping, args.top_k, args.maxiter, args.runs]:
        if arg < 0:
            raise ValueError(f"Parameter {arg} is malformed. It must be non-negative.")
    
    #check if files each file exists and it has access to be read
    #files that do not exist will be discarded
    files           = []
    pagerank_attr  = [args.signal, args.threads, args.epsilon, args.dumping, args.top_k, args.maxiter]

    for path in args.input_files:
        if not os.path.access(path,os.R_OK):
            print(f"No read permission on file/folder {path}")

        if os.path.isdir(path):
            for dirpath, _, filenames in os.walk(path):
                for filename in filenames:
                    file_path = os.path.join(dirpath, filename)
                    if os.access(file_path, os.R_OK):
                        files.append(file_path)

        elif os.path.isfile(path):
            files.append(path)
        # add support for symbolic links
    
    if len(files) == 0:
        print("No files provided had read rights")

    main(pagerank_attr,args.run,files,flags)
