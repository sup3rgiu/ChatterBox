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
 * @file chatty.c
 * @brief File principale del server chatterbox
 */


#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <sys/select.h>
#include <connections.h>
#include <message.h>
#include <ops.h>
#include "icl_hash.h"
#include "utils.h"
#include <pthread.h>
#include <MyPthread.h>
#include "coda.h"
#include "thread_pool.h"
#include <signal.h>
#include "codamessaggi.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parsing.h"
#include <time.h>
#include <sys/mman.h>
#include "stats.h"

/*
*********************************************
*/

#define UNIX_PATH_MAX  64
#define SERVERSOCKETNAME "/tmp/chatty_socket"
#define MTXMAX 128
#define MTXWRITE 16

// Macro utilizzata per controllare il valore restituito dalle funzioni
#define SYSCALL(r,c,e) \
if((r=c)==-1) { perror(e);exit(errno); }

// Macro utilizzata per controllare che il valore restituito dalla funzione sia 0
#define CHECKDIVZERO(r,c,e) \
if((r=c)!=0) { perror(e);exit(errno); }

typedef struct user_online_{
  pthread_mutex_t mtx;
  int fd;
} user_online;

/*
*********************************************
Variabili globali, mutex e variabili di condizione
*********************************************
*/

// Struttura in cui memorizzo i dati di configurazione del server estratti dal file di configurazione
config * configurazione;
// Memorizzazione utenti registrati
coda * coda_new;
// Tabella hash in cui memorizzo gli utenti registrati
icl_hash_t *utenti_registrati;
// Struttura per la memorizzazione delle statistiche d'uso del server
struct statistics * statistiche;
// Variabile che uso per controllare l'esecuzione del server, quando diventa 1 ovvero quando arriva un segnale
// viene settata a 1 in modo che si blocchi l'esecuzione del server
int avvio=0;
// Struttura in cui memorizzo gli utenti attualmente online con il rispettivo File Descriptor

pthread_mutex_t mtx_registrati[MTXMAX];
pthread_mutex_t mtx_write[MTXWRITE];
pthread_mutex_t mtxmain=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxStats=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond=PTHREAD_COND_INITIALIZER;


/*
*********************************************
Funzione hash e funzione per il confronto utilizzare nella creazione della
tabella hash utilizzata per memorizzare gli utenti registrati.
*********************************************
*/

static unsigned int
hash_pjw(void* key)
{
  char *datum = (char *)key;
  unsigned int hash_value, i;

  if(!datum) return 0;

  for (hash_value = 0; *datum; ++datum) {
    hash_value = (hash_value << ONE_EIGHTH) + *datum;
    if ((i = hash_value & HIGH_BITS) != 0)
    hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
  }
  return (hash_value);
}

static int string_compare(void* a, void* b)
{
  return (strcmp( (char*)a, (char*)b ) == 0);
}


/*
*********************************************
Funzione che serve ad inizializzare tutte le strutture e tutte le mutex, viene chiamata dal main
*********************************************
*/

void inizializza(){
  int i=0;
  // Mutex della tabella hash
  for(i=0;i<MTXMAX;i++){
    Pthread_mutex_init(&mtx_registrati[i], NULL);
  }

  // Mutex per la scrittura sui FD
  for(i=0;i<MTXWRITE;i++){
    Pthread_mutex_init(&mtx_write[i], NULL);
  }

  coda_new=creacoda(configurazione->MaxConnessioni);

  utenti_registrati = icl_hash_create(512, hash_pjw, string_compare);
  if(utenti_registrati==NULL){
    perror("Errore nella creazione della tabella Hash");
    exit(EXIT_FAILURE);
  }
  statistiche=Malloc(sizeof(struct statistics));
  statistiche->nusers=0;
  statistiche->nonline=0;
  statistiche->ndelivered=0;
  statistiche->nnotdelivered=0;
  statistiche->nfiledelivered=0;
  statistiche->nfilenotdelivered=0;
  statistiche->nerrors=0;

}

/*
*********************************************
FUNZIONI DI "SUPPORTO"
*********************************************
*/

/*
*********************************************
@ Parametri: la tabella hash degli utenti
@ Return: la nuova struttura contenente i nuovi thread se tutto è andato a buon fine.

@ Funzione di "pulizia" che prende come parametro la tabella hash degli utenti e va a
deallocare tutto lo spazio che è stato allocato per contenere i messaggi inviati
da un utente ad altri utenti che si trovavano offline nel momento dell'invio.
*********************************************
*/
void svuota_coda(icl_hash_t * ht){
  icl_entry_t *bucket, *curr, *next;
  int i;

  // Per ogni bucket della tabella hash controllo se ci sono utenti
  // Se ci sono utenti controllo se nella loro coda dei messaggi non letti sono presenti
  // dei messaggi. Se sono presenti li elimino
  for (i=0; i<ht->nbuckets; i++) {

    bucket = ht->buckets[i];
    Pthread_mutex_lock(&mtx_registrati[i/4]);
    for (curr=bucket; curr!=NULL; ) {
      next=curr->next;
      if(curr->coda->dim!=0){

        int dim=curr->coda->dim;

        while(dim>0){
          message_t msg=eliminaElementoMessaggi(curr->coda);
          free(msg.data.buf);
          dim--;
        }

      }
      if(curr->coda->message)
      free(curr->coda->message);
      if(curr->coda)
      free(curr->coda);
      curr=next;
    }
    Pthread_mutex_unlock(&mtx_registrati[i/4]);
  }

}


/*
*********************************************
@ Parametri: la tabella hash degli utenti, il nome dell'utente che vuole recuperare i precedenti messaggi
@ Return: la coda con i messaggi inviati all'utente mentre era offline, NULL se non ha trovato messaggi
inviati mentre era offline

@ Funzione che controlla se l'utente ha ricevuto messaggi mentre era offline
*********************************************
*/
codamessaggi * getPrevMessaggi(icl_hash_t * ht, void *key){
  icl_entry_t* curr;
  unsigned int hash_val;
  int x=0;
  codamessaggi * new=NULL;
  hash_val = (* ht->hash_function)(key) % ht->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
    if (strcmp(curr->data, key)==0){
      new=curr->coda;

      x=1;
    }
  }

  Pthread_mutex_unlock(&mtx_registrati[accesso]);

  if(x==1){
    return new;
  }

  return NULL;
}

