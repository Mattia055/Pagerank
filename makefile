CC		= gcc -g
CFLAGS	= -Wall -Wextra -Wuninitialized -O3 -std=gnu99
LDLIBS	= -lm -lrt -pthread

# preprocessors definitions for testbench
TEST_DEFS	= -DSIGNAL_STREAM=stderr -DINFO_STREAM=stderr -DCHECK_TIME=true
# preprocessors definition for grid search
GRAPH_DEFS	= -DBUF_SIZE=4096 -DDYN_DEF=300 -DMUX_DEF=1009

# eseguibili da costruire
EXECS	= pagerank
OTHER	= testbench
LIB 	= ./src/

all: $(EXECS)
	rm -f *.o

lib_supp.o: $(LIB)lib_supp*
	$(CC) $(CFLAGS) -c $(LIB)lib_supp.c -o $@

lib_graph.o: $(LIB)lib_graph* $(LIB)lib_supp.h
	$(CC) $(CFLAGS) -c $(LIB)lib_graph.c -o $@

lib_pagerank.o:$(LIB)*.h $(LIB)lib_pagerank.c
	$(CC) $(CFLAGS) -c $(LIB)lib_pagerank.c -o $@

pagerank.o: pagerank.c $(LIB)*.h
	$(CC) $(CFLAGS) -c pagerank.c -o $@

pagerank: lib_supp.o lib_graph.o lib_pagerank.o pagerank.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

testbench.o: pagerank.c $(LIB)*.h
	$(CC) $(CFLAGS) $(TEST_DEFS) -c pagerank.c -o $@

testbench: lib_supp.o lib_graph.o lib_pagerank.o testbench.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
	@rm -f *.o

clean:
	rm -f $(EXECS) $(OTHER) *.o *.exe *log* *.zip *.val *vgcore*

