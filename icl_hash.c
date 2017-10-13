/**
 * @file icl_hash.c
 *
 * Dependency free hash table implementation.
 *
 * This simple hash table implementation should be easy to drop into
 * any other peice of code, it does not depend on anything else :-)
 *
 * @author Jakub Kurzak
 * Alcune funzioni sono state modificate ed altre aggiunte
 */
/* $Id: icl_hash.c 2838 2011-11-22 04:25:02Z mfaverge $ */
/* $UTK_Copyright: $ */
/**
 * @file icl_hash.c
 * @brief Implementazione di una tabella hash
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "icl_hash.h"
#include "config.h"
#include "message.h"
#include <limits.h>
#include "codamessaggi.h"
#include <pthread.h>

#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))
/**
 * A simple string hash.
 *
 * An adaptation of Peter Weinberger's (PJW) generic hashing
 * algorithm based on Allen Holub's version. Accepts a pointer
 * to a datum to be hashed and returns an unsigned integer.
 * From: Keith Seymour's proxy library code
 *
 * @param[in] key -- the string to be hashed
 *
 * @returns the hash index
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


/**
 * Create a new hash table.
 *
 * @param[in] nbuckets -- number of buckets to create
 * @param[in] hash_function -- pointer to the hashing function to be used
 * @param[in] hash_key_compare -- pointer to the hash key comparison function to be used
 *
 * @returns pointer to new hash table.
 */

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) )
{
    icl_hash_t *ht;
    int i;

    ht = (icl_hash_t*) malloc(sizeof(icl_hash_t));
    if(!ht) return NULL;

    ht->nentries = 0;
    ht->buckets = (icl_entry_t**)malloc(nbuckets * sizeof(icl_entry_t*));
    if(!ht->buckets) return NULL;

    ht->nbuckets = nbuckets;
    for(i=0;i<ht->nbuckets;i++)
        ht->buckets[i] = NULL;


    ht->hash_function = hash_function ? hash_function : hash_pjw;
    ht->hash_key_compare = hash_key_compare ? hash_key_compare : string_compare;

    return ht;
}

/**
 * Search for an entry in a hash table.
 *
 * @param ht -- the hash table to be searched
 * @param key -- the key of the item to search for
 *
 * @returns pointer to the data corresponding to the key.
 *   If the key was not found, returns NULL.
 */

void *
icl_hash_find(icl_hash_t *ht, void* key)
{
    icl_entry_t* curr;
    unsigned int hash_val;

    if(!ht || !key) return NULL;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;
    //printf("ACCESSO DA FIND: %d\n",hash_val);

    //printf("stampa icl: %d %s\n",hash_val,(char*)key);
    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( strcmp(curr->data, key)==0)
            return(curr->data);

    return NULL;
}

/**
 * Insert an item into the hash table.
 *
 * @param ht -- the hash table
 * @param key -- the key of the new item
 * @param data -- pointer to the new item's data
 *
 * @returns pointer to the new item.  Returns NULL on error.
 */
icl_entry_t *
icl_hash_insert(icl_hash_t *ht, int fd, void * data, int  dimCoda)
{
    icl_entry_t *curr;
    unsigned int hash_val;

    if(!ht || !data) return NULL;
  //  printf("%d\n",ht->nbuckets);
    hash_val = (*ht->hash_function)(data) % ht->nbuckets;
  //  printf("INSER hash val: %d %s\n",hash_val, data);
    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
      if ( strcmp(curr->data, data)==0)
          return(NULL); /* key already exists */
    }


    /* if key was not found */
    curr = (icl_entry_t*)malloc(sizeof(icl_entry_t));
    if(!curr) return NULL;

    curr->data=strndup(data,strlen(data)+1);
    curr->next = ht->buckets[hash_val]; /* add at start */
    curr->fd= fd;
    curr->online=1;
    curr->coda=creacodaMessaggi(dimCoda);
    ht->buckets[hash_val] = curr;
    ht->nentries++;

    return curr;
}



/**
 * Aggiorna l'id di un certo utente
 *
 * @param ht -- hash table
 * @param key -- il nome dell'utente che vogliamo modificare
 * @param id -- il nuovo ID
 *
 * @ Restituisce 0 se non ci sono errori e -1 in caso di errori
 */
int
icl_hash_update_id(icl_hash_t *ht, void * key, int id)
{

    icl_entry_t* curr;
    unsigned int hash_val;

    if(!ht || !key) return -1;

    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( strcmp(curr->data, key)==0){
          curr->id=id;
          return 0;
        }

  return -1;

}


/**
 * Free one hash table entry located by key (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param key -- the key of the new item
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
int icl_hash_delete(icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*))
{
    icl_entry_t *curr, *prev;
    unsigned int hash_val;

    if(!ht || !key) return -1;
    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    prev = NULL;
    for (curr=ht->buckets[hash_val]; curr != NULL; )  {
        if ( strcmp(curr->data, key)==0) {
            if (prev == NULL) {
                ht->buckets[hash_val] = curr->next;
            } else {
                prev->next = curr->next;
            }

            free(curr->data);
            free(curr);
            ht->nentries--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

/**
 * Free hash table structures (key and data are freed using functions).
 *
 * @param ht -- the hash table to be freed
 * @param free_key -- pointer to function that frees the key
 * @param free_data -- pointer to function that frees the data
 *
 * @returns 0 on success, -1 on failure.
 */
 int
 icl_hash_destroy(icl_hash_t *ht, void (*free_key)(void*), void (*free_data)(void*))
 {
     icl_entry_t *bucket, *curr, *next;
     int i;

     if(!ht) return -1;

     for (i=0; i<ht->nbuckets; i++) {
         bucket = ht->buckets[i];
         for (curr=bucket; curr!=NULL; ) {
             next=curr->next;

             free(curr->data);
             free(curr);
             curr=next;
         }
     }

     if(ht->buckets) free(ht->buckets);
     if(ht) free(ht);

     return 0;
 }




