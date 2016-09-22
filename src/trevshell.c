/*
 * trevshell.c
 *
 *      Author: Trevor Buttrey
 */

//#include "Trevshell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
const int DEBUG = 0;
pid_t pid = 0;
pid_t pid1 = 0;
pid_t tpid2 = 0;
pid_t tpid = 0;
pid_t bgpid = 0;
int BACKGROUNDSIG;

void signal_terminate_handler(int signum)
{
	signal(SIGINT, signal_terminate_handler);
	if(DEBUG)printf("\nCaught signal %d\n",signum);
	if(DEBUG)printf("pid=%d\n",pid);
	if(DEBUG)printf("bgpid=%d\n",bgpid);
	if (pid != 0) {
		//signal(SIGINT, SIG_DFL);
		kill(pid,SIGINT);
		printf("\nProgram Terminated\n");
	}
	else{
		printf("\nTrevshell>");
	}
	fflush(stdout);
	fflush(stdin);
	//raise(SIGINT);
	//if (signum == 20){
	//	BACKGROUNDSIG = 1;
	//}
	//else if (signum == 2 && pid !=0){
	//	kill(pid, SIGSTOP);
	//}
	//exit(signum);
}
void signal_interupt_handler(int signum){
	signal(SIGTSTP, signal_interupt_handler);
	if(DEBUG)printf("Caught signal %d\n",signum);
	if(DEBUG)printf("\npid=%d\n",pid);
	if(DEBUG)printf("bgpid=%d\n",bgpid);
	//printf("Trevshell>");
	fflush(stdout);
	fflush(stdin);

	if (pid != 0) {
		//signal(SIGTSTP, SIG_DFL);
		kill(pid,SIGTSTP);
		bgpid = pid;
		pid = 0;
		printf("\nProgram Backgrounded\n");
	}
}

/*void handle_signal(int signo, sighandler_t handler){
	struct sigaction new_action, old_action;

	new_action.sa_handler = handler;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;

	sigaction(signo, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction(signo, &new_action, NULL);

}*/

void redirectIO(char *file_out, char *file_in){
	FILE *output;
	FILE *input;
	if (file_out != NULL){
		if(DEBUG)printf("Redirecting output to %s\n", file_out);
		output = fopen(file_out, "w");
		dup2(fileno(output), PIPE_WRITE);
		fclose(output);
	}

	if (file_in != NULL) {
		if(DEBUG)printf("Redirecting input from %s\n", file_in);
		input = fopen(file_in, "r");
		dup2(fileno(input), PIPE_READ);
		fclose(input);
	}
}

int executePipe(char** args1, char *file_in, char** args2, char *file_out, int background) {
	int fds[2];
	pid_t pid1;
	pid_t pid2;
	int child_return_status;

	pipe(fds);
	fflush(stdout);
	fflush(stdin);
	pid1 = fork();//fork process 1
	if(pid1 == 0) {
		//close(STDIN_FILENO);
		redirectIO(NULL, file_in); //attach input file (if exists)
		dup2(fds[PIPE_WRITE], STDOUT_FILENO); //attach output pipe
		close(fds[PIPE_READ]);
		//close(fds[PIPE_WRITE]);
		execvp(args1[0], args1);
		printf("Unknown Command\n");
		exit(1);
	}
	pid2 = fork();//fork process 2
	if(pid2 == 0) {
		//close(STDOUT_FILENO);
		redirectIO(file_out, NULL); //attach output file (if exists)
		dup2(fds[PIPE_READ], STDIN_FILENO); //attach input pipe
		close(fds[PIPE_WRITE]);
		//close(fds[PIPE_READ]);
		//close(fds[PIPE_READ]);
		//wait(NULL);
		execvp(args2[0], args2);
		printf("Unknown Command\n");
		exit(1);
	}
	close(fds[PIPE_WRITE]);
	close(fds[PIPE_READ]);
	if (!background){
		pid = pid1;
		do {
			tpid = waitpid(pid1, &child_return_status, WUNTRACED);
			if (WIFSTOPPED(child_return_status))
			{
				if(DEBUG)printf("child stopped\n");
			}
		} while (!WIFEXITED(child_return_status) && !WIFSIGNALED(child_return_status)&&!WIFSTOPPED(child_return_status));
		pid1 = 0;
		pid = pid2;
		do {
			tpid = waitpid(pid2, &child_return_status, WUNTRACED);
			if (WIFSTOPPED(child_return_status))
			{
				if(DEBUG)printf("child stopped\n");
			}
		} while (!WIFEXITED(child_return_status) && !WIFSIGNALED(child_return_status)&&!WIFSTOPPED(child_return_status));
		pid2 = 0;
		pid = 0;
		//while (wait(&child_return_status) != pid2)
			//return child_return_status;
	}
	return child_return_status;
}