/*
*********************************************
@ Parametri: la tabella hash degli utenti
@ Return: la stringa formattata nel modo adeguato che contiene i nomi degli utenti online attualmente

@ Funzione che viene chiamata quando un utente ha la necessità di avere la lista degli utenti attualmente
connessi.
*********************************************
*/
char  * icl_hash_print_online(icl_hash_t *ht){
  icl_entry_t *bucket, *curr;
  int i;
  char * con1=NULL;
  int dim=(MAX_NAME_LENGTH+1);

  char * con=(char*)calloc((((configurazione->MaxConnessioni)*(dim))),(sizeof(char)));
  if(con==NULL){
    perror("Errore allocazione memoria");
    exit(EXIT_FAILURE);
  }
  char *str=con;
  int x=0;


  for(i=0; i<ht->nbuckets; i++) {
    bucket = ht->buckets[i];

    Pthread_mutex_lock(&mtx_registrati[i/4]);

    for(curr=bucket; curr!=NULL; ) {
      if(curr->data!=NULL && strlen(curr->data)!=0 && curr->online==1){
        if(x!=0){
          con+=dim;
        }
        con1=(char*)calloc(dim,sizeof(char));
        if(con1==NULL){
          perror("Errore allocazione memoria");
          exit(EXIT_FAILURE);
        }
        strncpy(con1,curr->data,strlen(curr->data));
        strncat(con,con1,dim);
        free(con1);
        x++;
      }
      curr=curr->next;
    }
    Pthread_mutex_unlock(&mtx_registrati[i/4]);
  }

  str[x*dim]='\0';
  con=str;

  return con;
}

/*
*********************************************
Operazioni svolte dal server
*********************************************
*/


/*
*********************************************
@ Parametri: il file descriptor dell'utente che vuole disconnettersi
@ Return: 0 se disconnetto l'utente e -1 se non lo trovo e c'è un errore

@ Funzione utilizzata per disconnettere un utente attualmente connesso.
*********************************************
*/
int disconnectUser(int fd_servernew){
  icl_entry_t *bucket, *curr;
  int i;
  int fine=0;

  for (i=0; i<utenti_registrati->nbuckets; i++) {
    bucket = utenti_registrati->buckets[i];

    Pthread_mutex_lock(&mtx_registrati[i/4]);

    for (curr=bucket; curr!=NULL;curr=curr->next ) {
      if(fd_servernew==curr->fd){

        if(goOffline(utenti_registrati,curr->data)==0){
          Pthread_mutex_lock(&mtxStats);
          statistiche->nonline--;
          Pthread_mutex_unlock(&mtxStats);

          fine=1;
        }

      }
    }

    Pthread_mutex_unlock(&mtx_registrati[i/4]);
  }
  if(fine==1){
    return 0;
  }

  return -1;
}




/*
*********************************************
@ Parametri: il nome del nuovo utente da registrare, il file descriptor dell'utente

@ Funzione che serve per registrare un utente, durante la registrazione l'utente viene
aggiunto alla tabella hash e viene segnato come online.
*********************************************
*/
void registerUser(char * s,int fd){
  #ifdef DEBUG
  printf("registerUser\n");
  #endif


  int registrato=0;
  unsigned int hash_val;
  int id=0;
  hash_val = (* utenti_registrati->hash_function)(s) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;


  Pthread_mutex_lock(&mtx_registrati[accesso]);
  icl_entry_t * inserito=NULL;
  // Controlliamo che l'utente che vuole registrarsi non sia già registrato
  if((inserito=icl_hash_insert(utenti_registrati,fd,s,configurazione->MaxHistMsgs))==NULL)
    registrato=1;
  Pthread_mutex_unlock(&mtx_registrati[accesso]);

  // Caso in cui l'utente è già registrato
  if(registrato==1){
    Pthread_mutex_lock(&mtx_registrati[accesso]);
    id=icl_get_id(utenti_registrati,s);
    Pthread_mutex_unlock(&mtx_registrati[accesso]);

    message_hdr_t reply;
    setHeader(&reply, OP_NICK_ALREADY, "sender");
    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);
    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
  }
  // Se l'utente non è già registrato allora lo registriamo
  else{
    int online=0;
    message_hdr_t reply;


    // Invio lista utenti connessi
    char *conns=NULL;
    conns=icl_hash_print_online(utenti_registrati);
    message_data_t connessi;
    id=0;
    Pthread_mutex_lock(&mtxStats);
    statistiche->nusers++;
    statistiche->nonline++;
    id=statistiche->nusers;
    online=statistiche->nonline;
    setData(&connessi," ",conns,online*(MAX_NAME_LENGTH+1));
    Pthread_mutex_unlock(&mtxStats);

    Pthread_mutex_lock(&mtx_registrati[accesso]);
    icl_hash_update_id(utenti_registrati,s,id);
    Pthread_mutex_unlock(&mtx_registrati[accesso]);

    setHeader(&reply, OP_OK, "server");
    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);
    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendData(fd, &connessi);
    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
    free(conns);
  }

}


/*
*********************************************
@ Parametri: il nome dell'utente che vuole disconnettersi e il suo file descriptor

@ Funzione che elimina dalla tabella hash un utente attualmente registrato
*********************************************
*/
void deregisterUser(char * t,int fd){
  #ifdef DEBUG
  printf("deregisterUser\n");
  #endif


  char * user=(char*)Malloc(sizeof(char)*strlen(t)+1);
  strncpy(user,t,strlen(t)+1);

  message_hdr_t reply;

  unsigned int hash_val;
  hash_val = (* utenti_registrati->hash_function)(user) % utenti_registrati->nbuckets;
  int fd_client=0;
  int accesso=hash_val/4;
  int id=0;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  //Caso in cui l'utente è registrato
  if((fd_client=isRegistrato(utenti_registrati,user))!=-1){
      id=icl_get_id(utenti_registrati,user);
    // Se non riesco a deregistrarlo notifico il client con una operazione di fallimento
    if(icl_hash_delete(utenti_registrati,user,NULL,NULL)==-1){
      setHeader(&reply, OP_FAIL, "server");
      Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
      sendHeader(fd,&reply);

      Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

      Pthread_mutex_lock(&mtxStats);
      statistiche->nerrors++;
      Pthread_mutex_unlock(&mtxStats);
    }
    // Se tutto va bene e lo deregistro notifico il client con una op_ok
    else{
      Pthread_mutex_lock(&mtxStats);
      statistiche->nusers--;
      statistiche->nonline--;
      Pthread_mutex_unlock(&mtxStats);
      setHeader(&reply, OP_OK, "server");
      Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
      sendHeader(fd,&reply);
      Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

    }
  }
  Pthread_mutex_unlock(&mtx_registrati[accesso]);

  // Caso in cui chiedo di deregistrare un utente che non è già registrato
  if(fd_client==-1){


    setHeader(&reply, OP_NICK_UNKNOWN, "server");
    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);

    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

    Pthread_mutex_lock(&mtxStats);
    statistiche->nerrors++;
    Pthread_mutex_unlock(&mtxStats);
  }

  free(user);

}


