EXEC = park
CC = gcc
CFLAGS = -g

all: $(EXEC)

park: park.o 
	$(CC) $(CFLAGS) -o $@ $^

park.o: park.c
	$(CC) $(CFLAGS) -c park.c

clean:
	rm -f *.o $(EXEC)