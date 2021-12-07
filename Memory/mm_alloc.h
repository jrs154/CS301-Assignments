/*
 * mm_alloc.h
 *
 * Exports a clone of the interface documented in "man 3 malloc".
 */

#pragma once

#ifndef _malloc_H_
#define _malloc_H_

#include <stdlib.h>

typedef unsigned long int address_t;

typedef struct meta {
  struct meta* prev;
  struct meta* next;
  int free;
  size_t size;
  void* curr;
} meta_data;


void* mm_malloc(size_t size);
void* mm_realloc(void* ptr, size_t size);
void mm_free(void* ptr);

#endif
