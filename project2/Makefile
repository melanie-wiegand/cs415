EXEC = part1 part2 part3 part4
CC = gcc
FLAGS = -g

all: $(EXEC) iobound cpubound

$(EXEC): part1.o string_parser.o
	$(CC) $(FLAGS) -o $@ $^

part1.o: part1.c string_parser.h
	$(CC) $(FLAGS) -c $<

part2.o: part2.c string_parser.h
	$(CC) $(FLAGS) -c $<

part3.o: part3.c string_parser.h
	$(CC) $(FLAGS) -c $<

part4.o: part4.c string_parser.h
	$(CC) $(FLAGS) -c $<

# part5.o: part5.c string_parser.h
# 	$(CC) $(FLAGS) -c $<

string_parser.o: string_parser.c string_parser.h
	$(CC) $(FLAGS) -c $<

iobound: iobound.c
	$(CC) $(FLAGS) -o iobound iobound.c

cpubound: cpubound.c
	$(CC) $(FLAGS) -o cpubound cpubound.c

clean:
	rm -f *.o $(EXEC) iobound cpubound