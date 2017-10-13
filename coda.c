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
  * @file coda.c
  * @brief Funzioni necessarie per la creazione e la gestione di una coda
  */


#include <stdio.h>
#include <stdlib.h>
#include "coda.h"
#include "utils.h"

/*
  @ Parametri: un intero che indica la dimensione massima della coda

  @ Return: viene restituita la coda creata

  @ Viene creata una nuova coda (struttura definita sopra) in cui la dimensione
    dell'array di interi è definita dall'interno passato come parametro.
*/
coda * creacoda(int dim_max){
  coda * new=(coda*)Malloc(sizeof(coda));
  new->inizio=0;
  new->fine=0;
  new->dim=0;
  new->max=dim_max;
  new->elem=(int*)Malloc(sizeof(int)*dim_max);
  int i=0;
  for(i=0;i<dim_max;i++){
    new->elem[i]=0;
  }
  return new;
}

/*
  @ Parametri:  la coda in cui inserire l'intero
                l'intero da inserire in coda

  @ Inserisce un nuovo elemento all'interno della coda.
    L'intero viene inserito in fondo alla coda.
*/
int inserimento(coda * queue, int var){

  if(queue->dim==queue->max){
    printf("dim %d MAX %d",queue->dim,queue->max);
    return -1;
  }
  else{
    queue->elem[queue->fine]=var;
    queue->fine=(queue->fine+1)%queue->max;
    queue->dim++;

  }


  return 0;
}



/*
  @ Parametri: la coda da cui estrarre il primo elemento
  @ Return: Il primo elemento della coda

  @ Rimuove il primo elemento della coda passata come parametro e restituisce
    l'intero contenuto.
*/
int eliminaElemento(coda * queue){

  if(queue->dim==0){
    return -1;
  }

  else{
    int var=0;
    var=queue->elem[queue->inizio];
    queue->dim--;
    queue->inizio=(queue->inizio + 1)%queue->max;
    return var;
  }



}

/*
  @ Parametri: la coda da cui estrarre l'elemento
  @ Return: l'indice che mi dice dove si trova l'elemento da estrarre

  @ Rimuove l'elemento index della coda passata come parametro e restituisce
    l'intero contenuto.
*/
int eliminaElementoAt(coda * queue, int index){

  int var=0;
  var=queue->elem[index];
  queue->dim--;
  return var;
}

/*
  @ Parametri: la coda che vogliamo cancellare dalla memoria

  @ Toglie dalla memoria la coda passata come parametro
*/
void EliminaCoda(coda * queue){
  free(queue->elem);
  free(queue);
}


/*
  @ Parametri: La coda di cui vogliamo mostrare il contenuto

  @ Stampa il contenuto dell'intera coda
*/
void stampacoda(coda * queue){
  int i=0;
  for(i=0;i<queue->max;i++){
    printf("%d: %d\n",i,queue->elem[i]);

  }
}
