#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

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
    command_line cmd_array[MAX_PROCESSES];
    int process_count = 0;


    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    while (1) 
    {
        if (getline(&line, &len, input) == -1) {
            break; 
        }

        char *split;
        // separate inputs by semicolon delimeter
        char *curcmd = strtok_r(line, ";", &split);

        while (curcmd != NULL && process_count < MAX_PROCESSES)
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
                continue;
            }

            else if (pid == 0)
            {
                // child process
                int signum;
                sigwait(&sigset, &signum);
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
                cmd_array[process_count] = cmd;
                process_count++;
            }

            // free_command_line(&cmd);
            curcmd = strtok_r(NULL, ";", &split);
        }
    }
    
    fclose(input);
    free(line);

    // sigusr1
    for (int i = 0; i < process_count; i++)
    {
        printf("[MCP] Sending SIGUSR1 to process %d\n", pid_array[i]);
        kill(pid_array[i], SIGUSR1);
    }

    sleep(1);

    // sigstop
    for (int i = 0; i < process_count; i++)
    {
        printf("[MCP] Sending SIGSTOP to process %d\n", pid_array[i]);
        kill(pid_array[i], SIGSTOP);
    }

    sleep(1);

    // sigcont
    for (int i = 0; i < process_count; i++)
    {
        printf("[MCP] Sending SIGCONT to process %d\n", pid_array[i]);
        kill(pid_array[i], SIGCONT);
    }

    sleep(1);

    // waitpid
    for (int i = 0; i < process_count; i++) {
        waitpid(pid_array[i], NULL, 0);
        printf("[MCP] Process %d finished.\n", pid_array[i]);
        free_command_line(&cmd_array[i]);
    }
    
    return 0;
}