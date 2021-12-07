/*
 * Implementation of the word_count interface using Pintos lists.
 *
 * You may modify this file, and are expected to modify it.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PINTOS_LIST
#error "PINTOS_LIST must be #define'd when compiling word_count_l.c"
#endif

#include "word_count.h"

void init_words(word_count_list_t* wclist) { /* TODO */
  list_init(wclist);
}

size_t len_words(word_count_list_t* wclist) {
  /* TODO */
  return list_size(wclist);
}

word_count_t* find_word(word_count_list_t* wclist, char* word) {
  /* TODO */
  struct list_elem* i=list_begin(wclist);
  while(i!=list_end(wclist)){
    if(strcmp(word,list_entry(i,word_count_t,elem)->word)==0){
      return list_entry(i,word_count_t,elem);
    }
    i=list_next(i);
  }
  return NULL;
}

word_count_t* add_word(word_count_list_t* wclist, char* word) {
  /* TODO */
  word_count_t *temp=find_word(wclist,word);
  if(!temp){
    temp=malloc(sizeof(word_count_t));
    temp->word=word;
    temp->count=1;
    list_push_back(wclist,&temp->elem);
  }
  else{
    temp->count+=1;
    free(word);
  }
  return NULL;
}

void fprint_words(word_count_list_t* wclist, FILE* outfile) { /* TODO */
  struct list_elem* i=list_begin(wclist);
  while(i!=list_end(wclist)){
    fprintf(outfile, "%d\t%s\n",list_entry(i,word_count_t,elem)->count, list_entry(i,word_count_t,elem)->word);
    i=list_next(i);
  }

}

static bool less_list(const struct list_elem* ewc1, const struct list_elem* ewc2, void* aux) {
  /* TODO */
  word_count_t *var1=list_entry(ewc1,word_count_t,elem);
  word_count_t *var2=list_entry(ewc2,word_count_t,elem);
  bool (*comp)(const word_count_t *, const word_count_t *) = aux;

  return comp(var1,var2);
}

void wordcount_sort(word_count_list_t* wclist,
                    bool less(const word_count_t*, const word_count_t*)) {
  list_sort(wclist, less_list, less);
}
