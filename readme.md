# Opensource Pagerank Implementation
The algorithm works, taking an input file and parsing a graph. After the graph is parsed, pagerank is calculated as follow:
```
Aggiungere spiegazione calcolo
```

## Creazione del Grafo
IL parsing del grafo è una soluzione multithread, che a partire da un file .mtx produce una struttura dati di tipo grafo come segue:
```c
typedef struct graph{
    int nodes  //numero nodi
    int edges  //archi validi 
    int out[]  
    int dead_count //numero nodi dead-end
    int dead_end[] //array dei nodi dead end   
    inmap *in[]     
}graph;

/*
*Struct per liste di adiacenza
*/
typedef struct inmap{
    int length;
    int vector[];
}inmap;

``` 
Della struttura dati si devono precisare solo i due vettori:
- `inmap *in[]`: vettore che per ogni nodo (`in[i]`) memorizza una lista di nodi aventi un arco (valido) entrante nel nodo `i`
- `int out[]`: vettore di interi che per ogni nodo memorizza il numero di archi validi uscenti.

Dopo la definizione della struttura e la sua allocazione, si comincia a leggere dal file dispiegando un gruppo di threads `parser` che leggono da un buffer condiviso un arco alla volta.

> Questo probabilmente è il più grande bottleneck della performance del parsing. Il fatto di avere un buffer condiviso e un'inserimento disordinato degli archi, implica che si deve utilizzare una mutex.

Quello che sto ipotizzando è che probabilmente si potrebbe avere una migliore performance, eliminando il buffer condiviso e instaurando un buffer più una lista (stile `LockedArrays` di java), con ogni thread. 

Supponiamo 8 thread (uno per core)


