all: pseudoshell

pseudoshell: main.o string_parser.o command.o
	gcc main.o string_parser.o command.o

main.o: main.c
	gcc -c main.c

string_parser.o: string_parser.c
	gcc -c string_parser.c

command.o: command.c
	gcc -c command.c
