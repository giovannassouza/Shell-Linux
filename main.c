#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

int exec(int start, int end, char **argv){
	char **cmd;
	
	cmd = &argv[start];
	argv[end] = NULL;

	pid_t child = fork();

	if (child < 0) { 
		perror("fork()");
		return -1;
	}else if(child == 0){
		if(execvp(cmd[0], cmd) != 0){
			perror("execvp()");
			exit(-1);
		}
	}
	
	int status;
	waitpid(child, &status, 0);
	return status;
}

void execBack(int start, int end, char **argv){
	char **cmd;
	
	cmd = &argv[start];
	argv[end] = NULL;
	
	pid_t child = fork();
	
	if (child < 0) { 
		perror("fork()");
		exit(-1);
	}else if(child == 0){
		if(execvp(cmd[0], cmd) != 0){
			perror("execvp()");
			exit(-1);
		}
	}
}

void execPipe(char **argv, int pipeCounter, int *argArray, int *checker){
	int fd[pipeCounter][2];
	
	int comandos = pipeCounter + 1;	

	pid_t child;
	
	int status;
	int i;
	
	for(i = 0; i < comandos; i++){
		if(i < pipeCounter){
			if(pipe(fd[i]) < 0){
				perror("pipe()");
				exit(-1);
			}
		}
		
		child = fork();
		
		if(child < 0){
			perror("fork()");
			exit(-1);
		}else if(child == 0){
			if(i < comandos-1){
				dup2(fd[i][1], STDOUT_FILENO);
				close(fd[i][0]);
				close(fd[i][1]);
			}

			if(i > 0){
				dup2(fd[i-1][0], STDIN_FILENO);
				close(fd[i-1][1]);
				close(fd[i-1][0]);
			}

			char **cmd;
			cmd = &argv[argArray[i]];

			if(execvp(cmd[0], cmd) != 0){
				perror("execvp()");
				exit(-1);
			}
		}else{
			if(i > 0){
				close(fd[i-1][0]);
				close(fd[i-1][1]);
			}
			waitpid(-1, &status, 0);
		}
	}
	
	if(status != 0) *checker = 1;
	else *checker = 0;
}

int pipeLine(int start, int firstPipe, int end, char **argv, int *checker){
	int pipeCounter = 1;
	int argArray[end];
	
	argv[firstPipe] = NULL;
	
	argArray[0] = start;
	argArray[1] = firstPipe + 1;
	
	int i;
	
	for(i = firstPipe + 1; i < end; i++){
		if(strcmp(argv[i], "|") == 0){
			argv[i] = NULL;
			pipeCounter++;
			argArray[pipeCounter] = i + 1;
		}else if(strcmp(argv[i], ";") == 0){
			argv[i] = NULL;
			break;
		}else if( strcmp(argv[i], "||") == 0 || strcmp(argv[i], "&&") == 0 || strcmp(argv[i], "&") == 0) break;
	}
	
	execPipe(argv, pipeCounter, argArray, checker);

	return i;
}

int main(int argc, char **argv) {
	int i;
	int checker = 0;
	int argument = 1;

	if (argc == 1) { 
		printf("Use: %s <command> <arg_1> <arg_2> ... <arg_n>\n", argv[0]);
		return 0;
	}else{
		for(i = 1; i < argc; i++){
            		if(strcmp(argv[i], "&&") == 0){ 
				if(checker == 0){
					checker = exec(argument, i, argv);
				}
				argument = i + 1;
			
			}else if(strcmp(argv[i], "||") == 0){
				if(checker == 0){
					checker = exec(argument, i, argv);
				}
				argument = i + 1;

				if(checker != 0) checker = 0;
				else checker = 1;
			
			}else if(strcmp(argv[i], "&") == 0){
				if(checker == 0){
					execBack(argument, i, argv);
				}
				argument = i + 1;
				
			}else if(strcmp(argv[i], "|") == 0){
				if(checker == 0){
					i = pipeLine(argument, i, argc, argv, &checker);
				}
				argument = i + 1;
			}
		}
	}
	
	if(checker == 0 && i != argc + 1){
		checker = exec(argument, i, argv);
	}
	
	printf("---END---\n");
	return 0;
}