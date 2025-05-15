EXEC = part1 part2 part3 #part4 part5
CC = gcc
CFLAGS = -g

all: $(EXEC) iobound cpubound

part%: part%.o string_parser.o
	$(CC) $(CFLAGS) -o $@ $^

part%.o: part%.c string_parser.h
	$(CC) $(CFLAGS) -c $<

string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c $<

iobound: iobound.c
	$(CC) $(CFLAGS) -o $@ $<

cpubound: cpubound.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(EXEC) iobound cpubound