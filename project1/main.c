#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h" 
#include "command.h"

int main(int argc, char* argv[])
{
    FILE *input = NULL;
    FILE *output = NULL;
    int filemode = 0;

    // if -f flag is used, start file mode
    if (argc == 3 && strcmp(argv[1], "-f") == 0)
    {
        // update file mode bool
        filemode = 1;  
        printf("File mode:\n");

        // open provided file
        input = fopen(argv[2], "r");
        if (input == NULL)
        {
            perror("Could not open input file");
            return 1;
        }

        output = fopen("output.txt", "w");
        if (!output) {
            perror("Error opening output.txt");
            fclose(input);
            return 1;
        }
    }

    // if no flags, start interactive mode
    else if (argc == 1) 
    {
        // keep filemode bool at 0
        printf("Interactive mode:\n");

        // collect input from user
        input = stdin;  
        output = stdout;
    }

    // otherwise, give error message
    else 
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s           (for interactive mode)\n", argv[0]);
        fprintf(stderr, "  %s -f <file> (for file mode)\n", argv[0]);
        return 1;
    }


    // command loop
    char *line = NULL;
    size_t len = 0;

    while (1) {

        if (!filemode) {
            fprintf(output, "pseudo-shell> ");
            fflush(output); // ensure prompt appears
        }

        if (getline(&line, &len, input) == -1) {
            break; 
        }

        char *split;
        char *curcmd = strtok_r(line, ";", &split);

        while (curcmd != NULL)
        {
        // Parse the input into tokens
            command_line cmd = str_filler(curcmd, " \t\n");

            if (cmd.num_token == 0) 
            {
                free_command_line(&cmd);
                curcmd = strtok_r(NULL< ";", &split);
                continue;
            }

        

            if (strcmp(cmd.command_list[0], "exit") == 0) {
                break;
            }

            if (strcmp(cmd.command_list[0], "ls") == 0) {
                if (cmd.num_token == 1)
                {
                    listDir();
                }
                else
                {
                    fprintf(stderr, "Unrecognized parameter for command \"ls\"\n");
                }
                
            }

            else if (strcmp(cmd.command_list[0], "pwd") == 0) {
                if (cmd.num_token == 1)
                {
                    showCurrentDir();
                }
                else
                {
                    fprintf(stderr, "Unrecognized parameter for command \"pwd\"\n");
                }
                
            }

            if (strcmp(cmd.command_list[0], "mkdir") == 0) {
                if (cmd.num_token == 2)
                {
                    makeDir(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tmkdir <dirName>\n");
                }
                
            }

            if (strcmp(cmd.command_list[0], "cd") == 0) {
                if (cmd.num_token == 2)
                {
                    changeDir(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tcd <dirName>\n");
                }
            }

            if (strcmp(cmd.command_list[0], "cp") == 0) {
                if (cmd.num_token == 3)
                {
                    copyFile(cmd.command_list[1], cmd.command_list[2]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tcp <src> <dst>\n");
                }
            }

            if (strcmp(cmd.command_list[0], "mv") == 0) {
                if (cmd.num_token == 3)
                {
                    moveFile(cmd.command_list[1], cmd.command_list[2]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tmv <src> <dst>\n");
                }
            }

            if (strcmp(cmd.command_list[0], "rm") == 0) {
                if (cmd.num_token == 2)
                {
                    deleteFile(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\trm <file>\n");
                }
            }

            if (strcmp(cmd.command_list[0], "cat") == 0) {
                if (cmd.num_token == 2)
                {
                    displayFile(cmd.command_list[1]);
                }
                else
                {
                    fprintf(stderr, "Usage:\n");
                    fprintf(stderr, "\tcat <file>\n");
                }
            }

            free_command_line(&cmd);
            // go to next cmd
            curcmd = strtok_r(NULL, ";", &split);
        }
    }

    free(line);

    if (filemode)
    {
        fclose(input);
    }

    return 0;
}