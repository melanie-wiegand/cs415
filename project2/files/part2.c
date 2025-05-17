#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "string_parser.h"


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

    // store pids
    pid_t pid_array[MAX_PROCESSES];
    // store commands
    command_line cmd_array[MAX_PROCESSES];
    // num of processes
    int process_count = 0;

    // signal set
    sigset_t sigset;
    sigemptyset(&sigset);
    // add SIGUSR1 to set
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    while (1) 
    {
        if (getline(&line, &len, input) == -1) {
            break; 
        }

        // removing logic for semicolon parsing since not needed for this proj and i think it was messing up the signaling

        command_line cmd = str_filler(line, " \t\n");

        // check if str_filler returned null
        if (cmd.command_list == NULL || cmd.command_list[0] == NULL) {
            fprintf(stderr, "empty line\n");
            free_command_line(&cmd);
            continue;
        }

        // if empty line
        if (cmd.num_token == 0) 
        {
            free_command_line(&cmd);
            continue;
        }

        // make sure we didn't hit max processes
        if (process_count >= MAX_PROCESSES)
        {
            fprintf(stderr, "maximum processes reached! (%d processes)\n", MAX_PROCESSES);
            free_command_line(&cmd);
            break;
        }
    
        // create new fork
        pid_t pid = fork();
                    
        if (pid < 0)
        {
            // fork failed
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

        else // pid > 0 
        {
            // parent process
            pid_array[process_count] = pid;
            cmd_array[process_count] = cmd;
            process_count++;
        }

    }
    
    fclose(input);
    free(line);

    sleep(1);

    // sigusr1
    printf("Sending SIGUSR1 signal...\n");
    for (int i = 0; i < process_count; i++)
    {
        // sigusr1 signal ends waiting for all children (happens after all children ready)
        kill(pid_array[i], SIGUSR1);
    }

    // sleep(1);

    // sigstop
    printf("Sending SIGSTOP signal...\n");
    for (int i = 0; i < process_count; i++)
    {
        // printf("Sending SIGSTOP to process %d\n", pid_array[i]);
        kill(pid_array[i], SIGSTOP);
    }

    // sleep(1);

    // sigcont
    printf("Sending SIGCONT signal...\n");
    for (int i = 0; i < process_count; i++)
    {
        // printf("Sending SIGCONT to process %d\n", pid_array[i]);
        kill(pid_array[i], SIGCONT);
    }

    // sleep(1);

    // waitpid - parent waits for all children to die
    for (int i = 0; i < process_count; i++) {
        waitpid(pid_array[i], NULL, 0);
        // printf("Process %d finished.\n", pid_array[i]);
        free_command_line(&cmd_array[i]);
    }
    
    return 0;
}