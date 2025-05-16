EXEC = pseudoshell
FLAGS = -g -o
CC = gcc

all: $(EXEC)

$(EXEC): main.o string_parser.o command.o
	$(CC) -o $@ $^

main.o: main.c
	$(CC) -c $^

%.o: %.c %.h
	$(CC) -c $< 

string_parser.o: string_parser.c string_parser.h
	$(CC) -c $<

command.o: command.c command.h
	$(CC) -c $<

clean:
	rm -f *.o $(EXEC)
