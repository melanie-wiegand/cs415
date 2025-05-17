// priority round robin

// choose some stat file to be the priority

/*  going to check whether a process is CPU bound or IO bound 
    based on how much time it spends in user/kernel mode,
    cpu bound gets more time in RR (2s), io bound gets less (1s)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "string_parser.h"


#define MAX_PROCESSES 100
// #define QUANTUM 1

// putting all process info into a struct to pass out of the get_stats func
typedef struct 
{
    pid_t pid;
    char state;
    unsigned long utime;
    unsigned long ktime;
    long priority;
    long threads;
    unsigned long stack_addr;
    int quantum;
    char* type;
    int done;
} pinfo;

// separate function to get stats so we can call in loop
void get_stats(pinfo *pinfo, int isrunning)
{
    char path[128];

    // "proc/<pid>/stat" is path we want
    // print path name into path string
    snprintf(path, sizeof(path), "/proc/%d/stat", pinfo->pid);

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

    // dummy variables to skip fields we don't need
    int skipint;
    char skipstring[256];
    unsigned long skipul;    

    // skip fields 1-2
    fscanf(stats, "%d (%[^)])", &skipint, skipstring);

    // state (3)
    fscanf(stats, " %c", &pinfo->state);

    // manually set state if currently running
    if (isrunning) {
        pinfo->state = 'R';
    }

    // skip fields 4-13
    for (int i = 0; i < 10; i++)
    {
        fscanf(stats, " %lu", &skipul);
    }

    // user time
    fscanf(stats, " %lu", &pinfo->utime);

    // kernel time
    fscanf(stats, " %lu", &pinfo->ktime);

    // skip 16-17
    for (int i = 0; i < 2; i++)
    {
        fscanf(stats, " %lu", &skipul);
    }

    // priority
    fscanf(stats, " %ld", &pinfo->priority);

    // skip 19
    fscanf(stats, " %lu", &skipul);

    // # of threads
    fscanf(stats, " %ld", &pinfo->threads);

    // 21-27 are longs
    for (int i = 0; i < 7; i++)
    {
        fscanf(stats, " %lu", &skipul);
    }

    // stack address
    fscanf(stats, " %lu", &pinfo->stack_addr);

    fclose(stats);

    //determine if cpu bound or io bound:

    if (pinfo->utime >= pinfo->ktime)
    {
        // cpu bound spends more time in user mode
        pinfo->type = "CPU bound";
        pinfo->quantum = 2; // bigger quantum for cpu bound   
    }
    else
    {
        pinfo->type = "I/O bound";
        pinfo->quantum = 1; // smaller for io bound
    }  
}

// separate function to print
void print_stats(pinfo *pinfo)
{
    printf("|   %-9d | %-7c | %-15lu | %-15lu | %-10ld | %-13ld | %-15lu | %-10s | %-10d |\n",
        pinfo->pid, pinfo->state, pinfo->utime, pinfo->ktime, pinfo->priority, pinfo->threads, 
        pinfo->stack_addr, pinfo->type, pinfo->quantum);
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

    // store commands
    command_line cmd_array[MAX_PROCESSES];
    // num of processes

    // replace pid_array with process info array (use parray[i].pid instead)
    pinfo parray[MAX_PROCESSES];
    int process_count = 0;

    // signal set
    int signum;

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigaddset(&sigset, SIGUSR1);
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
                    
        if (pid < 0) // fork failed
        {
            perror("error forking");
            free_command_line(&cmd);
            continue;
        }

        else if (pid == 0) // child process
        {
            // int signum;
            int wait = sigwait(&sigset, &signum);
            if (wait != 0)
            {
                perror("sigwait failed");
                exit(1);
            }

            if (execvp(cmd.command_list[0], cmd.command_list) == -1) 
            {
                perror("execvp failed");
                exit(1);
            }
        }

        else // pid > 0 (parent process)
        {
            // set pid
            parray[process_count].pid = pid;
            // set done status
            parray[process_count].done = 0;

            // set cur cmd
            cmd_array[process_count] = cmd;

            // increment
            process_count++;
        }

    }
    
    fclose(input);
    free(line);


    // wake up children
    for (int i = 0; i < process_count; i++) {
        kill(parray[i].pid, SIGUSR1); // unblock
        // printf("Started child %d\n", parray[i].pid);
        kill(parray[i].pid, SIGSTOP); // halt
    }


    // round robin starts on all child processes
    // printf("Beginning round robin\n");

    int index = 0;
    int num_done = 0;
    int current = -1;

    // placeholder alarm until we determine quantum times
    alarm (1);

    // if there are unfinished processes
    while(num_done < process_count)
    {
        sigwait(&sigset, &signum);
        // printf("\nReceived SIGALRM signal...\n");

        if (current != -1 && parray[current].done == 0)
        {
            int status;
            pid_t result = waitpid(parray[current].pid, &status, WNOHANG);
            if (result == 0) 
            {
                // printf("\tStopping Process: [%d]\n", pid_array[current]);
                kill(parray[current].pid, SIGSTOP);
                usleep(100000);
            } 
            else 
            {
                // set status to done
                parray[current].done = 1;
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
            if (parray[next].done == 0)
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

        // printf("\tContinuing Process: [%d]\n", pid_array[index]);
        
        current = index;

        kill(parray[current].pid, SIGCONT);

        sleep(1);

        // sleep for one quantum before getting stats
        // sleep(parray[current].quantum);
        // stop process
        // kill(parray[current].pid, SIGSTOP);
        // briefly wait for state to update
        // usleep(100000);
        

        system("clear");
        printf("\n|   %-9s | %-7s | %-15s | %-15s | %-10s | %-13s | %-15s | %-10s | %-10s |\n",
            "Process", "State", "User Time", "Kernel Time", "Priority", "# Threads", "Stack Address", "Type", "Quantum");
        printf(" ------------------------------------------------------------------------------------------------------------------------------------\n");


        for (int i = 0; i < process_count; i++)
        {
            if (parray[i].done == 0)
            {
                // printf("\nProcess [%d]:\n", pid_array[i]);
                // get stats for current process
                int isrunning = (i == current);
                usleep(100000);
                get_stats(&parray[i], isrunning);
                print_stats(&parray[i]);
            }
            else
            {
                printf("--> Process %d has completed; no information to display.\n", parray[i].pid);

            }
        }

        // variable quantum based on process stats
        alarm(parray[current].quantum);
    }



    // waitpid - parent waits for all children to die
    for (int i = 0; i < process_count; i++) {
        waitpid(parray[i].pid, NULL, 0);
        // printf("Process %d finished.\n", pid_array[i]);
        free_command_line(&cmd_array[i]);
    }

    printf("\nAll processes have finished :)\n\n");
    
    return 0;
}