/*
*********************************************
@ Parametri: il nome dell'utente che si deve connettere, il suo file descriptor, e il messaggio arrivato daal client con la
richiesta di connessione

@ Segna come connesso un utente che deve già essere registrato
*********************************************
*/
void connectUser(char * t,int fd,message_t msg){
  #ifdef DEBUG
  printf("connectUser\n");
  #endif

  int nonregistrato=0;
  message_hdr_t reply;
  unsigned int hash_val;
  int id=0;
  hash_val = (* utenti_registrati->hash_function)(t) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  // Quando l'utente è già online aggiorno il suo FD
  if(isOnline(utenti_registrati,t)!=-1){
    icl_hash_updatefd(utenti_registrati,fd,t);
  }

  //Cerco di mandare online l'utente, se questo non è registrato allora devo notificarlo.
  if(goOnline(utenti_registrati,t,fd)== -1){
    id=icl_get_id(utenti_registrati,t);
    nonregistrato=1;
  }
  Pthread_mutex_unlock(&mtx_registrati[accesso]);

  // Notifico l'utente che cerca di andare online senza essere registrato
  if(nonregistrato==1){
    setHeader(&reply, OP_NICK_UNKNOWN, msg.hdr.sender);
    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);
    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

    Pthread_mutex_lock(&mtxStats);
    statistiche->nerrors++;
    Pthread_mutex_unlock(&mtxStats);
  }
  // Se invece l'utente che devo mandare online è già registrato allora
  // lo notifico dicendo che l'operazione è andata a buon fine e gli mando
  // l'elenco degli utenti connessi
  else{
    int online=0;

    // Invio lista utenti connessi
    char *conns;
    conns=icl_hash_print_online(utenti_registrati);
    message_data_t connessi;

    Pthread_mutex_lock(&mtxStats);
    statistiche->nonline++;
    online=statistiche->nonline;
    setData(&connessi,"",conns,online*(MAX_NAME_LENGTH+1)+2);

    Pthread_mutex_unlock(&mtxStats);

    setHeader(&reply, OP_OK, msg.hdr.sender);

    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);

    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));




    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendData(fd, &connessi);
    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));


    free(conns);

  }


}

/*
*********************************************
@ Parametri: file descriptor dell'utente che vuole visionare gli utenti online

@ Funzione che serve per inviare al client che ne fa richiesta la lista degli utenti
che attualmente sono online
*********************************************
*/
void get_connessi(int fd, char * user){
  #ifdef DEBUG
  printf("get_connessi\n");
  #endif

  int online=0;
  unsigned int hash_val;

  int id=0;
  hash_val = (* utenti_registrati->hash_function)(user) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);

  id=icl_get_id(utenti_registrati,user);

  Pthread_mutex_unlock(&mtx_registrati[accesso]);


  message_hdr_t reply;
  setHeader(&reply, OP_OK, "server");
  Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
  sendHeader(fd,&reply);
  Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));


    // Invio lista utenti connessi
    char *conns;
    conns=icl_hash_print_online(utenti_registrati);

    message_data_t connessi;



  Pthread_mutex_lock(&mtxStats);
  online=statistiche->nonline;
  setData(&connessi,"",conns,online*(MAX_NAME_LENGTH+1)+2);
  Pthread_mutex_unlock(&mtxStats);


  Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
  sendData(fd, &connessi);
  Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));


  free(conns);
}


/*
*********************************************
@ Parametri: messaggio da inviare, file descriptor del mittente

@ Funzione che server per inviare un messaggio ad un certo utente. Questo utente a cui
voglio inviare il messaggio potrebbe anche essere offline. In quel caso il messaggio che
dovrebbe ricevere lo inserisco nella sua coda
*********************************************
*/
void postMSG(message_t msg, int fd){
  #ifdef DEBUG
  printf("postMSG\n");
  #endif
  int inviato=0;
  int fdclient=0;
  message_hdr_t reply;
  int id=0, idrec=0;
  unsigned int hash_val;

  hash_val = (* utenti_registrati->hash_function)(msg.hdr.sender) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  id=icl_get_id(utenti_registrati,msg.hdr.sender);
  Pthread_mutex_unlock(&mtx_registrati[accesso]);


  char *s;
  // Controllo se il messaggio che voglio inviare supera la lunghezza massima stabilita nel
  // file di configurazione. Se la supero invio l'errore corrispondente
  if(msg.data.hdr.len > configurazione->MaxMsgSize){
    Pthread_mutex_lock(&mtxStats);
    statistiche->nerrors++;
    Pthread_mutex_unlock(&mtxStats);
    setHeader(&reply, OP_MSG_TOOLONG, "server");
    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);
    inviato=1;
    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
  }
  // Se invece il messaggio può essere inviato
  else{
    Pthread_mutex_lock(&mtxStats);
    statistiche->nnotdelivered++;
    Pthread_mutex_unlock(&mtxStats);



    hash_val = (* utenti_registrati->hash_function)(msg.data.hdr.receiver) % utenti_registrati->nbuckets;
    int accesso=hash_val/4;

    // Cerco il FD dell'utente a cui devo inviare il messaggio.
    // Controllo prima tra gli utenti online. Se l'utente è online il messaggio glielo invio
    // direttamente e lui lo legge subito
    Pthread_mutex_lock(&mtx_registrati[accesso]);
    fdclient=isOnline(utenti_registrati,msg.data.hdr.receiver);
    Pthread_mutex_unlock(&mtx_registrati[accesso]);

    if(fdclient!=-1){
      idrec=icl_get_id(utenti_registrati,msg.data.hdr.receiver);

      setHeader(&reply, OP_OK, "server");
      Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
      sendHeader(fd,&reply);
      Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));


      Pthread_mutex_lock(&mtxStats);
      statistiche->ndelivered++;
      Pthread_mutex_unlock(&mtxStats);

      msg.hdr.op=TXT_MESSAGE;

      // Se l'utente è online gli mando il messaggio
      if(fdclient>0){
        Pthread_mutex_lock(&(mtx_write[idrec%MTXMAX]));
        sendRequest(fdclient, &msg);
        Pthread_mutex_unlock(&(mtx_write[idrec%MTXMAX]));
        inviato=1;
      }
    }

    // Caso in cui l'utente non è online
    if(fdclient==-1){

      // Cerco tra gli utenti registrati se trovo l'utente a cui devo inviare e prendo il suo FD.
      hash_val = (* utenti_registrati->hash_function)(msg.data.hdr.receiver) % utenti_registrati->nbuckets;
      int accesso=hash_val/4;

      Pthread_mutex_lock(&mtx_registrati[accesso]);
      int fd2=isRegistrato(utenti_registrati,msg.data.hdr.receiver);
      Pthread_mutex_unlock(&mtx_registrati[accesso]);


      // Se l'utente non è registrato
      if(fd2==-1){
        setHeader(&reply, OP_NICK_UNKNOWN, "server");
        Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
        sendHeader(fd,&reply);

        Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

        Pthread_mutex_lock(&mtxStats);
        statistiche->nerrors++;
        Pthread_mutex_unlock(&mtxStats);
      }
      // Se invece è registrato ma non è online allora salvo il messaggio che devo
      // inviare nella sua coda in modo che successivamente possa richiedere la visualizzazione
      // della cronologia dei messaggi arrivati mentre non era connesso
      else{
        hash_val = (* utenti_registrati->hash_function)(msg.data.hdr.receiver) % utenti_registrati->nbuckets;
        int accesso=hash_val/4;

        s=Malloc(sizeof(char)*msg.data.hdr.len);
        strncpy(s,msg.data.buf,msg.data.hdr.len);
        message_t bck;
        setHeader(&bck.hdr, msg.hdr.op, msg.hdr.sender);
        setData(&bck.data,msg.data.hdr.receiver,s,msg.data.hdr.len);
        free(msg.data.buf);
        Pthread_mutex_lock(&mtx_registrati[accesso]);
        int ret=icl_add_message(utenti_registrati,bck.data.hdr.receiver, bck);
        Pthread_mutex_unlock(&mtx_registrati[accesso]);

        // Caso in cui non si riesce ad aggiungere il messaggio nella coda dell'utente
        if(ret==-1){
          setHeader(&reply, OP_NICK_UNKNOWN, "server");

          Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
          sendHeader(fd,&reply);
          Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

          Pthread_mutex_lock(&mtxStats);
          statistiche->nerrors++;
          Pthread_mutex_unlock(&mtxStats);
          free(msg.data.buf);
          msg.data.hdr.len=-1;
        }
        // Caso in cui va tutto bene
        else{
          setHeader(&reply, OP_OK, "server");
          Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
          sendHeader(fd,&reply);
          Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
        }
      }

    }

  }
  // Se ho inviato il messaggio dealloco la memoria occupata dal messaggio
  if(inviato==1){
    free(msg.data.buf);
    msg.data.hdr.len=-1;
  }

}

