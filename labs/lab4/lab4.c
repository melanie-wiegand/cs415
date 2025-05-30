#include<stdio.h>
#include <sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

void script_print (pid_t* pid_ary, int size);

int main(int argc,char*argv[])
{
	if (argc != 2)
	{
		printf ("Wrong number of arguments\n");
		exit (0);
	}

	/*
	*	TODO
	*	#1	declare child process pool
	*	#2 	spawn n new processes
	*		first create the argument needed for the processes
	*		for example "./iobound -seconds 10"
	*	#3	call script_print
	*	#4	wait for children processes to finish
	*	#5	free any dynamic memory
	*/

	int n = atoi(argv[1]);
	pid_t pid;
	int* pids = malloc(sizeof(pid_t) * n);

	for (int i = 0; i < n; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			printf("I am the child process. My PID: %d\n", getpid());
			// char* args[] = {"./hi", NULL};
			char* args[] = {"./iobound", "-seconds", "5", NULL};
			execvp(args[0], args);
			perror("execvp failed");
			exit(EXIT_FAILURE);
		}
		else if (pid > 0)
		{
			printf("I am the parent process. The child has PID: %d\n", pid);
			pids[i] = pid;
		}
		else
		{
			perror("fork fail");
			free(pids);
			exit(EXIT_FAILURE);
		}
		
	}

	script_print(pids, n);

	for(int i = 0; i < n; i++)
	{
		waitpid(pids[i], NULL, 0);
	}

	free(pids);
	return 0;
}


void script_print (pid_t* pid_ary, int size)
{
	FILE* fout;
	fout = fopen ("top_script.sh", "w");
	if (!fout) {
        perror("fopen failed");
        return;
    }

	fprintf(fout, "#!/bin/bash\ntop");
	for (int i = 0; i < size; i++)
	{
		fprintf(fout, " -p %d", (int)(pid_ary[i]));
	}
	fprintf(fout, "\n");
	fclose (fout);

	char* top_arg[] = {"gnome-terminal", "--", "bash", "top_script.sh", NULL};
	pid_t top_pid;

	top_pid = fork();

	if (top_pid == 0)
	{
		if(execvp(top_arg[0], top_arg) == -1)
		{
			perror ("top command: ");
		}
		exit(0);
	}
	
}


