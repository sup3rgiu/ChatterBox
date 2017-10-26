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


#ifndef CODA_MESSAGGI_H_
#define CODA_MESSAGGI_H_
#include "message.h"


typedef struct queuemessaggi{
  int inizio;
  int fine;
  int dim;
  int max;
  message_t * message;
} codamessaggi;


/*
  @ Parametri: un intero che indica la dimensione massima della coda

  @ Return: viene restituita la coda messaggi creata

  @ Viene creata una nuova coda (struttura definita sopra) in cui la dimensione
    dell'array di interi è definita dall'interno passato come parametro.
*/
codamessaggi * creacodaMessaggi(int dim_max);


/*
  @ Parametri:  la coda in cui inserire l'intero
                l'intero da inserire in coda

  @ Inserisce un nuovo elemento all'interno della coda.
    Il messaggio che devo inserire viene posto alla fine della coda
*/
void inserimentoMessaggi(codamessaggi * queue, message_t messaggio);


/*
  @ Parametri: la coda da cui estrarre il primo elemento
  @ Return: Il primo elemento della coda

  @ Rimuove il primo elemento della coda passata come parametro e restituisce
    il messaggio contenuto.
*/
message_t eliminaElementoMessaggi(codamessaggi * queue);





/*
  @ Parametri: la coda che vogliamo cancellare dalla memoria

  @ Toglie dalla memoria la coda passata come parametro
*/
void EliminaCodaMessaggi(codamessaggi * queue);




/*
  @ Parametri: La coda di cui vogliamo mostrare il contenuto

  @ Stampa il contenuto dell'intera coda
*/
void stampacodaMessaggi(codamessaggi * queue);




/*
  @ Parametri: la coda da cui estrarre l'elemento
  @ Return: l'indice che mi dice dove si trova l'elemento da estrarre

  @ Rimuove l'elemento index della coda passata come parametro e restituisce
    il messaggio contenuto
*/
message_t eliminaElementoMessaggiAt(codamessaggi * queue, int index);


#endif