/*
*********************************************
@ Parametri: messaggio inviato dal client e fd

@ Funzione che va a prendere la coda messaggi del client che fa richiesta (chiamando la funzione getPrevMessaggi)
poi prende uno per uno i vari messaggi presenti all'interno della coda e li invia al client che ha fatto richiesta di
recupero
*********************************************
*/
void getprevmsg(message_t msgt, int fd){
  #ifdef DEBUG
  printf("getprevmsg\n");
  #endif
  int id=0;
  unsigned int hash_val;

  hash_val = (* utenti_registrati->hash_function)(msgt.hdr.sender) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  id=icl_get_id(utenti_registrati,msgt.hdr.sender);
  Pthread_mutex_unlock(&mtx_registrati[accesso]);


  // Prendo la coda messaggi dell'utente che fa la richiesta
  codamessaggi * coda=getPrevMessaggi(utenti_registrati,msgt.hdr.sender);

  message_hdr_t reply;
  // Notifico il client
  setHeader(&reply, OP_OK, "server");
  Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
  sendHeader(fd,&reply);
  Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

  size_t dim=0;
  dim=(size_t)coda->dim;
  setData(&(msgt.data), "", (const char *) &dim, (unsigned int)sizeof(size_t*));

  // Invio al client la dimensione della coda dei messaggi
  Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
  sendData(fd, &(msgt.data));
  Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

  // Se la coda dei messaggi ha qualcosa al suo interno
  if(dim>0){

    message_t msg;
    int i=0;
    for(i=0;i< (int) dim;i++){
      //Prendo ogni elemento della coda
      msg=eliminaElementoMessaggi(coda);
      Pthread_mutex_lock(&mtxStats);
      statistiche->ndelivered++;
      Pthread_mutex_unlock(&mtxStats);

      // Faccio un controllo sul tipo di elemento che devo inviare al client,
      // Posso estrarre dalla coda o un normale messaggio o un file.
      // Faccio la distinzione perchè se è un file lo segno come file in modo che il client
      // lo riconosca come tale
      if(msg.hdr.op==21 || msg.hdr.op==2){

        if(fd>0){
          Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
          sendRequest(fd, &msg);
          Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
          free(msg.data.buf);
        }
        else{
          free(msg.data.buf);
        }

      }
      else if(msg.hdr.op==22 || msg.hdr.op==4){

        msg.hdr.op=FILE_MESSAGE;

        if(fd>0){
          Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
          sendRequest(fd, &msg);
          Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
        }
        else{
          free(msg.data.buf);
        }

      }


    }


  }

}


