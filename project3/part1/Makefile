# proj3 hints in class
# agent simulation with "agents" representing entities that do stuff
# 2 types of agents: people that move around, roller coaster cars

# make a bunch of people, put them in park, tell them how long to be there (random) (sleep)
# wakes up, joins queue for coaster
# ticket booth services queue in order, then person enters coaster queue
# coaster has limit for number of people

# queue implementation is like a semaphore

# how long should car wait to fill up before going (not full)?

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