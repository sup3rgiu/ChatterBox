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
  * @file codamessaggi.c
  * @brief Funzioni necessarie alla gestione e alla creazione di una coda di messaggi
            inviati ad un client non connesso
  */


#include <stdio.h>
#include <stdlib.h>
#include "codamessaggi.h"
#include "utils.h"
#include "message.h"

/*
  @ Parametri: un intero che indica la dimensione massima della coda

  @ Return: viene restituita la coda messaggi creata

  @ Viene creata una nuova coda (struttura definita sopra) in cui la dimensione
    dell'array di interi è definita dall'interno passato come parametro.
*/
codamessaggi * creacodaMessaggi(int dim_max){
  codamessaggi * new=(codamessaggi*)malloc(sizeof(codamessaggi));
  new->inizio=0;
  new->fine=0;
  new->dim=0;
  new->max=dim_max;
  new->message=(message_t*)malloc(sizeof(message_t)*dim_max);

  return new;
}

/*
  @ Parametri:  la coda in cui inserire l'intero
                l'intero da inserire in coda

  @ Inserisce un nuovo elemento all'interno della coda.
    Il messaggio che devo inserire viene posto alla fine della coda
*/
void inserimentoMessaggi(codamessaggi * queue, message_t message){
  if(queue->dim>=queue->max){
    message_t bck;
    setHeader(&bck.hdr, message.hdr.op, message.hdr.sender);
    setData(&bck.data,message.data.hdr.receiver,message.data.buf,message.data.hdr.len);

    queue->message[queue->inizio]=bck;
    queue->fine=(queue->fine+1)%(queue->max);
    if(queue->fine!=queue->inizio){
      //printf("fine:  %d  inizio:  %d",queue->fine, queue->inizio)
    }
    queue->inizio=(queue->inizio+1)%(queue->max);
  }
  else{
    message_t bck;
    setHeader(&bck.hdr, message.hdr.op, message.hdr.sender);
    setData(&bck.data,message.data.hdr.receiver,message.data.buf,message.data.hdr.len);

    queue->message[queue->fine]=bck;
    queue->dim++;
    queue->fine=(queue->fine+1)%(queue->max);
  }


}

/*
  @ Parametri: la coda da cui estrarre il primo elemento
  @ Return: Il primo elemento della coda

  @ Rimuove il primo elemento della coda passata come parametro e restituisce
    il messaggio contenuto.
*/
message_t eliminaElementoMessaggi(codamessaggi * queue){
  message_t bck=queue->message[queue->inizio];

  queue->dim--;
  queue->inizio=(queue->inizio + 1)%(queue->max);
  return bck;
}



/*
  @ Parametri: la coda da cui estrarre l'elemento
  @ Return: l'indice che mi dice dove si trova l'elemento da estrarre

  @ Rimuove l'elemento index della coda passata come parametro e restituisce
    il messaggio contenuto
*/
message_t eliminaElementoMessaggiAt(codamessaggi * queue, int index){

  message_t var;
  var=queue->message[index];
  queue->dim--;
  return var;
}

/*
  @ Parametri: la coda che vogliamo cancellare dalla memoria

  @ Toglie dalla memoria la coda passata come parametro
*/
void EliminaCodaMessaggi(codamessaggi * queue){

  free(queue->message);
  free(queue);
}


/*
  @ Parametri: La coda di cui vogliamo mostrare il contenuto

  @ Stampa il contenuto dell'intera coda
*/
void stampacodaMessaggi(codamessaggi * queue){
  int i=0;
  for(i=0;i<queue->max;i++){
    printf("%d: %s\n",i,(char*)((queue->message)[i].data.buf));

  }
}
