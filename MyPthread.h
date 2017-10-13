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


#if !defined(PTHREAD_H_)
#define PTHREAD_H_

/*
  Versione re-implementata della pthread_create con controlli
*/
void Pthread_create(pthread_t *t, const pthread_attr_t *attr, void * (* function)(), void *arg);

/*
  Versione re-implementata della pthread_mutex_lock con controlli
*/
void Pthread_mutex_lock(pthread_mutex_t *mutex);

/*
  Versione re-implementata della pthread_mutex_unlock con controlli
*/
void Pthread_mutex_unlock(pthread_mutex_t *mutex);

/*
  Versione re-implementata della pthread_cond_wait con controlli
*/
void Pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

/*
  Versione re-implementata della pthread_cond_signal con controlli
*/
void Pthread_cond_signal(pthread_cond_t *cond);

/*
  Versione re-implementata della Pthread_cond_broadcast con controlli
*/
void Pthread_cond_broadcast(pthread_cond_t *cond);

/*
  Versione re-implementata della pthread_join con controlli
*/
void Pthread_join(pthread_t t, void *status);

/*
  Versione re-implementata della Pthread_attr_destroy con controlli
*/
void Pthread_attr_destroy(pthread_attr_t attr);

/*
  Versione re-implementata della Pthread_mutex_init con controlli
*/
void Pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t *restrict attr);

#endif
