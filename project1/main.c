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

        // Parse the input into tokens
        command_line cmd = str_filler(line, " \t\n");

        if (cmd.num_token == 0) {
            free_command_line(&cmd);
            continue;
        }

        // // STUB: Handle "lfcat" as a test
        // if (strcmp(cmd.command_list[0], "lfcat") == 0) {
        //     fprintf(output, "Calling lfcat with arguments: ");
        //     for (int i = 1; i < cmd.num_token; i++) {
        //         fprintf(output, "%s ", cmd.command_list[i]);
        //     }
        //     fprintf(output, "\n");
        // } else {
        //     fprintf(output, "Unknown command: %s\n", cmd.command_list[0]);
        // }

        if (strcmp(cmd.command_list[0], "ls") == 0) {
            listDir();
        }

        else if (strcmp(cmd.command_list[0], "pwd") == 0) {
            showCurrentDir();
        }

        if (strcmp(cmd.command_list[0], "mkdir") == 0) {
            if (cmd.num_token == 2)
            {
                makeDir(cmd.command_list[1]);
            }
            else
            {
                fprintf(stderr, "Usage:\n");
                fprintf(stderr, "mkdir <dirName>\n");
                return 1;
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
                fprintf(stderr, "cd <dirName>\n");
                return 1;
            }
        }

        // if (strcmp(cmd.command_list[0], "ls") == 0) {
        //     listDir();
        // }

        // if (strcmp(cmd.command_list[0], "ls") == 0) {
        //     listDir();
        // }

        // if (strcmp(cmd.command_list[0], "ls") == 0) {
        //     listDir();
        // }

        free_command_line(&cmd);
    }

    free(line);

    if (filemode)
    {
        fclose(input);
    }

    return 0;
}