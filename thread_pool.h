/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 * Si dichiara che il programma è, in ogni sua parte,
 * opera originale dello studente.
 *
 * Studente: Luca Corbucci
 * Matricola: 516450
 *
 */

#if !defined(THREAD_POOL_H_)
#define THREAD_POOL_H_
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <sys/select.h>

typedef struct my_thread{
  int id;
  pthread_t thread;
} my_thread;

typedef struct my_pool{
  int num;
  int attivi;
  pthread_t *thread;
} my_PoolThread;



typedef struct thread_param_{
  int  fd_num;
  fd_set * set;
} thread_param;

typedef my_PoolThread * poolThread;

/*
  @ Parametri: un intero che mi dice il numero di thread nel pool
  @ Return: la nuova struttura contenente i nuovi thread se tutto è andato a buon fine.

  @ Inizializza un nuovo pool di thread.
*/
poolThread poolCreate(int n);

/*
  @ Parametri:  Un intero che mi dice il numero di thread nel pool
                Il pool di thread in cui vogliamo inizializzare il thread
                La funzione da inserire nella Pthread_create
  @ Return: la nuova struttura contenente i nuovi thread se tutto è andato a buon fine.

  @ Crea i thread presenti all'interno del pool
    eseguendo le chiamate pthread_create e assegnando ai thread la
    funzione da svolgere
*/
void poolInit(int n, poolThread pool, void *(*extract)(void *), thread_param ** param);


/*
  @ Parametri: il pool di thread che vogliamo cancellare

  @ Cancella la memoria occupata dalla struttura che contiene il pool di thread
*/
void poolDelete(poolThread thread);

#endif
