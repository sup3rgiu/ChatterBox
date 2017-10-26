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
 * Matricola: 
 *
 */
 /**
  * @file MyPthread.c
  * @brief Funzioni di pthread con controlli per il valore restituito.
  */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "MyPthread.h"

/*
  Versione re-implementata della pthread_create con controlli
*/

void Pthread_create(pthread_t *t, const pthread_attr_t *attr, void * (* function)(), void *arg){
  if((pthread_create(t,attr, function ,(void*)arg))!=0){
    perror("Pthread_create: ");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della pthread_mutex_lock con controlli
*/
void Pthread_mutex_lock(pthread_mutex_t *mutex){
  int err=1;
  if((err=pthread_mutex_lock(mutex))!=0){
    perror("Pthread_mutex_lock:");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della pthread_mutex_unlock con controlli
*/
void Pthread_mutex_unlock(pthread_mutex_t *mutex){
  int err=1;
  if((err=pthread_mutex_unlock(mutex))!=0){
    perror("Pthread_mutex_lock:");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della pthread_cond_wait con controlli
*/
void Pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex){
  int err=1;
  if((err=pthread_cond_wait(cond, mutex))!=0){
    perror("Pthread_cond_wait:");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della pthread_cond_signal con controlli
*/
void Pthread_cond_signal(pthread_cond_t *cond){
  int err=1;
  if((err=pthread_cond_signal(cond))!=0){
    perror("Pthread_cond_signal:");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della pthread_cond_broadcast con controlli
*/
void Pthread_cond_broadcast(pthread_cond_t *cond){
  int err=1;
  if((err=pthread_cond_broadcast(cond))!=0){
    perror("Pthread_cond_signal:");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della Pthread_join con controlli
*/
void Pthread_join(pthread_t t, void * status){
  int err=1;
  if((err=pthread_join(t,status))!=0){
    perror("Pthread_cond_signal:");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della Pthread_attr_destroy con controlli
*/
void Pthread_attr_destroy(pthread_attr_t attr){
  int err=1;
  if((err=pthread_attr_destroy(&attr))!=0){
    perror("Pthread_cond_signal:");
    exit(EXIT_FAILURE);
  }
}

/*
  Versione re-implementata della Pthread_mutex_init con controlli
*/
void Pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t *restrict attr){
  int err=1;
  if((err=pthread_mutex_init(mutex,attr))!=0){
    perror("Pthread_cond_signal:");
    exit(EXIT_FAILURE);
  }
}
