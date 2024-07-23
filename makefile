# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC		=gcc -g
CFLAGS	= -Wall -O -std=c11
LDLIBS	= -lm -lrt -pthread

# eseguibili da costruire
EXECS	= pagerank
LIB 	= ./src/


# di default make cerca di realizzare il primo target 
all: $(EXECS)
	rm -r *.o

# non devo scrivere il comando associato ad ogni target 
# perché il defualt di make in questo caso va bene

lib_supp.o: $(LIB)lib_supp*
	$(CC)  -c $(LIB)lib_supp.c -o $@

lib_graph.o: $(LIB)lib_graph* $(LIB)lib_supp.h
	$(CC)  -c $(LIB)lib_graph.c -o $@

lib_pagerank.o:$(LIB)*.h $(LIB)lib_pagerank.c
	$(CC)  -c $(LIB)lib_pagerank.c -o $@

pagerank.o: pagerank.c $(LIB)*.h
	$(CC)  -c pagerank.c -o $@

pagerank: lib_supp.o lib_graph.o lib_pagerank.o pagerank.o
	$(CC)  $(CFLAGS) $^ -o $@ $(LDLIBS)

# target che cancella eseguibili e file oggetto
clean:
	rm -f $(EXECS) *.o 

