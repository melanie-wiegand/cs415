EXEC = pseudo-shell
FLAGS = -g
CC = gcc

all: $(EXEC)

$(EXEC): main.o string_parser.o iobound.o signal.o
	$(CC) $(FLAGS) -o $@ $^

main.o: main.c
	$(CC) $(FLAGS) -c $<

string_parser.o: string_parser.c string_parser.h
	$(CC) $(FLAGS) -c $<

signal: signal.c
	gcc -o $@ $^

iobound: iobound.c
	gcc -o $@ $^

clean:
	rm -f *.o $(EXEC)
