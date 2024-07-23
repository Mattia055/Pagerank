#ifndef LIB_SUPP
#define LIB_SUPP

#include <stddef.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>



/**
 * ### Error Display Function
 * ----------------------
 * Default: EXIT_FAILURE on process
 * printing basic information on the error
 * and the location of it
 */
void error(const char *mess,char *file,int line);

/**
 * ### Memory Management Functions
 * ---------------------------
 */
void *xmalloc(size_t size,char *file,int line);

void *xcalloc(size_t nmemb, size_t size, char *file,int line);

void *xrealloc(void *ptr,size_t size,char *file,int line);

void *xreallocarray(void *ptr, size_t nmemb, size_t size, char *file, int line);

/**
 * ### File Streams
 * ------------
 */
FILE *xfopen(const char *path,const char *mode,char *file,int line);

void xfclose(FILE *f,char *file,int line);

/**
 * ### File Descriptors
 * ----------------
 */
int xopen(char *path,int oflags,char *file,int line);

ssize_t xwrite(int fd, void *ptr,size_t n,char *file,int line);

ssize_t xread(int fd, void *ptr,size_t n,char *file,int line);

void xclose(int fd, char *file,int line);

ssize_t readn(int fd,void *ptr,size_t n,char *file,int line);

ssize_t writen(int fd,void *ptr,size_t n,char *file,int line);

/**
 * ### Un-named POSIX Semaphores
 * -------------------------
 */

void xsem_init(sem_t *sem,int pshared,int init,char *file,int line);

void xsem_wait(sem_t *sem,char *file,int line);

void xsem_post(sem_t *sem,char *file,int line);

void xsem_close(sem_t *name,char *file,int line);

void xsem_destroy(sem_t *sem,char *file,int line);

/**
 * ### Pthread Threads
 * ---------------
 */

void xpthread_create(pthread_t *t, void *(*start_routine) (void *), void *args,char *file,int line);

void xpthread_join(pthread_t t, void **retval,char *file,int line);

/**
 * ### Pthread Mutexes
 * ---------------
 */

void xpthread_mutex_init(pthread_mutex_t *l,char *file,int line);

void xpthread_mutex_destroy(pthread_mutex_t *lock,char *file,int line);

void xpthread_mutex_lock(pthread_mutex_t *lock, char *file,int line);

void xpthread_mutex_unlock(pthread_mutex_t *lock,char *file,int line);

/**
 * ### Condition Variables
 * -------------------
 */

void xpthread_cond_init(pthread_cond_t *c,char *file,int line);

void xpthread_cond_destroy(pthread_cond_t *cond,char *file,int line);

void xpthread_cond_wait(pthread_cond_t *c,pthread_mutex_t *l,char *file,int line);

void xpthread_cond_signal(pthread_cond_t *c,char *file,int line);

void xpthread_cond_broadcast(pthread_cond_t *c,char *file,int line);

/**
 * ### Logging module
 * ------------------
 * Implementation of basic interface that lets you
 * keep track of log files
 */

#endif