/**
 * Dump the hash table's contents to the given file pointer.
 *
 * @param stream -- the file to which the hash table should be dumped
 * @param ht -- the hash table to be dumped
 *
 * @returns 0 on success, -1 on failure.
 */

int
icl_hash_dump(FILE* stream, icl_hash_t* ht)
{
    icl_entry_t *bucket, *curr;
    int i;

    if(!ht) return -1;

    for(i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for(curr=bucket; curr!=NULL; ) {
            if(curr->data)
                fprintf(stream, "icl_hash_dump:  %s\n", (char *) curr->data);
            curr=curr->next;
        }
    }

    return 0;
}


/**
 * Aggiorna il file descriptor assegnato all'utente
 *
 * @param fd -- tabella hash
 * @param fd -- il nuovo file descriptor
 * @param key -- il nome utente da aggiornare
 *
 * @returns 0 in caso di successo, altrimenti NULL
 */
void * icl_hash_updatefd(icl_hash_t *ht,int fd, void * key){

  icl_entry_t* curr;
  unsigned int hash_val;

  if(!ht || !key) return NULL;

  hash_val = (* ht->hash_function)(key) % ht->nbuckets;

  for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
      if ( strcmp(curr->data, key)==0){
        curr->fd=fd;
      }

return 0;
}


/**
 * Aggiunta di un messaggio all'interno della coda messaggi di un certo utente
 *
 * @param ht -- tabella hash
 * @param key -- il nome utente da aggiornare
 * @param msg -- il messaggio da inserire in codamessaggi
 *
 * @returns 0 in caso di successo, altrimenti -1
 */
int icl_add_message(icl_hash_t *ht, void * key, message_t msg){
  icl_entry_t* curr;
  unsigned int hash_val;

  if(!ht || !key) return -1;

  hash_val = (* ht->hash_function)(key) % ht->nbuckets;

  for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
    if (strcmp(curr->data, key)==0){
      inserimentoMessaggi(curr->coda, msg);
      return 0;
    }
  }


  return -1;
}


/**
 * Controlla se un utente è online o meno
 *
 * @param ht -- tabella hash
 * @param key -- il nome utente da controllare
 *
 * @ Restituisce il FD dell'utente in caso di successo, altrimenti -1
 */
int isOnline(icl_hash_t *ht, void * key){
  icl_entry_t* curr;
  unsigned int hash_val;

  if(!ht || !key) return -1;

  hash_val = (* ht->hash_function)(key) % ht->nbuckets;

  for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
    if (strcmp(curr->data, key)==0 && curr->online==1){
      return curr->fd;
    }
  }

  return -1;
}


/**
 * Controlla se un utente è registrato o meno
 *
 * @param ht -- tabella hash
 * @param key -- il nome utente da controllare
 *
 * @ Restituisce 0 in caso di successo, altrimenti -1
 */
int isRegistrato(icl_hash_t *ht, void * key){
  icl_entry_t* curr;
  unsigned int hash_val;

  if(!ht || !key) return -1;

  hash_val = (* ht->hash_function)(key) % ht->nbuckets;

  for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
    if (strcmp(curr->data, key)==0){

      return 0;
    }
  }


  return -1;
}


/**
 * Manda online un utente che ne fa richiesta
 *
 * @param ht -- tabella hash
 * @param key -- il nome utente da connettere
 * @param fd -- file descriptor dell'utente che si deve connettere
 *
 * @ Restituisce 0 in caso di successo, altrimenti -1
 */
int goOnline(icl_hash_t *ht, void * key,int fd){
  icl_entry_t* curr;
  unsigned int hash_val;

  if(!ht || !key) return -1;

  hash_val = (* ht->hash_function)(key) % ht->nbuckets;

  for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
    if (strcmp(curr->data, key)==0){
      curr->online=1;
      curr->fd=fd;
      return 0;
    }
  }


  return -1;
}


/**
 * Manda offline un utente che ne fa richiesta
 *
 * @param ht -- tabella hash
 * @param key -- il nome utente da disconnettere
 *
 * @ Restituisce 0 in caso di successo, altrimenti -1
 */
int goOffline(icl_hash_t *ht, void * key){
  icl_entry_t* curr;
  unsigned int hash_val;

  if(!ht || !key) return -1;

  hash_val = (* ht->hash_function)(key) % ht->nbuckets;

  for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
    if (strcmp(curr->data, key)==0 && curr->online==1){
      curr->online=0;
      curr->fd=-1;
      return 0;
    }
  }


  return -1;
}



/**
 * Serve per ottenere l'ID di un certo utente
 *
 * @param ht -- tabella hash
 * @param key -- il nome utente per cui vogliamo ottenere l'id
 *
 * @ Restituisce l'id dell'utente in caso di successo, altrimenti -1
 */
int icl_get_id(icl_hash_t *ht, char * user){
    icl_entry_t *curr;
    unsigned int hash_val;

    if(!ht || !user) return -1;
    hash_val = (*ht->hash_function)(user) % ht->nbuckets;

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next){
      if ( strcmp(curr->data, user)==0)
          return curr->id;
    }

return -1;
}
