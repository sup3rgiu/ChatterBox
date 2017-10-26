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
  * @file connections.c
  * @brief Funzioni necessarie per la comunicazione tra server e client
  */


#include "message.h"
#include "connections.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"


/**
Prende come parametro il path del socket, il numero massimo di tentativi di retry e
il tempo di attesa tra due retry.
La funzione crea un nuovo socket di tipo AF_UNIX ed esegue la connect per un massimo di ntimes
volte. Se in una di queste connect riesce a connettersi al socket allora restituisce al chiamante
il socket creato con la connessione aperta.
*/
int openConnection(char* path, unsigned int ntimes, unsigned int secs){
  // File descriptor del nuovo socket
  int fd_client;
  struct sockaddr_un ser;

  socklen_t serlen;
  ser.sun_family=AF_UNIX;
  strncpy(ser.sun_path, path, UNIX_PATH_MAX);

  serlen=sizeof(ser);

  fd_client=socket(AF_UNIX, SOCK_STREAM, 0);
  int i=0;
  for(i=0;i<ntimes;i++){
    if(connect(fd_client,(struct sockaddr *) &ser, serlen)==-1){
      if(errno==ENOENT)
      sleep(secs);
      else{
        return -1;
      }
    }
    else{
      return fd_client;
    }
  }
  return -1;
}

// -------- server side -----
/**

Funzione che prende come parametri il descrittore della connessione e l'header in cui
andrò a scrivere quello che estraggo dal socket.
Viene fatta una sola read con cui estraggo l'header del messaggio.
Se va tutto bene allora restituisco 0, altrimenti -1,
*/
int readHeader(long connfd, message_hdr_t *hdr){
  int x=0;
  int r=0;

  if(connfd<0 || hdr==NULL){
    return -1;
  }

  // Read: estrazione dell'header del messaggio
  // Prima estraggo l'operazione che deve essere effettuata dal server
  message_hdr_t newhdr;
  r=read(connfd,&newhdr.op,sizeof(op_t));
  if(r==0){
    //printf("chiuso  \n");
    return 1;
  }
  checkMU(x, r, "Prima read di readHeader");
  hdr->op=newhdr.op;

  // Poi estraggo il nome del sender del messaggio
  r=read(connfd,&newhdr.sender,MAX_NAME_LENGTH+1);
  checkMU(x, r, "Seconda read di readHeader");
  if(r==0){
    return 1;
  }
  strncpy(hdr->sender,newhdr.sender,strlen(newhdr.sender)+1);


  return 0;
}

/**
Funzione che prende come parametri il descrittore della connessione e la struttura dati
message_data_t in cui memorizzo parte dell'informazione del message_t.
Vengono fatte due read, con la prima estraggo il message_data_hdr_t e lo inserisco all'interno
della struttura passata come parametro. Poi faccio la seconda read con cui vado a leggere
il buffer la cui lunghezza è contenuta nella struttura letta con la prima read.
*/
int readData(long fd, message_data_t *data){
  int r=0;
  int x=0;

  if(fd<0 || data==NULL){
    return -1;
  }

  //Prima read: estrazione del message_data_hdr_t
  // Prima estraggo il destinatario e poi la lunghezza del buffer
  message_data_hdr_t newhdr_data;
  r=read(fd,&(newhdr_data.receiver),MAX_NAME_LENGTH+1);
  checkMU(x, r, "Prima read di readData");
  if(r==0){
    return 1;
  }

  r=read(fd,&(newhdr_data.len),sizeof(unsigned int));
  checkMU(x, r, "Seconda read di readData");
  if(r==0){
    return 1;
  }

  data->hdr=newhdr_data;

  int leggere=newhdr_data.len;
  data->buf=malloc(leggere*sizeof(char));
  char * scrivere=data->buf;

  if(scrivere==NULL || leggere==0)
  return 0;

  //Seconda read: estrazione del buffer
  while(leggere>0){
    r=read(fd,scrivere,leggere);
    checkMU(x, r, "Terza read di readData");
    if(r==0){
      return 1;
    }
    leggere-=r;
    scrivere+=r;
  }

  return 0;
}

/**
Funzione che prende come parametri il descrittore della connessione e il message_t che voglio
estrarre dal socket.
In questo caso faccio tre read, con la prima read estraggo l'header del messaggio che Contiene
l'operazione da svolgere e il mittente.
Con la seconda read invece estraggo la struttura che contiene il destinatario del messaggio
e la lunghezza del buffer.
L'ultima read mi serve per estrarre il buffer.
Se tutto va bene restituisco 0 altrimenti -1.
*/
int readMsg(long fd, message_t *msg){
  int x=0;
  int r=0;

  if(fd<0 || msg==NULL){
    return -1;
  }


  // Read: estrazione dell'header del messaggio
  // Prima estraggo l'operazione che deve essere effettuata dal server
  message_hdr_t newhdr;
  r=read(fd,&(newhdr.op),sizeof(op_t));
  if(r==0){
    return 1;
  }
  checkMU(x, r, "Prima read di readMsg");
  //printf("prima read %d dd\n",newhdr.op);


  // Poi estraggo il nome del sender del messaggio
  r=read(fd,&newhdr.sender,MAX_NAME_LENGTH+1);
  checkMU(x, r, "Seconda read di readMsg");
  //printf("Letto: %s %lu\n",newhdr.sender, strlen(newhdr.sender));
  setHeader(&(msg->hdr), newhdr.op, newhdr.sender);
  if(r==0){
    return 1;
  }
  //printf("prima read %s dd\n",newhdr.sender);
  // Seconda read: estrazione del message_data_hdr_t
  message_data_hdr_t newhdr_data;
  r=read(fd,&newhdr_data.receiver,MAX_NAME_LENGTH+1);
  checkMU(x, r, "Terza read di readMsg");
  if(r==0){
    return 1;
  }
  r=read(fd,&newhdr_data.len,sizeof(unsigned int));
  checkMU(x, r, "Quarta read di readMsg");
  //printf("terza read\n");
  if(r==0){
    return 1;
  }

  msg->data.hdr=newhdr_data;


  int leggere=newhdr_data.len;
  msg->data.buf=malloc(leggere);
  char * scrivere=msg->data.buf;

  if(leggere==0){
    return 0;
  }


  // Terza read: estrazione del buffer
  while(leggere>0){
    r=read(fd,scrivere,leggere);
    checkMU(x, r, "Quinta read di readMsg");
    //printf("quarta read\n");
    if(r==0){
      //printf("chiuso\n");
      return 1;
    }
    leggere-=r;
    scrivere+=r;
  }

  return 0;

}

