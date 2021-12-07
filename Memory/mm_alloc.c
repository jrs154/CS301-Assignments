/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

meta_data* base_heap = NULL;

meta_data* new_meta_data(meta_data* prev, size_t size) {
    meta_data* block_new = sbrk(sizeof(meta_data) + size);
    block_new->prev = prev;
    block_new->next = NULL;
    block_new->free = 0;
    block_new->size = size;
    block_new->curr  = (void*) (sizeof(meta_data) + (address_t) block_new); // The boundary between metadata and allocated data block

    return block_new;
}



void* mm_malloc(size_t size) {
  //TODO: Implement malloc

  if (size == 0){
      return NULL;
  }

  if (base_heap == NULL) {
      base_heap = sbrk(0);
      void *block_data = new_meta_data(NULL, size)->curr;
      memset(block_data,0,size);
      return block_data;
  }

  meta_data* block_curr = base_heap;

  while (block_curr){
      if (block_curr->free) {
          if(size <= block_curr->size){
              
            size_t req_size = sizeof(meta_data) + size;

            if (req_size < block_curr-> size) {
                meta_data* block_new = (meta_data *) ((address_t) block_curr + sizeof(meta_data) + size);
                block_new->prev = block_curr;
                block_new->next = block_curr->next;
                block_new->free = 1;
                block_new->size = block_curr->size - size - sizeof(meta_data);
                block_new->curr = (void*) ((address_t) block_new + sizeof(meta_data));

                block_curr->next = block_new;
            }

            block_curr->free = 0;
            block_curr->size = size;

            void *block_data = block_curr->curr;
            memset(block_data,0,size);
            return block_data;
          }
      }

      if (block_curr->next == NULL)
          break;

      block_curr = block_curr->next;
  };

  block_curr->next = new_meta_data(block_curr, size);


  void *block_data=block_curr->next->curr;
  memset(block_data,0,size);
  return block_data;

  //return NULL;
}

void* mm_realloc(void* ptr, size_t size) {
  //TODO: Implement realloc

  if(ptr==NULL){
      return mm_malloc(size);
  }

  if(ptr!=NULL && size==0){
      mm_free(ptr);
      return NULL;
  }

 meta_data* block_old = (meta_data*) ((address_t) ptr - sizeof(meta_data));
 block_old->free = 1;


  char buf[block_old->size < size ? block_old->size : size];
  memmove(buf, ptr, block_old->size < size ? block_old->size : size);

  mm_free(ptr);

  meta_data* block_new = mm_malloc(size);
  
  if(!block_new){
      ptr = mm_malloc(block_old->size);
      memmove(ptr, buf, block_old->size);
      return NULL;
  }

  memmove(block_new, buf, block_old->size < size ? block_old->size : size); 

  return block_new->curr;


 // return NULL;
}


void mm_free(void* ptr) {
  //TODO: Implement free

  if(ptr==NULL){
      return;
  }

  meta_data* block_curr = base_heap;

  while (block_curr) {
      if (block_curr->curr == ptr) {
          block_curr->free = 1;

          if (block_curr->next != NULL && block_curr->next->free) {
              block_curr->next = block_curr->next->next;
              block_curr->size += sizeof(meta_data) + block_curr->next->size;
          }
          if (block_curr->prev != NULL && block_curr->prev->free) {
              block_curr->prev->next = block_curr->next;
              block_curr->prev->size +=  sizeof(meta_data) + block_curr->size;
          }

          break;
      }

      if (block_curr->next == NULL)
          break;

      block_curr = block_curr->next;
  }
}
