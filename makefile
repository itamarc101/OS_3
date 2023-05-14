GC = gcc
FLAGS = -Wall -g

default: all

all: stnc.c
	$(GC) $(FLAGS) -o stnc stnc.c

.PHONY: all clean

clean:
	rm -f *txt stnc

