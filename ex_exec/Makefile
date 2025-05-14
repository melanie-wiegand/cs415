EXEC = project2
CC = gcc
FLAGS = -g

all: signal iobound

# $(EXEC): string_parser.o
# 	$(CC) $(FLAGS) -o $@ $^

# part1.o: string_parser.h
# 	$(CC) $(FLAGS) -c $<

# string_parser.o: string_parser.c string_parser.h
# 	$(CC) $(FLAGS) -c $<

signal: signal.c
	$(CC) $(FLAGS) -o signal signal.c

iobound: iobound.c
	$(CC) $(FLAGS) -o iobound iobound.c

clean:
	rm -f *.o signal iobound