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
#ifndef PARSING_H_
#define PARSING_H_

#include "utils.h"

/*
  @ Parametri: la struttura dati in cui salvare i valori estratti dal file di
                configurazione e il nome del file da aprire per estrarre i valori

  @ Funzione che viene utilizzata per riempire la struttura dati di configurazione chiamando
  le due funzioni GetStr e GetInt.
*/
void parsing(config * configurazione, char * config);


#endif
