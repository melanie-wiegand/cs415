EXEC = pseudo-shell
FLAGS = -g
CC = gcc

all: $(EXEC)

$(EXEC): main.o string_parser.o command.o
	$(CC) $(FLAGS) -o $@ $^

main.o: main.c
	$(CC) $(FLAGS) -c $<

# %.o: %.c %.h
# 	$(CC) $(FLAGS) -c $< 

string_parser.o: string_parser.c string_parser.h
	$(CC) $(FLAGS) -c $<

command.o: command.c command.h
	$(CC) $(FLAGS) -c $<

clean:
	rm -f *.o $(EXEC)
