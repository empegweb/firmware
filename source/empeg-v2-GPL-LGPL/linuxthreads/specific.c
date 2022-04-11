/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1996 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* Thread-specific data */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include "pthread.h"
#include "internals.h"

typedef void (*destr_function)(void *);

/* Table of keys. */

struct pthread_key_struct {
  int in_use;                   /* already allocated? */
  destr_function destr;         /* destruction routine */
};

static struct pthread_key_struct pthread_keys[PTHREAD_KEYS_MAX] =
  { { 0, NULL } };

/* Mutex to protect access to pthread_keys */

static pthread_mutex_t pthread_keys_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Create a new key */

int __pthread_key_create(pthread_key_t * key, destr_function destr)
{
  int i;

  pthread_mutex_lock(&pthread_keys_mutex);
  for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
    if (! pthread_keys[i].in_use) {
      /* Mark key in use */
      pthread_keys[i].in_use = 1;
      pthread_keys[i].destr = destr;
      pthread_mutex_unlock(&pthread_keys_mutex);
      *key = i;
      return 0;
    }
  }
  pthread_mutex_unlock(&pthread_keys_mutex);
  return EAGAIN;
}
strong_alias (__pthread_key_create, pthread_key_create)

/* Delete a key */

int pthread_key_delete(pthread_key_t key)
{
  pthread_descr self = thread_self();
  pthread_descr th;
  unsigned int idx1st, idx2nd;

  pthread_mutex_lock(&pthread_keys_mutex);
  if (key >= PTHREAD_KEYS_MAX || !pthread_keys[key].in_use) {
    pthread_mutex_unlock(&pthread_keys_mutex);
    return EINVAL;
  }
  pthread_keys[key].in_use = 0;
  pthread_keys[key].destr = NULL;
  /* Set the value of the key to NULL in all running threads, so
     that if the key is reallocated later by pthread_key_create, its
     associated values will be NULL in all threads. */
  idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
  idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;
  th = self;
  do {
    /* If the thread already is terminated don't modify the memory.  */
    if (!th->p_terminated && th->p_specific[idx1st] != NULL)
      th->p_specific[idx1st][idx2nd] = NULL;
    th = th->p_nextlive;
  } while (th != self);
  pthread_mutex_unlock(&pthread_keys_mutex);
  return 0;
}

/* Set the value of a key */

int __pthread_setspecific(pthread_key_t key, const void * pointer)
{
  pthread_descr self = thread_self();
  unsigned int idx1st, idx2nd;

  if (key >= PTHREAD_KEYS_MAX || !pthread_keys[key].in_use)
    return EINVAL;
  idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
  idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;
  if (THREAD_GETMEM_NC(self, p_specific[idx1st]) == NULL) {
    void *newp = calloc(PTHREAD_KEY_2NDLEVEL_SIZE, sizeof (void *));
    if (newp == NULL)
      return ENOMEM;
    THREAD_SETMEM_NC(self, p_specific[idx1st], newp);
  }
  THREAD_GETMEM_NC(self, p_specific[idx1st])[idx2nd] = (void *) pointer;
  return 0;
}
strong_alias (__pthread_setspecific, pthread_setspecific)

/* Get the value of a key */

void * __pthread_getspecific(pthread_key_t key)
{
  pthread_descr self = thread_self();
  unsigned int idx1st, idx2nd;

  if (key >= PTHREAD_KEYS_MAX)
    return NULL;
  idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
  idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;
  if (THREAD_GETMEM_NC(self, p_specific[idx1st]) == NULL
      || !pthread_keys[key].in_use)
    return NULL;
  return THREAD_GETMEM_NC(self, p_specific[idx1st])[idx2nd];
}
strong_alias (__pthread_getspecific, pthread_getspecific)

/* Call the destruction routines on all keys */

void __pthread_destroy_specifics()
{
  pthread_descr self = thread_self();
  int i, j, round, found_nonzero;
  destr_function destr;
  void * data;

  for (round = 0, found_nonzero = 1;
       found_nonzero && round < PTHREAD_DESTRUCTOR_ITERATIONS;
       round++) {
    found_nonzero = 0;
    for (i = 0; i < PTHREAD_KEY_1STLEVEL_SIZE; i++)
      if (THREAD_GETMEM_NC(self, p_specific[i]) != NULL)
        for (j = 0; j < PTHREAD_KEY_2NDLEVEL_SIZE; j++) {
          destr = pthread_keys[i * PTHREAD_KEY_2NDLEVEL_SIZE + j].destr;
          data = THREAD_GETMEM_NC(self, p_specific[i])[j];
          if (destr != NULL && data != NULL) {
            THREAD_GETMEM_NC(self, p_specific[i])[j] = NULL;
            destr(data);
            found_nonzero = 1;
          }
        }
  }
  for (i = 0; i < PTHREAD_KEY_1STLEVEL_SIZE; i++) {
    if (THREAD_GETMEM_NC(self, p_specific[i]) != NULL)
      free(THREAD_GETMEM_NC(self, p_specific[i]));
  }
}

/* Thread-specific data for libc. */

static int
libc_internal_tsd_set(enum __libc_tsd_key_t key, const void * pointer)
{
  pthread_descr self = thread_self();

  THREAD_SETMEM_NC(self, p_libc_specific[key], (void *) pointer);
  return 0;
}
int (*__libc_internal_tsd_set)(enum __libc_tsd_key_t key, const void * pointer)
     = libc_internal_tsd_set;

static void *
libc_internal_tsd_get(enum __libc_tsd_key_t key)
{
  pthread_descr self = thread_self();

  return THREAD_GETMEM_NC(self, p_libc_specific[key]);
}
void * (*__libc_internal_tsd_get)(enum __libc_tsd_key_t key)
     = libc_internal_tsd_get;