int execute(int argc, char** args, char *file_out, char *file_in, int background){
	int child_return_status;
	if (argc > 0){
		if (strcmp (args[0], "exit") == 0){
			exit(0);
		}
		else if(strcmp (args[0], "resume") == 0){
			if (bgpid != 0){
				printf("Resuming Program...\n");
				pid = bgpid;
				kill(bgpid,SIGCONT);
				bgpid = 0;
				do {
					tpid = waitpid(pid, &child_return_status, WUNTRACED);
					if (WIFSTOPPED(child_return_status))
					{
						if(DEBUG)printf("child stopped\n");
					}
				} while (!WIFEXITED(child_return_status) && !WIFSIGNALED(child_return_status)&&!WIFSTOPPED(child_return_status));
				pid = 0;
				return child_return_status;
			}
			else{
				printf("No Backgrounded Program\n");
				return 0;
			}
		}
		else{
			if(DEBUG)printf("running: %s\n",args[0]);
			pid = fork();
			if(pid == 0) {
				//Child
				redirectIO(file_out, file_in);
				execvp(args[0], args);
				printf("Unknown Command\n");
				exit(1);
			}
			else {
				if (!background){
					do {
						tpid = waitpid(pid, &child_return_status, WUNTRACED);
						if (WIFSTOPPED(child_return_status))
						{
							if(DEBUG)printf("child stopped\n");
						}
					} while (!WIFEXITED(child_return_status) && !WIFSIGNALED(child_return_status)&&!WIFSTOPPED(child_return_status));
				}
				pid = 0;
				return child_return_status;
				//Parent
				/*do {
					pid_t tpid = wait(&child_return_status);
					//if(tpid != pid) process_terminated(tpid);
				} while(tpid != pid);*/
				/*if(!background){
					while (wait(&child_return_status) != pid && !BACKGROUNDSIG);
				}
				else{
					child_return_status = 0;
				}
				return child_return_status;
				 */
			}
		}
	}
	else{
		return 0;
	}
}

