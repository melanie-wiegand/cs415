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

    // "proc/<pid>/stat" is path we want
    // print path name into path string
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    // open path
    FILE *stats = fopen(path, "r");
    if (stats == NULL)
    {
        perror("Could not open stats file");
        return;
    }

    // from https://stackoverflow.com/questions/39066998/what-are-the-meaning-of-values-at-proc-pid-stat
    // https://man7.org/linux/man-pages/man5/proc_pid_stat.5.html
    /* desired fields:
            state (3) %c
            time in user mode (14) %lu
            time in kernel mode (15) %lu
            priority (18) %ld
            # of threads (20) %ld
            address of stack (28) %lu
    */

    char state;
    unsigned long utime;
    unsigned long ktime;
    long priority;
    long threads;
    unsigned long stack_addr;

    // dummy variables to skip fields we don't need
    int skipint;
    char skipstring[256];
    unsigned long skipul;    

    // skip fields 1-2
    fscanf(stats, "%d (%[^)])", &skipint, skipstring);

    // state (3)
    fscanf(stats, " %c", &state);

    // skip fields 4-13
    for (int i = 0; i < 10; i++)
    {
        fscanf(stats, " %lu", &skipul);
    }

    // user time
    fscanf(stats, " %lu", &utime);

    // kernel time
    fscanf(stats, " %lu", &ktime);

    // skip 16-17
    for (int i = 0; i < 2; i++)
    {
        fscanf(stats, " %lu", &skipul);
    }

    // priority
    fscanf(stats, " %ld", &priority);

    // skip 19
    fscanf(stats, " %lu", &skipul);

    // # of threads
    fscanf(stats, " %ld", &threads);

    // 21-27 are longs
    for (int i = 0; i < 7; i++)
    {
        fscanf(stats, " %lu", &skipul);
    }

    // stack address
    fscanf(stats, " %lu", &stack_addr);

    fclose(stats);


    printf("Process %-7d:   %-5c | %-20lu | %-20lu | %-10ld | %-18ld | %-18lu\n",
       pid, state, utime, ktime, priority, threads, stack_addr);
}


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
    // printf("Beginning round robin\n");

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
        // printf("\nReceived SIGALRM signal...\n");

        if (current != -1 && done[current] == 0)
        {
            int status;
            pid_t result = waitpid(pid_array[current], &status, WNOHANG);
            if (result == 0) 
            {
                // printf("\tStopping Process: [%d]\n", pid_array[current]);
                kill(pid_array[current], SIGSTOP);
            } 
            else 
            {
                done[current] = 1;
                // printf("\tProcess [%d] has finished.\n", pid_array[current]);
                num_done++;
            }
        }

        // printf("\tFinding next process to run...\n");
        // check if there is another unfinished process
        int found = 0;
        int next = index;
        for (int i = 0; i < process_count; i++)
        {
            next++;
            next = next % process_count;
            if (done[next] != 0)
            {
                // printf("\tProcess: %d is complete.\n", pid_array[next]);
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
        printf("\n\t\t | %-5s | %-20s | %-20s | %-10s | %-18s | %-18s\n",
            "State", "User Time", "Kernel Time",
            "Priority", "# Threads", "Stack Address");
        printf("----------------------------------------------------------------------------------------------------------------------\n");

        // printf("========= MCP Process Stats =========\n");
        for (int i = 0; i < process_count; i++)
        {
            if (done[i] == 0)
            {
                // printf("\nProcess [%d]:\n", pid_array[i]);
                print_stats(pid_array[i]);
            }
            else
            {
                printf("Process %d has completed. No information to display\n", pid_array[i]);

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

    printf("\nAll processes have finished :)\n\n");
    
    return 0;
}