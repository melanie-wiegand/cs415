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

void print_stats(pid_t pid)
{
    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *stats = fopen(path, "r");
    if (!stats) {
        perror("Could not open stat file");
        return;
    }

    int pid_read;
    char comm[256];
    char state;
    unsigned long utime, ktime, stack_addr;
    long priority, threads;

    // Skip first 2 fields: pid (%d), comm (%s but inside parentheses)
    // We read comm as a string in parentheses using %[^(] and %*c to skip the closing ')'
    fscanf(stats, "%d ", &pid_read);
    fscanf(stats, " (%[^)])", comm); // read everything inside parentheses
    fscanf(stats, " %c", &state);    // now read the state

    // Now skip fields 4 to 13 (10 fields total)
    for (int i = 0; i < 10; ++i) {
        unsigned long dummy;
        fscanf(stats, " %lu", &dummy);
    }

    // Field 14: user mode time
    fscanf(stats, " %lu", &utime);

    // Field 15: kernel mode time
    fscanf(stats, " %lu", &ktime);

    // Skip fields 16–17
    for (int i = 0; i < 2; ++i) {
        unsigned long dummy;
        fscanf(stats, " %lu", &dummy);
    }

    // Field 18: priority
    fscanf(stats, " %ld", &priority);

    // Field 19: nice (skip)
    unsigned long dummy;
    fscanf(stats, " %lu", &dummy);

    // Field 20: number of threads
    fscanf(stats, " %ld", &threads);

    // Skip fields 21–27
    for (int i = 0; i < 7; ++i) {
        fscanf(stats, " %lu", &dummy);
    }

    // Field 28: stack address
    fscanf(stats, " %lu", &stack_addr);

    fclose(stats);

    // Print result
    printf("Process %d:\t| %c\t|\t%lu\t|\t%lu\t|\t%ld\t|\t%ld\t|\t%lu\n",
           pid, state, utime, ktime, priority, threads, stack_addr);
}


// void print_stats(pid_t pid)
// {
//     char path[128];
//     snprintf(path, sizeof(path), "/proc/%d/stat", pid);

//     FILE *stats = fopen(path, "r");
//     if (stats == NULL)
//     {
//         perror("Could not open stat file");
//         return;
//     }

//     char buffer[2048];
//     if (fgets(buffer, sizeof(buffer), stats) == NULL)
//     {
//         perror("Failed to read stat file");
//         fclose(stats);
//         return;
//     }
//     fclose(stats);

//     // Step 1: extract pid, comm, state
//     int pid_read;
//     char comm[256];
//     char state;

//     sscanf(buffer, "%d (%[^)]) %c", &pid_read, comm, &state);

//     // Step 2: tokenize to get the rest of the fields
//     // After pid (field 1), comm (2), state (3) — we need to skip to fields 14, 15, 18, 20, and 28
//     // We’ll use strtok starting *after* the state field
//     char *ptr = strchr(buffer, ')'); // find closing parenthesis
//     if (!ptr) {
//         fprintf(stderr, "Malformed stat file\n");
//         return;
//     }
//     ptr++; // move past ')'
//     int field = 3;

//     // Tokenize from field 4 onward
//     char *token = strtok(ptr, " ");
//     unsigned long utime = 0, ktime = 0, stack_addr = 0;
//     long priority = 0, threads = 0;

//     while (token && field <= 28)
//     {
//         field++;
//         if (field == 14)
//             utime = strtoul(token, NULL, 10);
//         else if (field == 15)
//             ktime = strtoul(token, NULL, 10);
//         else if (field == 18)
//             priority = strtol(token, NULL, 10);
//         else if (field == 20)
//             threads = strtol(token, NULL, 10);
//         else if (field == 28)
//             stack_addr = strtoul(token, NULL, 10);

//         token = strtok(NULL, " ");
//     }

//     // Final formatted output line
//     printf("Process %d:\t| %c\t|\t%lu\t|\t%lu\t|\t%ld\t|\t%ld\t|\t%lu\n",
//            pid, state, utime, ktime, priority, threads, stack_addr);
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
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);


    while (1) 
    {
        if (getline(&line, &len, input) == -1) {
            break; 
        }

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
            if (result == 0) 
            {
                printf("\tStopping Process: [%d]\n", pid_array[current]);
                kill(pid_array[current], SIGSTOP);
            } 
            else 
            {
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

        

        printf("\tContinuing Process: [%d]\n", pid_array[index]);
        kill(pid_array[index], SIGCONT);
        current = index;

        system("clear");
        printf("\t\t\tState\t| Time in User Mode\t| Time in Kernel Mode\t| Priority\t| Number of Threads\t| Address of Stack\n");
        printf("__________________________________________________________________________________________________________________________________________\n");
        
        // printf("========= MCP Process Stats =========\n");
        for (int i = 0; i < process_count; i++)
        {
            if (done[i] == 0)
            {
                // printf("\nProcess [%d]:\n", pid_array[i]);
                print_stats(pid_array[i]);
            }
        }

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