int main(int argc, char** argv){
	signal(SIGINT, signal_terminate_handler);
	signal(SIGTSTP, signal_interupt_handler);
	BACKGROUNDSIG = 0;
	int BUFFER = 1024;
	int i = 0;
	char* args[25];
	char* p;
	char* line = (char *) malloc(sizeof(char)*BUFFER);
	int *pipeloc;
	int *outputloc;
	int *inputloc;
	int *amploc;
	pid = 0;

	while(1){
		int returncode = 0;
		printf("Trevshell>");
		fflush(stdout);
		p = fgets(line, BUFFER, stdin);
		fflush(stdin);
		i = 0;
		if(DEBUG)printf("line: %s",line);
		//fflush(stdout);
		if(strcmp (line, "\n") != 0){
			args[i++] = strtok(line, " \n");
			while ((args[i++] = strtok(NULL, " \n")) != NULL);
			int j;
			int pipecount = 0;
			int inputcount = 0;
			int outputcount = 0;
			int ampcount = 0;
		//if(args != NULL){
			if(DEBUG)printf("number of args is %d\n",i);
			for (j = 0; j < i; j++){
				if (args[j] != NULL){
					if (strcmp (args[j], "|") == 0){
						if(DEBUG)printf("found |\n");
						pipecount++;
					}
					else if (strcmp (args[j], "<") == 0){
						if(DEBUG)printf("found <\n");
						inputcount++;
					}
					else if (strcmp (args[j], ">") == 0){
						if(DEBUG)printf("found <\n");
						outputcount++;
					}
					else if (strcmp (args[j], "&") == 0){
						if(DEBUG)printf("found &\n");
						ampcount++;
					}
				}
				else{
					//printf("EOL\n");
				}
			}
			pipeloc=(int *) malloc((pipecount + 1)*sizeof(int));
			inputloc=(int *) malloc(inputcount*sizeof(int));
			outputloc=(int *) malloc(outputcount*sizeof(int));
			amploc=(int *) malloc(ampcount*sizeof(int));
			pipecount = 0;
			inputcount = 0;
			outputcount = 0;
			for (j = 0; j < i; j++){
				if (args[j] != NULL){
					if (strcmp (args[j], "|") == 0){
						if(DEBUG)printf("changing | to NULL\n");
						pipeloc[pipecount] = j;
						args[j] = NULL;
						pipecount++;
					}
					else if (strcmp (args[j], "<") == 0){
						if(DEBUG)printf("Changing < to NULL\n");
						inputloc[inputcount] = j;
						args[j] = NULL;
						inputcount++;
					}
					else if (strcmp (args[j], ">") == 0){
						if(DEBUG)printf("Changing < to NULL\n");
						outputloc[outputcount] = j;
						args[j] = NULL;
						outputcount++;
					}
					else if (strcmp (args[j], "&") == 0){
						if(DEBUG)printf("Changing & to NULL\n");
						amploc[ampcount] = j;
						args[j] = NULL;
						ampcount++;
					}
				}
				else {
					pipeloc[pipecount] = j;
				}
			}
			int start = 0;
			int k;
			int l;
			char* newargs[25];
			char* inputfile = NULL;
			char* outputfile = NULL;
			if (pipecount == 0){ //to simplify code check if no pipe
				for (k = 0; k < inputcount; k++){ //for each <
					if(DEBUG)printf("redirecting input\n");
					inputfile = args[inputloc[k]+1];
				}
				for (k = 0; k < outputcount; k++){ //for each >
					if(DEBUG)printf("redirecting output\n");
					outputfile = args[outputloc[k]+1];
				}
				returncode = execute(i, args, outputfile, inputfile, ampcount);
				if(returncode != 0){
					printf("Program Returned with Code: %d\n", returncode);
				}
			}
			else if (pipecount == 1){ //check if only one pipe
				l=0;
				char* newargs1[25];
				char* newargs2[25];
				for (k = 0; k < pipeloc[0]; k++){ //get args before |
					newargs1[l++] = args[k];
					if(DEBUG)printf("arg1: %s\n", args[k]);
				}
				l=0;
				for (k = pipeloc[0]+1; k < pipeloc[1]; k++){ //get args after |
					newargs2[l++] = args[k];
					if(DEBUG)printf("arg2: %s\n", args[k]);
				}
				for (k = 0; k < inputcount; k++){ //for each <
					if(DEBUG)printf("redirecting input\n");
					inputfile = args[inputloc[k]+1];
				}
				for (k = 0; k < outputcount; k++){ //for each >
					if(DEBUG)printf("redirecting output\n");
					outputfile = args[outputloc[k]+1];
				}
				returncode = executePipe(newargs1, inputfile, newargs2, outputfile, ampcount);
				if(returncode != 0){
					printf("Program Returned with Code: %d\n", returncode);
				}
			}
			else{
				printf("Multiple pipes are not supported at this time\n");
			}
			/*for (j = 0; j < pipecount + 1; j++){ //for each command
			//printf("A command\n");
			l = 0;
			for (k = start; k < pipeloc[j]; k++){ //for each argument
				newargs[l++] = args[k];
				printf("arg: %s\n", args[k]);
			}
			//printf("running\n");
			execute(pipeloc[j]-start+1, newargs);
			start = pipeloc[j]+1;
		}
			 */
		}
	}
}