/*
*********************************************
@ Parametri: messaggio da inviare, file descriptor dell'utente

@ Funzione che invia un certo messaggio a tutti gli utenti che sono **registrati**, se un certo utente non è connesso
il messaggio finirà nella sua coda messaggi.
*********************************************
*/
void postMSGatAll(message_t msg,int fd){
  #ifdef DEBUG
  printf("PostMsgtoall\n");
  #endif

  msg.hdr.op=TXT_MESSAGE;

  char *s;

  int fdclient=0;
  message_hdr_t reply;

  int i;
  int check=0;
  int id=0, idrec=0;
  unsigned int hash_val;

  hash_val = (* utenti_registrati->hash_function)(msg.hdr.sender) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  id=icl_get_id(utenti_registrati,msg.hdr.sender);
  Pthread_mutex_unlock(&mtx_registrati[accesso]);

  // Controllo che la lunghezza del messaggio non sia maggiore del limite.
  if(msg.data.hdr.len > configurazione->MaxMsgSize){
    Pthread_mutex_lock(&mtxStats);
    statistiche->nerrors++;
    Pthread_mutex_unlock(&mtxStats);
    setHeader(&reply, OP_MSG_TOOLONG, "server");
    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);
    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
    free(msg.data.buf);
    msg.data.hdr.len=-1;
  }
  // Se non è maggiore
  else{
    Pthread_mutex_lock(&mtxStats);
    statistiche->nnotdelivered++;
    Pthread_mutex_unlock(&mtxStats);

    icl_entry_t *bucket, *curr;

    for (i=0; i<utenti_registrati->nbuckets; i++) {
      bucket = utenti_registrati->buckets[i];

      for (curr=bucket; curr!=NULL; curr=curr->next) {

        // Devo controllare che la persona a cui invio non è lo stesso che
        // vuole inviare il messaggio perchè altrimenti mi invierei il messaggio da solo e non
        // ha molto senso
        if(strcmp(curr->data,msg.hdr.sender)!=0){

          hash_val = (* utenti_registrati->hash_function)(curr->data) % utenti_registrati->nbuckets;
          int accesso=hash_val/4;

          Pthread_mutex_lock(&mtx_registrati[accesso]);
          fdclient=isOnline(utenti_registrati,curr->data);
          Pthread_mutex_unlock(&mtx_registrati[accesso]);

          // Controllo se l'utente a cui voglio inviare è online.
          if(fdclient!=-1){
            // Se è online mando l'header con OK al client che ha inviato il messaggio a tutti
            // con check mi ricordo di aver già inviato la notifica al client che ha chiesto di
            // inviare il messaggio a tutti, altrimenti gli invierei una notifica ogni volta che invio
            // il messaggio ad un altro client
            if(check==0){
              setHeader(&reply, OP_OK, "server");
              Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
              sendHeader(fd,&reply);
              Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
              check=1;
            }
            Pthread_mutex_lock(&mtxStats);
            statistiche->ndelivered++;
            Pthread_mutex_unlock(&mtxStats);
            idrec=icl_get_id(utenti_registrati,curr->data);

            // Poi invio il messaggio al client attivo.
            Pthread_mutex_lock(&(mtx_write[idrec%MTXMAX]));
            sendRequest(fdclient, &msg);
            Pthread_mutex_unlock(&(mtx_write[idrec%MTXMAX]));

          }


          // Se non è online devo mandare al client non collegato il messaggio
          // salvandolo nella coda. Non chiedo di nuovo se è registrato perchè conosco già il FD.
          if(fdclient==-1){

            hash_val = (* utenti_registrati->hash_function)(curr->data) % utenti_registrati->nbuckets;
            int accesso=hash_val/4;

            Pthread_mutex_lock(&mtx_registrati[accesso]);

            s=Malloc(sizeof(char)*msg.data.hdr.len);
            strncpy(s,msg.data.buf,msg.data.hdr.len);
            message_t bck;
            setHeader(&bck.hdr, msg.hdr.op, msg.hdr.sender);
            setData(&bck.data,msg.data.hdr.receiver,s,msg.data.hdr.len);

            int ret=icl_add_message(utenti_registrati,curr->data, bck);
            Pthread_mutex_unlock(&mtx_registrati[accesso]);

            if(msg.data.hdr.len==0){
              free(s);
            }

            // Se non riesco a inviare il messaggio all'utente
            if(ret==-1){
              if(check==0){
                setHeader(&reply, OP_FAIL, "server");
                Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
                sendHeader(fd,&reply);

                Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
                free(s);
                msg.data.hdr.len=-1;

                check=1;
              }
            }

            if(check==0){
              setHeader(&reply, OP_OK, "server");
              Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
              sendHeader(fd,&reply);

              Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

              check=1;
            }

          }

        }

      }
    }
    free(msg.data.buf);
  }

}


/*
*********************************************
@ Parametri: messaggio con il file da inviare e file descriptor dell'utente

@ Funzione che invia un certo file ad un utente specifico scelto dal client, l'utente
a cui viene inviato il file
*********************************************
*/
void postFile(message_t msg,int fd){
  #ifdef DEBUG
  printf("PostFile\n");
  #endif
  int inviato=0;
  message_data_t file;
  message_hdr_t reply;
  char *s;
  // Lettura del contenuto del file
  readData(fd, &file);
  int new;

  int id=0,idrec=0;
  unsigned int hash_val;

  hash_val = (* utenti_registrati->hash_function)(msg.hdr.sender) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  id=icl_get_id(utenti_registrati,msg.hdr.sender);
  Pthread_mutex_unlock(&mtx_registrati[accesso]);

  if(file.hdr.len!=0){
    // Controllo se la grandezza del file è maggiore del massimo
    if((file.hdr.len)/1024 > configurazione->MaxFileSize){

      setHeader(&reply, OP_MSG_TOOLONG, " ");
      Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
      sendHeader(fd,&reply);

      Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

      Pthread_mutex_lock(&mtxStats);
      statistiche->nerrors++;
      Pthread_mutex_unlock(&mtxStats);
      free(msg.data.buf);
      msg.data.hdr.len=-1;
    }
    else{
      Pthread_mutex_lock(&mtxStats);
      statistiche->nfilenotdelivered++;
      Pthread_mutex_unlock(&mtxStats);
      // Creo il nuovo file che finisce nella cartella del server
      // Devo salvare in /tmp quindi faccio in modo di avere la stringa con /tmp/nome_file_da_inviare
      char * dir;

      dir=Malloc(sizeof(char)*60);
      char * buf1;
      strncpy(dir,configurazione->DirName,strlen(configurazione->DirName)+1);

      // Ci sono due casi, il client mi manda il file da inviare nella forma "./file" o "file"
      // quindi la stringa va modificata in modo da poter accedere al file senza problemi
      if((buf1=strstr(msg.data.buf,"/"))==NULL){
        strncat(dir,"/",1);
        strncat(dir,msg.data.buf,strlen(msg.data.buf)+1);
      }
      else{
        strncat(dir,buf1,strlen(buf1)+1);
      }

      // Apro il nuovo file creato per scriverci, se il file non esiste lo creo.
      if((new=open(dir, O_RDWR|O_CREAT, 0777))==-1){
        perror("Aprendo il file");
        exit(EXIT_FAILURE);
      }
      ftruncate(new, file.hdr.len);
      char *dest;
      // Scrivo nel file che ho creato
      dest=mmap(NULL,file.hdr.len,PROT_READ | PROT_WRITE, MAP_SHARED, new, 0);
      memcpy(dest,file.buf,file.hdr.len);

      int fdclient=0;
      free(file.buf);
      file.hdr.len=-1;

      unsigned int hash_val;

      hash_val = (* utenti_registrati->hash_function)(msg.data.hdr.receiver) % utenti_registrati->nbuckets;
      int accesso=hash_val/4;

      Pthread_mutex_lock(&mtx_registrati[accesso]);
      fdclient=isOnline(utenti_registrati, msg.data.hdr.receiver);
      Pthread_mutex_unlock(&mtx_registrati[accesso]);

      // Se l'utente è online allora il file glielo mando subito
      if(fdclient!=-1){
        setHeader(&reply, OP_OK, "server");
        Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
        sendHeader(fd,&reply);
        Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

        msg.hdr.op=FILE_MESSAGE;
        idrec=icl_get_id(utenti_registrati,msg.data.hdr.receiver);

        if(fdclient>0){
          Pthread_mutex_lock(&(mtx_write[idrec%MTXMAX]));
          sendRequest(fdclient, &msg);
          Pthread_mutex_unlock(&(mtx_write[idrec%MTXMAX]));
          inviato=1;
        }

      }

      // Caso in cui la persona a cui dobbiamo inviare il file non è online
      if(fdclient==-1){
        unsigned int hash_val;

        hash_val = (* utenti_registrati->hash_function)(msg.data.hdr.receiver) % utenti_registrati->nbuckets;
        int accesso=hash_val/4;

        // Controllo che sia registrato
        Pthread_mutex_lock(&mtx_registrati[accesso]);
        int fd2=isRegistrato(utenti_registrati,msg.data.hdr.receiver);
        Pthread_mutex_unlock(&mtx_registrati[accesso]);

        // Se non è registrato
        if(fd2==-1){
          setHeader(&reply, OP_NICK_UNKNOWN, "server");
          Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
          sendHeader(fd,&reply);
          Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

          Pthread_mutex_lock(&mtxStats);
          statistiche->nerrors++;
          Pthread_mutex_unlock(&mtxStats);
          free(msg.data.buf);
        }
        else{

          unsigned int hash_val=0;
          hash_val = (* utenti_registrati->hash_function)(msg.data.hdr.receiver) % utenti_registrati->nbuckets;
          int accesso=hash_val/4;

          s=Malloc(sizeof(char)*msg.data.hdr.len);
          strncpy(s,msg.data.buf,msg.data.hdr.len);
          message_t bck;
          setHeader(&bck.hdr, msg.hdr.op, msg.hdr.sender);
          setData(&bck.data,msg.data.hdr.receiver,s,msg.data.hdr.len);
          free(msg.data.buf);

          Pthread_mutex_lock(&mtx_registrati[accesso]);
          // Aggiungo in coda il messaggio
          int x=icl_add_message(utenti_registrati,bck.data.hdr.receiver, bck);
          Pthread_mutex_unlock(&mtx_registrati[accesso]);


          if(x==-1){
            setHeader(&reply, OP_FAIL, "server");
            Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
            sendHeader(fd,&reply);

            Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

            Pthread_mutex_lock(&mtxStats);
            statistiche->nerrors++;
            Pthread_mutex_unlock(&mtxStats);
            free(msg.data.buf);
            msg.data.hdr.len=-1;
          }
          else{

            setHeader(&reply, OP_OK, "server");
            Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
            sendHeader(fd,&reply);
            Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

          }
        }

      }

      close(new);
      free(dir);
      if(inviato==1){
        free(msg.data.buf);
        msg.data.hdr.len=-1;

      }
    }
  }
  // Caso in cui il file che invio ha una lunghezza uguale a 0.
  else{
    Pthread_mutex_lock(&mtxStats);
    statistiche->nerrors++;
    Pthread_mutex_unlock(&mtxStats);

    setHeader(&reply, OP_FAIL, "server");
    Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
    sendHeader(fd,&reply);

    Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));

  }


}


