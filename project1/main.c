#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h" 

int main(int argc, char* argv[])
{
    int filemode = 0;

    // if -f flag is used, start file mode
    if (argc == 3 && strcmp(argv[1], "-f") == 0)
    {
        // update file mode bool
        filemode = 1;  
        printf("File mode:\n");

        // open provided file
        FILE *file = fopen(argv[2], "r");
        if (file == NULL)
        {
            perror("Could not open file");
            return 1;
        }
    }

    // if no flags, start interactive mode
    else if (argc == 1) 
    {
        // keep filemode bool at 0
        printf("Interactive mode:\n");

        // collect input from user
        FILE *input = stdin;  
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
            break; // EOF or error
        }

        // Parse the input into tokens
        command_line cmd = str_filler(line, " \t\n");

        if (cmd.num_token == 0) {
            free_command_line(&cmd);
            continue;
        }

        // STUB: Handle "lfcat" as a test
        if (strcmp(cmd.command_list[0], "lfcat") == 0) {
            fprintf(output, "Calling lfcat with arguments: ");
            for (int i = 1; i < cmd.num_token; i++) {
                fprintf(output, "%s ", cmd.command_list[i]);
            }
            fprintf(output, "\n");
        } else {
            fprintf(output, "Unknown command: %s\n", cmd.command_list[0]);
        }

        free_command_line(&cmd);
    }

    free(line);

    if (filemode)
    {
        fclose(file);
    }

    return 0;
}