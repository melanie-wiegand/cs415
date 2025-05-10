#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

void signaler(pid_t* pid_ary, int size, int signal);

int main(int argc, char* argv[])
{
	

	if (argc < 2){

		printf("Input: ./lab5 <num_processes>\n");
		exit(-1);

	}

	// initialization of pid like lab 4

	int size = atoi(argv[1]);

	pid_t *pid_ary = (pid_t *)malloc(sizeof(pid_t) * size);
	if (pid_ary == NULL) {
        perror("malloc failed");
        exit(-1);
    }

	char *args[] = {"./iobound", "-seconds", "4", 0};

	// initialize sigset
	sigset_t sigset;
	int sig;

	// create an empty sigset_t
	sigemptyset(&sigset);

	// use sigaddset() to add the SIGUSR1 signal to the set
	sigaddset(&sigset, SIGUSR1);

	// use sigprocmask() to add the signal set in the sigset for blocking
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	
	for(int i = 0; i < size; i++)
	{

		pid_ary[i] = fork();

		if(pid_ary[i] == 0)
		{
			// print: Child Process: <pid> - Waiting for SIGUSR1…
			printf("Child Process: %d - Waiting for SIGUSR1...\n", getpid());
			
			// wait for the signal
			sigwait(&sigset, &sig);

			// print: Child Process: <pid> - Received signal: SIGUSR1 - Calling exec().
            printf("Child Process: %d - Received signal: SIGUSR1 - Calling exec().\n", getpid());

			// call execvp with ./iobound like in lab 4
			execvp(args[0], args);

			//if unsuccessful
			perror("execvp failed");
            exit(EXIT_FAILURE);

		}
		else if (pid_ary[i] < 0) {
			// fork failed error
			perror("fork failed");
            free(pid_ary);
			exit(-1);
		}
	}
	
	// send SIGUSR1 
	signaler(pid_ary, size, SIGUSR1);

	// send SIGSTOP 
	signaler(pid_ary, size, SIGSTOP);

	// send SIGCONT
	signaler(pid_ary, size, SIGCONT);

	// send SIGINT
	signaler(pid_ary, size, SIGINT);


	for (int i = 0; i < size; i++) {
        waitpid(pid_ary[i], NULL, 0);
    }

	free(pid_ary);
	
	return 0;
}

void signaler(pid_t* pid_ary, int size, int signal)
{
	// sleep for three seconds
	sleep(3);

	for(int i = 0; i < size; i++)
	{
		// print: Parent process: <pid> - Sending signal: <signal> to child process: <pid>
        printf("Parent process: %d - Sending signal: %s to child process: %d\n", getpid(), strsignal(signal), pid_ary[i]);

		// send the signal
		if (kill(pid_ary[i], signal) == -1) 
		{
            perror("kill failed");
        }

	}
}