/*
*********************************************
@ Parametri: messaggio con la richiesta dell'operazione e con il nome del file da scaticare e file descriptor

@ Funzione che va a recuperare il file che è stato inviato da un certo utente e che il server ha salvato in un
percorso stabilito. Quando viene chiesto il recupero del file si prende il file dalla cartella del server e
si copia in un altro percorso.
*********************************************
*/
void getFile(message_t msg, int fd){
  #ifdef DEBUG
  printf("getFiles");
  #endif

  int id=0;
  unsigned int hash_val;

  hash_val = (* utenti_registrati->hash_function)(msg.hdr.sender) % utenti_registrati->nbuckets;
  int accesso=hash_val/4;

  Pthread_mutex_lock(&mtx_registrati[accesso]);
  id=icl_get_id(utenti_registrati,msg.hdr.sender);
  Pthread_mutex_unlock(&mtx_registrati[accesso]);

  // In msg h o salvato chi ha fatto la richiesta e il nome del file da scaricare.
  message_t bck;
  setData(&bck.data,msg.data.hdr.receiver,msg.data.buf,msg.data.hdr.len);

  message_hdr_t reply;
  setHeader(&reply, OP_OK, "");
  Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
  sendHeader(fd,&reply);

  Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));


  int new;
  int sav;
  char * dir=Malloc(sizeof(char)*60);
  strncpy(dir,configurazione->DirName,strlen(configurazione->DirName)+1);

  // Apro il file nuovo in cui andrò a scrivere
  if((new=open(msg.data.buf,O_WRONLY | O_CREAT , 0777))==-1){
    perror("Errore apertura file\n");
    exit(EXIT_FAILURE);
  }

  char * buf1;
  // Anche in questo caso modifico la stringa
  if((buf1=strstr(msg.data.buf,"/"))==NULL){
    strncat(dir,"/",1);
    strncat(dir,msg.data.buf,strlen(msg.data.buf)+1);
  }
  else{
    strncat(dir,buf1,strlen(buf1)+1);
  }

  // Apro il file da cui devo leggere
  if((sav=open(dir,O_RDONLY))==-1){
    perror("Errore apertura 2 file\n");
    exit(EXIT_FAILURE);
  }


  char buf[1024];
  int r=0;
  int len=0;
  int l=0;
  //Copio dal file in /tmp/chatty al file nuovo
  while((r=read(sav,buf,1024))>0){
    if(((l=write(new,buf,r))==-1)){
      perror("Errore scrittura");
      exit(EXIT_FAILURE);
    }
    len+=l;
    l=0;

  }

  free(dir);
  close(sav);
  close(new);
  Pthread_mutex_lock(&mtxStats);
  statistiche->nfiledelivered++;
  Pthread_mutex_unlock(&mtxStats);

  Pthread_mutex_lock(&(mtx_write[id%MTXMAX]));
  sendData(fd,&bck.data);
  Pthread_mutex_unlock(&(mtx_write[id%MTXMAX]));
  free(msg.data.buf);
  msg.data.hdr.len=-1;

}

/*
*********************************************
Fine operazioni del server
*********************************************
*/


