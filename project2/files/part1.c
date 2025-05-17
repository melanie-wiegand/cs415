#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "string_parser.h" 

#define MAX_LINE 1024
#define MAX_PROCESSES 100

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    FILE *input = NULL;
    input = fopen(argv[1], "r");

    if (input == NULL)
    {
        perror("Could not open input file");
        return 1;
    }

    char *line = NULL;
    size_t len = 0;

    pid_t pid_array[MAX_PROCESSES];
    int process_count = 0;

    while (1) 
    {
        if (getline(&line, &len, input) == -1) {
            break; 
        }

        char *split;
        // separate inputs by semicolon delimeter
        char *curcmd = strtok_r(line, ";", &split);

        while (curcmd != NULL)
        {
            // same string parsing as in proj1
            // tokenize input
            command_line cmd = str_filler(curcmd, " \t\n");

    
            if (cmd.num_token == 0) 
            {
                free_command_line(&cmd);
                curcmd = strtok_r(NULL, ";", &split);
                continue;
            }
        
            pid_t pid = fork();
            
            if (pid < 0)
            {
                perror("error forking");
                free_command_line(&cmd);
                // curcmd = strtok_r(NULL, ";", &split);
                // continue;
            }

            else if (pid == 0)
            {
                // child process
                if (execvp(cmd.command_list[0], cmd.command_list) == -1) 
                {
                    perror("execvp failed");
                    // free_command_line(&cmd); #don't need to free here bc could double free mem
                    exit(1);
                }
            }
            else 
            {
                // parent process
                pid_array[process_count] = pid;
                process_count++;
            }

            free_command_line(&cmd);
            curcmd = strtok_r(NULL, ";", &split);
        }
    }
    
    fclose(input);
    free(line);

    //launching processes
    for (int i = 0; i < process_count; i++)
    {
        int status;
        pid_t finished = waitpid(pid_array[i], &status, 0);
        if (finished == -1)
        {
            perror("waitpid failed");
        }
    }

    return 0;
}