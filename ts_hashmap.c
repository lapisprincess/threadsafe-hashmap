#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ts_hashmap.h"

/**
 * Creates a new thread-safe hashmap. 
 *
 * @param capacity initial capacity of the hashmap.
 * @return a pointer to a new thread-safe hashmap.
 */
ts_hashmap_t *initmap(int capacity) {
  ts_hashmap_t *out = (ts_hashmap_t*) malloc(sizeof(ts_hashmap_t));
  ts_entry_t **table = (ts_entry_t**) malloc(capacity * sizeof(ts_entry_t));
  for (int i = 0; i < capacity; i++) 
    table[i] = NULL;
  int size = capacity * sizeof(pthread_mutex_t);
  pthread_mutex_t **locks = (pthread_mutex_t**) malloc(size);
  for (int i = 0; i < capacity; i++) {
    locks[i] = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(locks[i], NULL);
  }
  out->table = table;
  out->capacity = capacity;
  out->locks = locks;
  out->numOps = 0;
  out->size = 0;
  return out;
}

/**
 * Obtains the value associated with the given key.
 * @param map a pointer to the map
 * @param key a key to search
 * @return the value associated with the given key, or INT_MAX if key not found
 */
int get(ts_hashmap_t *map, int key) {
  map->numOps++;
  int index = ((unsigned int) key) % map->capacity;
  
  // check if index is locked... if so, sleep

  if (pthread_mutex_trylock(map->locks[index]) == 0) {
    ;
  }
  //pthread_mutex_lock(map->locks[index]);
  ts_entry_t *entry = map->table[index];
  while (entry != NULL) {
    if (entry->key == key) 
      return entry->value;
    entry = entry->next;
  }
  //pthread_mutex_unlock(map->locks[index]);
  return INT_MAX;
}

/**
 * Associates a value associated with a given key.
 * @param map a pointer to the map
 * @param key a key
 * @param value a value
 * @return old associated value, or INT_MAX if the key was new
 */
int put(ts_hashmap_t *map, int key, int value) {
  map->numOps++;
  int index = ((unsigned int) key) % map->capacity, temp;
  pthread_mutex_lock(map->locks[index]);
  ts_entry_t *cur_entry = map->table[index];
  ts_entry_t *new_entry = (ts_entry_t*) malloc(sizeof(ts_entry_t));
  new_entry->key = key; 
  new_entry->value = value; 
  new_entry->next = NULL;
  if (cur_entry == NULL) {
    map->table[index] = new_entry;
    map->size++;
    pthread_mutex_unlock(map->locks[index]);
    return INT_MAX;         // no entry at index
  }
  while (cur_entry != NULL) {
    if (cur_entry->key == key) {
      free(new_entry);
      temp = cur_entry->value;
      cur_entry->value = value;
      pthread_mutex_unlock(map->locks[index]);
      return temp;          // key already exists
    }
    if (cur_entry->next == NULL) {
      cur_entry->next = new_entry;
      map->size++;
      pthread_mutex_unlock(map->locks[index]);
      return INT_MAX;       // add to end of index
    }
    cur_entry = cur_entry->next;
  }
  pthread_mutex_unlock(map->locks[index]);
  return -1;                // put unsuccessful
}

/**
 * Removes an entry in the map
 * @param map a pointer to the map
 * @param key a key to search
 * @return the value associated with the given key, or INT_MAX if key not found
 */
int del(ts_hashmap_t *map, int key) {
  map->numOps++;
  int index = (unsigned int) key % map->capacity;
  if (pthread_mutex_trylock(map->locks[index]) == 0) {
    ;
  }
  ts_entry_t *entry = map->table[index];
  if (entry == NULL) 
    return INT_MAX;
  if (entry->key == key) {
    map->size--;
    int val = entry->value;
    map->table[index] = entry->next;
    free(entry);
    return val;                // target key at index head
  }
  ts_entry_t *next_entry = entry->next;
  while (next_entry != NULL) {
    if (next_entry->key == key) {
      map->size--;
      int val = next_entry->value;
      entry->next = next_entry->next;
      free(next_entry);
      return val;             // target key inside index
    } 
    entry = entry->next;
    next_entry = entry->next;
  }
  return INT_MAX;             // no target found
}


/**
 * Prints the contents of the map
 */
void printmap(ts_hashmap_t *map) {
  for (int i = 0; i < map->capacity; i++) {
    printf("[%d] -> ", i);
    ts_entry_t *entry = map->table[i];
    while (entry != NULL) {
      printf("(%d,%d)", entry->key, entry->value);
      if (entry->next != NULL)
        printf(" -> ");
      entry = entry->next;
    }
    printf("\n");
  }
}

/**
 * Free up the space allocated for hashmap
 * @param map a pointer to the map
 */
void freeMap(ts_hashmap_t *map) {
  for (int i = 0; i < map->capacity; i++) {
    ts_entry_t *entry = map->table[i];
    while (entry != NULL) {
      ts_entry_t *next = entry->next;
      free(entry);
      map->size--;
      entry = next;
    }
    free(map->locks[i]);
  }
  free(map->locks);
  free(map->table);
  free(map);
}