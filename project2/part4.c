#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "string_parser.h"


#define MAX_PROCESSES 100
#define QUANTUM 1

// void alarm_handle(int signum)
// {
//     printf("context switch");
//     alarm(QUANTUM);
// }

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
    int signum;
    sigset_t sigset;
    sigemptyset(&sigset);

    // // add SIGUSR1 to set
    // sigaddset(&sigset, SIGUSR1);

    // add signals to set
    // sigaddset(&sigset, SIGCONT);
    sigaddset(&sigset, SIGALRM);

    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // if context switch
    // signal(SIGALRM, alarm_handle);

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
            // int signum;
            sigwait(&sigset, &signum);

            if (execvp(cmd.command_list[0], cmd.command_list) == -1) 
            {
                perror("execvp failed");
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


    // wake up children
    for (int i = 0; i < process_count; i++) {
        kill(pid_array[i], SIGALRM); // unblock
        kill(pid_array[i], SIGSTOP); // halt
    }


    // round robin starts on all child processes
    printf("Beginning round robin\n");

    int index = 0;
    // array to track status of each process
    int done[MAX_PROCESSES] = {0};
    int num_done = 0;
    int current = -1;


    alarm(QUANTUM);

    // if there are unfinished processes
    while(num_done < process_count)
    {
        sigwait(&sigset, &signum);
        printf("\nReceived SIGALRM signal...\n");

        if (current != -1 && done[current] == 0)
        {
            int status;
            pid_t result = waitpid(pid_array[current], &status, WNOHANG);
            if (result == 0) {
                printf("\tStopping Process: [%d]\n", pid_array[current]);
                kill(pid_array[current], SIGSTOP);
            } else {
                done[current] = 1;
                printf("\tProcess [%d] has finished.\n", pid_array[current]);
                num_done++;
            }
        }

        printf("\tFinding next process to run...\n");
        // check if there is another unfinished process
        int found = 0;
        int next = index;
        for (int i = 0; i < process_count; i++)
        {
            next++;
            next = next % process_count;
            if (done[next] != 0)
            {
                printf("\tProcess: %d is complete.\n", pid_array[next]);
            }
            else
            {
                // found an unfinished process
                found = 1;
                break;
            }
        }

        if (!found)
        {
            // no more unfinished processes
            break;
        }

        index = next;

        

        // int status;

        // if (waitpid(pid_array[index], &status, WNOHANG) == pid_array[index])
        // {
        //     printf("\tProcess: %d has finished.\n", pid_array[index]);
        //     done[index] = 1;
        //     num_done++;
        //     current = -1;
        //     alarm(QUANTUM);
        //     continue;

        // }

        printf("\tContinuing Process: [%d]\n", pid_array[index]);
        kill(pid_array[index], SIGCONT);
        current = index;

        alarm(QUANTUM);
    }



    // waitpid - parent waits for all children to die
    for (int i = 0; i < process_count; i++) {
        waitpid(pid_array[i], NULL, 0);
        // printf("Process %d finished.\n", pid_array[i]);
        free_command_line(&cmd_array[i]);
    }

    printf("\n\tAll processes have finished :)\n");
    
    return 0;
}