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
  * @file utils.c
  * @brief Funzioni utili per il progetto.
  */


#include <stdio.h>
#include <stdlib.h>
#include "utils.h"


/*
  @ Parametri: La quantità di spazio da allocare
  @ Return: il puntatore all'area di memoria allocata

  @ Allora una quantità di memoria definita dal parametro controllando
    che la memoria sia stata effettivamente allocata
*/
void * Malloc(size_t size){
  void *m=malloc(size);
  if(m==NULL){
    fprintf(stderr, "Errore allocando memoria");
    exit(EXIT_FAILURE);
  }
  else
  return m;
}