/*
*********************************************
Switch con tutte le operazioni che possono essere eseguite
dal server.
*********************************************
*/
void chooseOperation(op_t op, message_t msg, int fd){
  #ifdef DEBUG
  printf("OPERAZIONE %d FD: %d MITTENTE: %s\n",op,fd,msg.hdr.sender);

  #endif

  switch(op){
    case REGISTER_OP:
    registerUser(msg.hdr.sender,fd);
    break;
    case CONNECT_OP:
    connectUser(msg.hdr.sender,fd,msg);
    break;
    case POSTTXT_OP:
    postMSG(msg,fd);
    break;
    case POSTTXTALL_OP:
    postMSGatAll(msg,fd);
    break;
    case POSTFILE_OP:
    postFile(msg,fd);
    break;
    case GETFILE_OP:
    getFile(msg,fd);
    break;
    case GETPREVMSGS_OP:
    getprevmsg(msg,fd);
    break;
    case USRLIST_OP:
    get_connessi(fd,msg.hdr.sender);
    break;
    case UNREGISTER_OP:
    deregisterUser(msg.hdr.sender, fd);
    break;
    case DISCONNECT_OP:
    disconnectUser(fd);
    break;
    default:{
      printf("non presente\n");
    }
  }
}



/*
*********************************************
@ Parametri: struttura param con il fd_set e un intero che identifica il thread, in realtà l'intero l'ho usato solo per
fare delle prove.

@ Funzione che viene eseguita nel momento in cui vengono creati i thread, abbiamo un while in cui controlliamo che la coda condivisa
sia stata riempita con dei FD che possono essere estratti e processati. Se la coda è vuota il thread si ferma su una variabile
di condizione e viene risvegliato o se viene aggiunto qualcosa in coda oppure se arriva un segnale che comporta la terminazione del
server.
*********************************************
*/
void * esegui(void * x){

  thread_param * param;
  param=(thread_param*) x;
  fd_set * set=(fd_set*) param->set;
  //int y=(int) param->fd_num;

  int fd_servernew;
  message_t msg1;

  while(1){

    Pthread_mutex_lock(&mtxmain);
    while(coda_new->dim==0){
      if(avvio==1)
      break;
      Pthread_cond_wait(&cond, &mtxmain);
    }
    // Controllo avvio perchè potrebbe essere arrivato un segnale SIGINT | SIGQUIT | SIGTERM
    if(avvio==1){
      Pthread_mutex_unlock(&mtxmain);

      return (void*) 0;
    }


    fd_servernew=eliminaElemento(coda_new);

    Pthread_mutex_unlock(&mtxmain);

    int r=0;
    // caso in cui la connessione è chiusa, tolgo il fd dal set
    r=readMsg(fd_servernew,&msg1);
    if(r==1 || r==-1){

      disconnectUser(fd_servernew);

      close(fd_servernew);
    }
    // caso in cui c'è qualcosa da leggere
    else if(r==0){
      if(strlen(msg1.hdr.sender)>0){

        chooseOperation(msg1.hdr.op, msg1,fd_servernew);


        FD_SET(fd_servernew,set);

        // In alcuni casi posso subito fare la free
        if(msg1.hdr.op==0  || msg1.hdr.op==1  || msg1.hdr.op==6  || msg1.hdr.op==7  ||msg1.hdr.op==8 || msg1.hdr.op==9){
          free(msg1.data.buf);
          msg1.data.hdr.len=-1;
        }

      }
    }

  }

  return (void*) 0;
}



/*
*********************************************
@ Return: Struttura con le informazioni sull'orario attuale

@ Funzione che serve per ottenere la data e l'ora attuale, viene restituita in una
struttura apposita in modo che i dati possano essere poi usati nel file di log
*********************************************
*/
struct tm timestamp(){
  time_t ltime;

  struct tm result;
  char stime[32];

  ltime = time(NULL);
  localtime_r(&ltime, &result);
  asctime_r(&result, stime);
  return result;

}

/*
*********************************************
E' la funzione eseguita dal thread che gestisce i segnali che possono arrivare.
Viene catturato il segnale SIGUSR1 che produce le statistiche del server
I segnali SIGQUIT, SIGTERM e SIGQUIT invece provocano la chiusura del server.
Per chiudere il server viene settata a 1 la variabile avvio, in questo modo si esce dal while presente
nel main in cui vengono accettate le nuove connessioni, poi si risvegliano i thread "Worker" in modo che anche
loro possano fare il controllo della variabile avvio e accorgendosi che è settata a 1 andranno a terminare.
*********************************************
*/
void * thread_gestore(void * x){

  int sig;
  int r;
  sigset_t sets;
  SYSCALL(r, sigemptyset(&sets), "sigfillset main");
  SYSCALL(r, sigaddset(&sets, SIGUSR1), "sigaddset");
  SYSCALL(r, sigaddset(&sets, SIGQUIT), "sigaddset");
  SYSCALL(r, sigaddset(&sets, SIGTERM), "sigaddset");
  SYSCALL(r, sigaddset(&sets, SIGINT), "sigaddset");


  while(avvio==0){
    if((r=sigwait(&sets, &sig))==0){
      // Caso in cui vanno stampate le statistiche
      if(sig==SIGUSR1){
        int stats=open(configurazione->StatFileName,O_APPEND|O_WRONLY|O_CREAT,0777);
        if(stats!=-1){
          char *str=Malloc(sizeof(char)*1024);
          Pthread_mutex_lock(&mtxStats);
          struct tm result=timestamp();
          snprintf(str, 1024, "%d:%d:%d - %ld %ld %ld %ld %ld %ld %ld\n",result.tm_hour,result.tm_min, result.tm_sec,statistiche->nusers,statistiche->nonline,statistiche->ndelivered, statistiche->nnotdelivered,statistiche->nfiledelivered,statistiche->nfilenotdelivered,statistiche->nerrors);
          Pthread_mutex_unlock(&mtxStats);

          if((write(stats,str,strlen(str))==-1)){
            perror("Errore scrittura");
            exit(EXIT_FAILURE);
          }
          free(str);
          close(stats);
        }

        continue;
      }
      // Caso in cui arrivano dei segnali per far terminare il server
      if (sig==SIGINT || sig==SIGQUIT || sig==SIGTERM){
        // Setto a 1 la variabile avvio che mi controlla il while nel main e poi
        // risveglio tutti i thread fermi sulla condizione cond.
        avvio=1;
        Pthread_cond_broadcast(&cond);
        return (void*) NULL;
      }

    }
  }
  return (void*) NULL;
}




