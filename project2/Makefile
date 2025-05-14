EXEC = part1
CC = gcc
FLAGS = -g

all: $(EXEC) signal iobound cpubound

$(EXEC): part1.o string_parser.o
	$(CC) $(FLAGS) -o $@ $^

part1.o: part1.c string_parser.h
	$(CC) $(FLAGS) -c $<

string_parser.o: string_parser.c string_parser.h
	$(CC) $(FLAGS) -c $<

signal: signal.c
	$(CC) $(FLAGS) -o signal signal.c

iobound: iobound.c
	$(CC) $(FLAGS) -o iobound iobound.c

cpubound: cpubound.c
	$(CC) $(FLAGS) -o cpubound cpubound.c

clean:
	rm -f *.o $(EXEC) signal iobound cpubound