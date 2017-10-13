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


#define _GNU_SOURCE 1
#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <limits.h>

#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

typedef struct config_{
  char * UnixPath;
  int MaxConnessioni;
  int ThreadInPool;
  int MaxMsgSize;
  int MaxFileSize;
  int MaxHistMsgs;
  char * DirName;
  char * StatFileName;
} config;



#define checkMU(r,c,e) \
if((r=c)==-1) {  return -1; }


void * Malloc(size_t size);




#endif
