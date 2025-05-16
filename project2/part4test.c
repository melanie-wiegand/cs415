#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include "string_parser.h"

void context_switch_alarm() {
    printf("Context switch occured in parent process between alarm and sigwait\n");
    alarm(1);
}


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
    int sig;
    sigset_t sigSet;
    sigemptyset(&sigSet);
    // add signal we're watching for to set
    sigaddset(&sigSet, SIGCONT);
    sigaddset(&sigSet, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigSet, NULL);
    // if context switch occurs
    signal(SIGALRM, context_switch_alarm);

    // pull lines one at a time, start up a new process for each one
    int num_childs = 0; // keeps track of where we are in pid_arr
    pid_t cpid = 1;
    while ((read = getline(&line, &length, file)) != -1) {
        // parent thread conditional
        // need conditional bc we don't want race condition and want parents
        // perspective child pid in pid_arr[num_childs]
        if (cpid != 0) {
            // fork processes
            cpid = fork();
            pid_arr[num_childs] = cpid;
            // thread error
            if (cpid < 0) {
                perror("error on opening new thread");
                return EXIT_FAILURE;
            }
        }

        // child thread
        if (pid_arr[num_childs] == 0) {
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
            // printf("child waiting ...\n");
            if (sigwait(&sigSet, &sig) != 0)
                fprintf(stderr, "error in sigwait\n");
        
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
        num_childs++;
    }
    // need parent process to wait till all child processes have waited
    // printf("Parent waiting for children to all be waiting\n");
    alarm(1);
    sigwait(&sigSet, &sig);

    // printing format
    char print_table[num_childs+1][2048];
    strcpy(print_table[0], "cpid\tstate\tsize\tuser\tkernel\n");
    char initial_table[1024] = "0\t0\t0\t0\t0\n";
    for (int i = 0; i < num_childs; i++) {
        strcpy(print_table[i+1], initial_table);
    }

    // round robin
    printf("Round robin beginning on child processes\n");
    int process_i = 0;
    int finished_processes = 0;
    int status;
    while (1) {
        // when all childs have exited, break while loop
        // at the beggining of each cycle, count how many processes
        // were finished in the previous cycle
        if (process_i % num_childs == 0) {
            if (finished_processes == num_childs) { 
                // printf("All child processes have been completed\n");
                break;
            }
            // reset finished_processes at the start of each new cycle
            finished_processes = 0;
        }

        // if process is completed, go to next process
        if (waitpid(pid_arr[process_i], &status, WNOHANG) != 0) {
            if (WIFEXITED(status)) {
                strcpy(print_table[process_i+1], "");
                process_i = (process_i + 1) % num_childs;
                finished_processes += 1;
                // printf("cpid finished: %d\n", pid_arr[process_i]);
                continue;
            }
        }

        // alarm and run process_i
        // printf("child process: %d time slice\n", pid_arr[process_i]);
        kill(pid_arr[process_i], SIGCONT);
        
        alarm(1);
        if (sigwait(&sigSet, &sig) != 0) {
            fprintf(stderr, "error with sigwait\n");
        };

        // send SIGSTOP to each array
        // SIGSTOP can't be caught by process
        // immeidately stop the execution until SIGCONT arrives
        kill(pid_arr[process_i], SIGSTOP);

        // building proc path for each child process
        char stat_path[256] = "/proc/";
        cpid = pid_arr[process_i];
        char cpid_str[40];
        sprintf(cpid_str, "%d", cpid);
        strcat(stat_path, cpid_str);
        strcat(stat_path, "/stat");

        // open cpid stat file and read into buffer
        FILE *cpid_stat;
        if ((cpid_stat = fopen(stat_path, "r")) < 0) {
            fprintf(stderr, "failed to open state path\n");
        }
        char *buffer = NULL;
        if (getline(&buffer, &length, cpid_stat) < 0) {
            fprintf(stderr, "Reading from stat failed\n");
        }

        // string parse cpid stats
        // 52 elements !
        command_line cpid_token_buff;
        cpid_token_buff = str_filler(buffer, " ");

        // updating process_i stat info in table
        char process_stat_info[2048] = "";
        strcat(process_stat_info, cpid_token_buff.command_list[0]);
        strcat(process_stat_info, "\t");
        strcat(process_stat_info, cpid_token_buff.command_list[2]);
        strcat(process_stat_info, "\t");
        strcat(process_stat_info, cpid_token_buff.command_list[23]);
        strcat(process_stat_info, "\t");
        strcat(process_stat_info, cpid_token_buff.command_list[13]);
        strcat(process_stat_info, "\t");
        strcat(process_stat_info, cpid_token_buff.command_list[14]);
        strcat(process_stat_info, "\n");

        strcpy(print_table[process_i+1], process_stat_info);

        system("clear");
        for (int i = 0; i < num_childs + 1; i++) {
            printf("%s", print_table[i]);
            if (i == 0) {
                printf("\n");
            }
        }

        fclose(cpid_stat);
        free_command_line(&cpid_token_buff);
        free(buffer);

        // increment to next process
        process_i = (process_i + 1) % num_childs;
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
    system("clear");
    printf("%s", print_table[0]);

    // parent needs to use kill(cpid, SIGUSR1) to send signal to child processes
    free(line);
    fclose(file);
    free(pid_arr);
}


/*
for each processin round robin
    path <- /proc/<pid>/stat
    file <- open(path)
    read file into buffer
    parsed <- string_pasre on buffer
    print parsed[i], parsed[k] ....

*/