#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include "string_parser.h"


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Invalid use: incorrect number of parameters\n");
        return EXIT_FAILURE;
    }
    // open input file
    FILE *file = fopen(argv[2], "r");

    if (file == NULL) {
        perror("couldn't open file");
        return EXIT_FAILURE;
    }
    int read;
    char *line = NULL;
    size_t length = 0;
    int num_lines = 0;

    // count how many lines are in file
    while ((read = getline(&line, &length, file)) != -1) {
        num_lines += 1;
    }
    // reset file pointer
    fclose(file);

    // reopen file and pull line by line
    file = fopen(argv[2], "r");
    if (file == NULL) {
        perror("couldn't open file");
        return EXIT_FAILURE;
    }
    // for string parsing
    command_line token_buffer;
    pid_t *pid_arr = (pid_t *)malloc(sizeof(pid_t) * num_lines);

    // declare and initialize signal set
    sigset_t sigSet;
    sigemptyset(&sigSet);
    // add signal we're watching for to set
    sigaddset(&sigSet, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigSet, NULL);

    // pull lines one at a time, start up a new process for each one
    int index = 0; // keeps track of where we are in pid_arr
    pid_t cpid = 1;
    while ((read = getline(&line, &length, file)) != -1) {
        // parent thread conditional
        // need conditional bc we don't want race condition and want parents
        // perspective child pid in pid_arr[index]
        if (cpid != 0) {
            // fork processes
            cpid = fork();
            pid_arr[index] = cpid;
            // thread error
            if (cpid < 0) {
                perror("error on opening new thread");
                return EXIT_FAILURE;
            }
        }

        // child thread
        if (pid_arr[index] == 0) {
            // line parsing
            token_buffer = str_filler(line, " ");
    
            // array to hold command and associated args
            // array should be free'd when execpv replaces current image process
            char **command_args = (char **)malloc(sizeof(char *) * (token_buffer.num_token + 1)); // +1 for NULL
            int i;
            for (i = 0; i < token_buffer.num_token; i++) {
                command_args[i] = (char *)malloc(sizeof(char) * 100);
                strcpy(command_args[i], token_buffer.command_list[i]); 
            }
            command_args[i] = NULL;
    
            // wait here until signal recieved
            printf("child waiting ...\n");
            int sig;
            if (sigwait(&sigSet, &sig) != 0)
                fprintf(stderr, "yikes\n");
        
            // code below only runs after sigwait receives signal in sigset
            // use system call execvp to run command and args
            // child should die after this runs, no thread bomb
            // printf("command: %s\n", command_args[0]);
            if (execvp(command_args[0], command_args) < 0) {
                fprintf(stderr, "execvp: problem with %s\n", command_args[0]);
                exit(-1);
            }
            for (int i = 0; i < token_buffer.num_token; i++) {
                free(command_args[i]);
            }
            free(command_args);
            exit(-1); // stopping thread bomb
        }
        index++;
    }
    // need parent process to wait till all child processes have waited
    sleep(1);

    // sending SIGUSR1 signal to every child process to end waiting
    // index is now the total number of child processes
    printf("sending SIGUSR1 to every child process...\n");
    for (int i = 0; i < index; i++) {
        kill(pid_arr[i], SIGUSR1);
    }
    printf("all childs should be executing execvp...\n");

    printf("sending SIGSTOP to all child processes...\n");
    for (int i = 0; i < index; i++) {
        kill(pid_arr[i], SIGSTOP);
    }
    printf("sending SIGCONT to all child processes...\n");
    for (int i = 0; i < index; i++) {
        kill(pid_arr[i], SIGCONT);
    }
    // this makes parent process wait for all child processes to die before
    // it kills itself
    for (int i = 0; i < num_lines; i++) {
        int status;
        // waitpid halts execution until process with PID == pid
        // has exited
        // parent should only call
        waitpid(pid_arr[i], &status, 0);
        // waitpid halts execution until process with PID == pid has exited
    }

    // parent needs to use kill(cpid, SIGUSR1) to send signal to child processes
    free(line);
    fclose(file);
    free(pid_arr);
}