int main(int argc, char * argv[]){
  unlink(SERVERSOCKETNAME);

  if(argc==1){
    printf("Inserire il path del file di configurazione preceduto dall'opzione -f");
    exit(0);
  }

  int r;
  struct sigaction ignorare;
  sigset_t sets;
  const char optstring[] = "f:";

  // Controllo dell'argomento passato come parametro
  int optc;
  char *conf;
  while ((optc = getopt(argc, argv,optstring)) != -1) {
    switch (optc) {
      case 'f':
      conf=strdup(optarg);
      break;
    }
  }

  /*
  *********************************************
  GESTIONE DEI SEGNALI
  *********************************************
  */
  // Ignoriamo il segnale SIGPIPE
  memset(&ignorare, 0, sizeof(ignorare));
  ignorare.sa_handler = SIG_IGN;
  SYSCALL(r, sigaction(SIGPIPE, &ignorare, NULL), "sigpipe");

  // Thread per la gestione dei segnali
  pthread_t gestore_segnale;
  pthread_attr_t attr;

  SYSCALL(r, sigemptyset(&sets), "sigfillset main");
  SYSCALL(r, sigaddset(&sets, SIGUSR1), "sigaddset");
  SYSCALL(r, sigaddset(&sets, SIGQUIT), "sigaddset");
  SYSCALL(r, sigaddset(&sets, SIGTERM), "sigaddset");
  SYSCALL(r, sigaddset(&sets, SIGINT), "sigaddset");
  SYSCALL(r,pthread_sigmask(SIG_SETMASK,&sets,NULL),"pthread_sigmask main");
  CHECKDIVZERO(r,pthread_attr_init(&attr),"Pthread_attr_init")
  CHECKDIVZERO(r,pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED),"pthread_attr_setdetachstate");
  Pthread_create(&gestore_segnale, &attr, thread_gestore,NULL);


  /*
  *********************************************
  Estrazione dei valori che servono per il funzionamento del server
  dal file di configurazione
  *********************************************
  */

  configurazione=Malloc(sizeof(config));
  configurazione->MaxConnessioni=0;
  configurazione->ThreadInPool=0;
  configurazione->MaxMsgSize=0;
  configurazione->MaxFileSize=0;
  configurazione->MaxHistMsgs=0;
  parsing(configurazione,conf);

  // Sposto la directory del processo all'interno della directory che trovo specificata
  // nel file di configurazione
  chdir(configurazione->DirName);


  /*
  *********************************************
  Creazione del socket e generazione del pool di thread
  *********************************************
  */

  // Struttura da usare nella select per dire alla select quanto tempo deve passare tra un controllo del fd_set e il successivo
  struct timeval time;
  time.tv_usec=150;
  time.tv_sec=0;

  //socket per la comunicazione tra server e clients
  int fd_sk;
  //socket per I/O
  int fd_c;
  // numero massimo di descrittori
  int fd_num=0;
  // indice usato per scorrere la maschera di bit
  int fd;
  // file descriptor attivi
  fd_set set;
  // Insieme dei file descriptor attesi in lettura
  fd_set rdset;
  int notused;
  int i=0;

  // Crezione del socket
  SYSCALL(fd_sk,socket(AF_UNIX,SOCK_STREAM,0),"socket");
  struct sockaddr_un ser;
  memset(&ser,'0',sizeof(ser));
  ser.sun_family=AF_UNIX;
  strncpy(ser.sun_path, configurazione->UnixPath, strlen(SERVERSOCKETNAME)+1);
  SYSCALL(notused,bind(fd_sk,(struct sockaddr*) &ser, sizeof(ser)),"bind");
  SYSCALL(notused,listen(fd_sk,configurazione->MaxConnessioni),"listen");


  // Creazione del parametro da passare ai thread worker. Ho un array di param e in ogni posizione dell'array
  // faccio in modo di passare un numero che mi identifica il thread e l'fd_set
  thread_param ** param=Malloc(sizeof(thread_param*)*configurazione->ThreadInPool);

  for(i=0;i<configurazione->ThreadInPool;i++){
    param[i]=Malloc(sizeof(thread_param));
  }

  for(i=0;i<configurazione->ThreadInPool;i++){
    param[i]->set=&set;
    param[i]->fd_num=i;
  }

  // Chiamo la funzione che mi fa l'inizializzazione delle strutture dati e delle mutex
  inizializza();

  // Creazione e inizializzazione del pool di thread
  poolThread pool=poolCreate(configurazione->ThreadInPool);
  poolInit(configurazione->ThreadInPool, pool, &esegui,param);


  // Metto a 0 la maschera e a 1 il bit relativo a fd_sk.
  FD_ZERO(&set);
  FD_ZERO(&rdset);
  FD_SET(fd_sk,&set);

  // In fd_num tengo il descrittore più alto, in questo caso confronto
  // fd_sk con fd_num dove fd_sk è l'unico descrittore che ho fino ad ora
  if (fd_sk > fd_num)
  fd_num = fd_sk;


  while (avvio==0) {

    /* preparo la maschera per la select */
    rdset=set;
    int notused;
    SYSCALL(notused,select(fd_num+1,&rdset,NULL,NULL,&time),"select");

    // Scandisco ogni bit della maschera per cercare se ne trovo uno messo a 1
    for (fd = 0; fd<=fd_num;fd++) {
      if (FD_ISSET(fd,&rdset)) {

        // Se il bit che trovo a 1 è quello relativo a fd_sk allora devo creare un
        // file descriptor per la comunicazione.
        if (fd==fd_sk){

          SYSCALL(fd_c,accept(fd_sk,(struct sockaddr *) NULL,(socklen_t *) NULL),"accept");
          FD_SET(fd_c, &set);

          // Aggiorno fd_num
          if (fd_c > fd_num)
          fd_num = fd_c;
        }
        else{
          FD_CLR(fd,&set);
          Pthread_mutex_lock(&mtxmain);
          // Controllo che la coda in cui finiscono gli FD non sia già piena
          // Se è piena attendo che uno dei thread worker finisca il lavoro e si prenda
          // un FD risvegliando con una signal questa condizione
          if(coda_new->dim>=configurazione->MaxConnessioni){
            message_hdr_t reply;
            setHeader(&reply, OP_FAIL, "server");
            sendHeader(fd,&reply);
          }
          else if(inserimento(coda_new, fd)==-1){
            perror("Errore inserimento in coda");
            exit(-1);
          }
          Pthread_cond_signal(&cond);
          Pthread_mutex_unlock(&mtxmain);

        }

      }
    }
  }

  /*
  *********************************************
  Il server deve terminare quindi dealloco la memoria
  *********************************************
  */

  int status[configurazione->ThreadInPool];
  for(i=0;i<configurazione->ThreadInPool;i++){
    Pthread_join((pool->thread[i]),(void*)&status);

  }
  for(i=0;i<configurazione->ThreadInPool;i++){
    free(param[i]);
  }

  svuota_coda(utenti_registrati);
  poolDelete(pool);
  EliminaCoda(coda_new);
  free(configurazione->UnixPath);
  free(configurazione->DirName);
  free(configurazione->StatFileName);
  free(configurazione);
  free(statistiche);
  free(param);
  close(fd);
  free(conf);
  Pthread_attr_destroy(attr);
  icl_hash_destroy(utenti_registrati ,NULL, NULL);

  return 0;
}
