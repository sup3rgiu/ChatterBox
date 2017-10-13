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
 /**
  * @file thread_pool.c
  * @brief Funzioni dedicate alla creazione del pool di thread
  */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "thread_pool.h"
#include "MyPthread.h"
#include "utils.h"


/*
  @ Parametri: un intero che mi dice il numero di thread nel pool
  @ Return: la nuova struttura contenente i nuovi thread se tutto è andato a buon fine.

  @ Inizializza un nuovo pool di thread.
*/
poolThread poolCreate(int n){
  poolThread new=(poolThread)malloc(sizeof(my_PoolThread));
  new->num=n;
  new->attivi=0;


  new->thread=(pthread_t *)malloc(sizeof(pthread_t)*n);
  if(new->thread==NULL){
    perror("Impossibile eseguire la Malloc in poolCreate\n");
    exit(EXIT_FAILURE);
  }
  return new;
}



/*
  @ Parametri:  Un intero che mi dice il numero di thread nel pool
                Il pool di thread in cui vogliamo inizializzare il thread
                La funzione da inserire nella Pthread_create
  @ Return: la nuova struttura contenente i nuovi thread se tutto è andato a buon fine.

  @ Crea i thread presenti all'interno del pool
    eseguendo le chiamate pthread_create e assegnando ai thread la
    funzione da svolgere
*/

void poolInit(int n, poolThread pool, void * (*extract)(void *), thread_param ** param){
  int i=0;


  for(i=0;i<n;i++){
    //pthread_create(&(pool->thread[i]), NULL,(void*) &Hello,  &array[i]);
    Pthread_create(&(pool->thread[i]), NULL, extract, (void*) param[i]);
  }

}

/*
  @ Parametri: il pool di thread che vogliamo cancellare

  @ Cancella la memoria occupata dalla struttura che contiene il pool di thread
*/
void poolDelete(poolThread thread){
  free(thread->thread);
  free(thread);
}
