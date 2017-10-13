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

#ifndef CODA_H_
#define CODA_H_


typedef struct myqueue{
  int inizio;
  int fine;
  int dim;
  int max;
  int * elem;
} coda;


/*
  @ Parametri: un intero che indica la dimensione massima della coda

  @ Return: viene restituita la coda creata

  @ Viene creata una nuova coda (struttura definita sopra) in cui la dimensione
    dell'array di interi è definita dall'interno passato come parametro.
*/
coda * creacoda(int dim_max);


/*
  @ Parametri:  la coda in cui inserire l'intero
                l'intero da inserire in coda

  @ Inserisce un nuovo elemento all'interno della coda.
    L'intero viene inserito in fondo alla coda.
*/
int inserimento(coda * queue, int var);


/*
  @ Parametri: la coda da cui estrarre il primo elemento
  @ Return: Il primo elemento della coda

  @ Rimuove il primo elemento della coda passata come parametro e restituisce
    l'intero contenuto.
*/
int eliminaElemento(coda * queue);


/*
  @ Parametri: la coda che vogliamo cancellare dalla memoria

  @ Toglie dalla memoria la coda passata come parametro
*/
void EliminaCoda(coda * queue);


/*
  @ Parametri: La coda di cui vogliamo mostrare il contenuto

  @ Stampa il contenuto dell'intera coda
*/
void stampacoda(coda * queue);




/*
  @ Parametri: la coda da cui estrarre l'elemento
  @ Return: l'indice che mi dice dove si trova l'elemento da estrarre

  @ Rimuove l'elemento index della coda passata come parametro e restituisce
    l'intero contenuto.
*/
int eliminaElementoAt(coda * queue, int index);


#endif
