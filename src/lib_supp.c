#include "lib_supp.h"

/*
 * Error function
 * --------------
 * Termina il processo stampando un
 * messaggio di errore associato contenente
 * [PID] processo, linea, file
 */
void error(const char *mess,char *file,int line){
    if(errno==0)
        fprintf(stderr,"==Process %d== file[%s] line[%d]: %s\n",getpid(),file,line,mess);
    else
        fprintf(stderr,"==Process %d== file[%s] line[%d]: %s |-| %s\n",getpid(),file,line,mess,strerror(errno));
    
    exit(EXIT_FAILURE);
}

void *xmalloc(size_t size,char *file,int line){
    void *ret = malloc(size);
    if(ret==NULL)
        error("[Bad malloc]",file,line);
    return ret;
}

void *xcalloc(size_t nmemb, size_t size, char *file,int line){
    void *ret = calloc(nmemb, size);
    if(ret==NULL)
        error("[Bad calloc]",file,line);
    return ret;
}

void *xrealloc(void *ptr,size_t size,char *file,int line){
    void *ret = realloc(ptr, size);
    if(ret==NULL)
        error("[Bad realloc]",file,line);
    return ret;
}

void *xreallocarray(void *ptr, size_t nmemb, size_t size, char *file, int line){
    void *ptr = reallocarray(ptr,nmemb,size);
    if(ptr == NULL){
        error("[Bad reallocarray]",file,line);
    }
}


FILE *xfopen(const char *path,const char *mode,char *file,int line){
    FILE *f=fopen(path,mode);
    if(f==NULL)
        error("[bad fopen]",file,line);
    return f;
}

void xfclose(FILE *f,char *file,int line){
    int e=fclose(f);
    if(e!=0)
        error("[Bad fclose]", file, line);
}


int xopen(char *path,int oflags,char *file,int line){
    int ret = open(path,oflags);
    if(ret<0)
        error("[Bad open]",file,line);
    return ret;
}

ssize_t xwrite(int fd, void *ptr,size_t n,char *file,int line){
    ssize_t ret = write(fd,ptr,n);
    if(ret==-1)
        error("[Bad write]",file,line);
    return ret;
}

ssize_t xread(int fd, void *ptr,size_t n,char *file,int line){
    ssize_t ret = read(fd,ptr,n);
    if(ret==-1)
        error("[Bad read]",file,line);
    return ret;
}

void xclose(int fd, char *file,int line){
    if(close(fd)!=0)
        error("[Bad Close]",file,line);
    }


void xsem_init(sem_t *sem,int pshared,int init,char *file,int line){
    if(sem_init(sem,pshared,init)!=0)
        error("[Bad sem_init]",file,line);   
}

void xsem_wait(sem_t *sem,char *file,int line){
    if(sem_wait(sem)==-1)
        error("[Bad sem_wait]",file,line);
}

void xsem_post(sem_t *sem,char *file,int line){
    if(sem_post(sem)==-1)
        error("[Bad sem_post]",file,line);
}

void xsem_close(sem_t *name,char *file,int line){
    if(sem_close(name)==-1)
        error("[Bad sem_close]",file,line);
}

void xsem_destroy(sem_t *sem, char *file,int line){
    if(sem_destroy(sem)!=0)
        error("[Bad sem_destroy]",file,line);
}

void xpthread_create(pthread_t *t, void *(*start_routine) (void *), void *args,char *file,int line){
    if(pthread_create(t,NULL,start_routine,args)!=0)
        error("[Bad pthread_create]",file,line);
}

void xpthread_join(pthread_t t, void **retval,char *file,int line){
    if(pthread_join(t,retval)!=0)
        error("[Bad pthread_join]",file,line);
}

void xpthread_mutex_init(pthread_mutex_t *l,char *file,int line){
    if(pthread_mutex_init(l,NULL)!=0)
        error("[Bad mutex_init]",file,line);
}

void xpthread_mutex_destroy(pthread_mutex_t *lock,char *file,int line){
    if(pthread_mutex_destroy(lock)!=0)
        error("[Bad mutex_destroy]",file,line);
}

void xpthread_mutex_lock(pthread_mutex_t *lock, char *file,int line){
    if(pthread_mutex_lock(lock)!=0)
        error("[Bad mutex_lock]",file,line);
}

void xpthread_mutex_unlock(pthread_mutex_t *lock,char *file,int line){
    if(pthread_mutex_unlock(lock)!=0)
        error("[Bad mutex_unlock]",file,line);
}

void xpthread_cond_init(pthread_cond_t *c,char *file,int line){
    if(pthread_cond_init(c,NULL)!=0)
        error("[Bad cond_init]",file,line);
}

void xpthread_cond_destroy(pthread_cond_t *cond,char *file,int line){
    if(pthread_cond_destroy(cond)!=0)
        error("[Bad cond_destroy]",file,line);
}

void xpthread_cond_wait(pthread_cond_t *c,pthread_mutex_t *l,char *file,int line){
    if(pthread_cond_wait(c,l)!=0)
        error("[Bad cond_wait]",file,line);
}

void xpthread_cond_signal(pthread_cond_t *c,char *file,int line){
    if(pthread_cond_signal(c)!=0)
        error("[Bad cond_signal]",file,line);
}

void xpthread_cond_broadcast(pthread_cond_t *c,char *file,int line){
    if(pthread_cond_broadcast(c)!=0)
        error("[Bad cond_broadcast]",file,line);
}

