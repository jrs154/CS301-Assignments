EXECUTABLES=words lwords
CC=gcc
CFLAGS=-g3 -Wall -std=gnu99

.PHONY: all clean

all: $(EXECUTABLES)

words: words.o word_helpers.o word_count.o
lwords: lwords.o word_count_l.o word_helpers.o list.o debug.o

$(EXECUTABLES):
	$(CC) $^ -o $@

word_count_l.o: word_count_l.c

word_count_l.o:
	$(CC) $(CFLAGS) -DPINTOS_LIST -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.ONESHELL:
clean:
	tmp_dir=`mktemp -d`
	cp words.o lwords.o word_count.o word_helpers.o $$tmp_dir
	rm -f $(EXECUTABLES) *.o
	cp $${tmp_dir}/*.o ./
	rm -r $$tmp_dir
