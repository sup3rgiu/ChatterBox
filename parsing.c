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
 * @file parsing.c
 * @brief Funzioni per il parsing del file di configurazione
 */

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include <string.h>
#include "parsing.h"

#define N 1024

/*
@ Parametri: la struttura dati in cui salvare i valori estratti dal file di
configurazione e il nome del file da aprire per estrarre i valori

@ Funzione che viene utilizzata per riempire la struttura dati di configurazione chiamando
le due funzioni GetStr e GetInt.
*/
void parsing(config * configurazione, char * config){

  int trovati[8]={0,0,0,0,0,0,0,0};

  FILE *fp;
  if((fp=fopen(config, "r"))==NULL){
    perror("Errore\n");
    exit(0);
  }

  int i=0;
  int j=0;
  int x=0;
  char buf[N];
  while((fgets(buf,N,fp))!=NULL){


    char nome[N]="";
    char valore[N]="";
    char carattere=buf[0];
    int val=0;
    j=0;
    x=0;
    for(i=0;carattere!='\n' && carattere!='\0' && carattere!=EOF ;i++){
      carattere=buf[i];

      if(carattere=='#'|| carattere== '\n' || carattere=='\0' || carattere==EOF)
        break;
      if(carattere=='='){
        val=1;
        continue;
      }
      if(carattere==' ' || carattere==9){
        continue;
      }


      if(val==1){
        valore[j]=carattere;
        j++;
      }
      else{
        nome[x]=carattere;
        x++;
      }

    }
    if(x>0){
      nome[x]='\0';
      valore[j]='\0';

      if(strcmp(nome,"UnixPath")==0){
        configurazione->UnixPath=Malloc(sizeof(char)+strlen(valore));
        strncpy(configurazione->UnixPath, valore, strlen(valore)+sizeof(char));
        trovati[0]++;
        continue;
      }
      if(strcmp(nome,"DirName")==0){
        configurazione->DirName=Malloc(sizeof(char)+strlen(valore));
        strncpy(configurazione->DirName, valore, strlen(valore)+sizeof(char));
        trovati[1]++;
        continue;
      }
      if(strcmp(nome,"StatFileName")==0){
        configurazione->StatFileName=Malloc(sizeof(char)+strlen(valore));
        strncpy(configurazione->StatFileName, valore, strlen(valore)+sizeof(char));
        trovati[2]++;
        continue;
      }
      if(strcmp(nome,"MaxConnections")==0){
        configurazione->MaxConnessioni=atoi(valore);
        trovati[3]++;
        continue;
      }
      if(strcmp(nome,"ThreadsInPool")==0){
        configurazione->ThreadInPool=atoi(valore);
        trovati[4]++;
        continue;

      }
      if(strcmp(nome,"MaxMsgSize")==0){
        configurazione->MaxMsgSize=atoi(valore);
        trovati[5]++;
        continue;
      }
      if(strcmp(nome,"MaxFileSize")==0){
        configurazione->MaxFileSize=atoi(valore);
        trovati[6]++;
        continue;

      }
      if(strcmp(nome,"MaxHistMsgs")==0){
        configurazione->MaxHistMsgs=atoi(valore);
        trovati[7]++;
        continue;

      }

    }


  }

  fclose(fp);
}