/* da completare da parte dello studente con altri metodi di interfaccia */


// ------- client side ------
/**

Funzione che prende come parametro il descrittore della connessione e un messaggio da inviare
Si scrive per 3 volte nel buffer.
Con la prima write invio l'header del messaggio, con la seconda invio il destinatario
e la lunghezza del buffer, con l'ultima write poi invio il buffer vero e proprio.
Se tutto va bene restituisco 0 altrimenti -1.

*/
int sendRequest(long fd, message_t *msg){

  // Per prima cosa controllo che il messaggio da inviare
  // sia diverso da NULL, se è uguale non faccio nulla.
  if(msg==NULL || fd<0){

    return -1;
  }

  int r=0;
  int w=0;
  int scrivere=((msg->data).hdr).len;
  char * str=(msg->data).buf;


  // Scrivo nel socket l'operazione da svolgere
  w=write(fd,&(msg->hdr).op,sizeof(op_t));
  checkMU(r,w,"sendRequest: errore nella prima scrittura");

  // Scrivo nel socket il mittente del messaggio
  w=write(fd,(msg->hdr).sender,MAX_NAME_LENGTH+1);
  checkMU(r,w,"sendRequest: errore nella seconda scrittura");


  // Scrivo nel socket il destinatario del messaggio e
  // La lunghezza del buffer
  w=write(fd,((msg->data).hdr).receiver,MAX_NAME_LENGTH+1);
  checkMU(r,w,"sendRequest: errore nella terza scrittura");

  // Scrivo nel socket la lunghezza del buffer
  w=write(fd,&((msg->data).hdr.len),sizeof(unsigned int));
  checkMU(r,w,"sendRequest: errore nella quarta scrittura");


  // Caso in cui i byte da scrivere sono 0 oppure
  // il buffer è NUll. Allora salto la scrittura del
  // buffer e restituisco direttamente 0.
  if(scrivere==0 || msg->data.buf==NULL)
  return 0;


  while(scrivere>0 && str!=NULL){

    w=write(fd,str,scrivere);
    checkMU(r,w,"sendRequest: errore nella terza scrittura");

    scrivere-=w;
    str+=w;

  }


  return 0;

}

/**

Funzione che prende come parametro il descrittore della connessione e un message_data_t da inviare
Si scrive per 2 volte nel buffer.
Con la prima write invio  il destinatario
e la lunghezza del buffer, con la seconda write poi invio il buffer vero e proprio.
Se tutto va bene restituisco 0 altrimenti -1.

*/
int sendData(long fd, message_data_t *msg){

  // Per prima cosa controllo che il messaggio da inviare
  // sia diverso da NULL, se è uguale non faccio nulla.
  if(msg==NULL || fd<0 ){
    perror("sendData Messaggio NULL");
    return -1;
  }

  int r=0;
  int w=0;
  int scrivere=(msg->hdr).len;
  char * str=msg->buf;


  // Scrivo nel socket il destinatario del messaggio e
  // La lunghezza del buffer
  w=write(fd,&((msg->hdr).receiver),MAX_NAME_LENGTH+1);
  checkMU(r,w,"SendData: errore nella seconda scrittura");

  // Scrivo nel socket la lunghezza del buffer
  w=write(fd,&((msg->hdr).len),sizeof(unsigned int));
  checkMU(r,w,"SendData: errore nella seconda scrittura");


  // Caso in cui i byte da scrivere sono 0 oppure
  // il buffer è NUll. Allora salto la scrittura del
  // buffer e restituisco direttamente 0.
  if(scrivere==0 || msg->buf==NULL)
    return 0;
  else{
    while(scrivere>0 && str!=NULL){
      w=write(fd,str,scrivere);

      checkMU(r,w,"SendData: errore nella terza scrittura");
      scrivere-=w;
      str+=w;

    }
  }



  return 0;
}


/**

Funzione che prende come parametro il descrittore della connessione e un
header da inviare al server
Con la prima write invio l'operazione da svolgere, con la seconda write invio
il mittente.
Se tutto va bene restituisco 0, altrimenti -1

*/
int sendHeader(long fd, message_hdr_t *msg){

  // Per prima cosa controllo che il messaggio da inviare
  // sia diverso da NULL, se è uguale non faccio nulla.
  if(msg==NULL){
    perror("sendHeader Messaggio NULL");
    return -1;
  }

  int r=0;
  int w=0;

  // Invio della operazione da svolgere
  w=write(fd,&(msg->op),sizeof(op_t));
  checkMU(r,w,"SendHeader: errore nella prima scrittura");

  // Invio del mittente del messaggio
  w=write(fd,&(msg->sender),MAX_NAME_LENGTH+1);
  checkMU(r,w,"sendHeader: errore nella seconda scrittura");

  return 0;
}


/* da completare da parte dello studente con eventuali altri metodi di interfaccia */
