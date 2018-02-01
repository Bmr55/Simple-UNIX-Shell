#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define SIZE 512

void my_exit(int);
void my_pwd();
void my_cd(char*);
void my_pushd(char *);
void my_popd();
void print_directory_stack();
void run_regular_program(char**, int);
void run_regular_program_with_redir(char**, int, int, int, int, int, char, char);

char* stack[4];
int stack_pointer = 0;

int main(){

	int line_num = 1;
	char *token_holder[SIZE];
	int token_pos = 0;
	char buffer[SIZE];
	char *token;
	char *delim = " \t\n()|&;";

	while(1) {

		token_pos = 0;
		int redir_bool = 0;

		int num_of_out_redir = 0;
	  int num_of_in_redir = 0;
	  int redir_out_symb_pos = -2;
	  int redir_in_symb_pos = -2;
	  char output_symb = ' ';
	  char input_symb = ' ';

		printf("(%d) myshell $ ", line_num);
		line_num++;

		fgets(buffer, SIZE, stdin);

		if(buffer[0] == '\n') {
			continue;
		}

		char *copy = strdup(buffer);
		token_holder[token_pos] = token = strtok(copy, delim);
		token_pos++;

		while(token != NULL) {

			printf(" %s\n", token);

			if(!strcmp(token, "<")) {

				redir_bool = 1;
	      input_symb = token[0];
	      num_of_in_redir++;
	      redir_in_symb_pos = token_pos-1;
	      printf("Input Redirection Symbol: %c\n", input_symb);
	      printf("Input Redirection Symbol Pos: %d\n", redir_in_symb_pos);

	    }

	    if(!strcmp(token, ">")) {
	      redir_bool = 1;
	      output_symb = token[0];
	      num_of_out_redir++;
	      redir_out_symb_pos = token_pos-1;
	      printf("Redirection Symbol: %c\n", output_symb);
	      printf("Redirection Symbol Pos: %d\n", redir_out_symb_pos);
	    }

			token_holder[token_pos] = token = strtok(NULL, delim);
			token_pos++;
		}

		if(redir_bool) {
			printf("redir_bool is true\n");
		}

		printf("String in token_holder[0] : %s\n", token_holder[0]);

		if(!strcmp(token_holder[0], "exit\0")) {
			printf("exit command called\n");
			if(token_holder[1] != NULL){
				my_exit(atoi(token_holder[1]));
			}
			my_exit(0);
		} else if(!strcmp(token_holder[0], "pwd\0")) {
			printf("pwd command called\n");
			my_pwd();
		} else if(!strcmp(token_holder[0], "cd\0")) {
			printf("cd command called\n");
			if(token_holder[1] != NULL) {
				my_cd(token_holder[1]);
			}
		} else if(!strcmp(token_holder[0], "pushd\0")) {
			printf("pushd command called\n");
			my_pushd(token_holder[1]);
		} else if(!strcmp(token_holder[0], "popd\0")) {
			printf("popd command called\n");
			my_popd();
		} else if(!strcmp(token_holder[0], "dirs\0")) {
			print_directory_stack();
		} else {
			printf("running regular program\n");

			if(redir_bool) {
				run_regular_program_with_redir(token_holder, token_pos, num_of_out_redir,
					 num_of_in_redir, redir_in_symb_pos, redir_out_symb_pos, input_symb, output_symb);
			} else {
				run_regular_program(token_holder, token_pos);
			}
		}
	}
	return 0;
}

void run_regular_program_with_redir(char** token_holder, int token_pos,
			int num_of_out_redir, int num_of_in_redir, int redir_in_symb_pos,
			int redir_out_symb_pos, char input_symb, char output_symb) {

	if(num_of_out_redir > 1) {
    printf("Error: Attempt to redirect output more than once\n");
    return;
  }

  if(num_of_in_redir > 1) {
    printf("Error: Attempt to redirect input more than once\n");
    return;
  }


  char* input_file;
  char* output_file;
  int i, j;
  char* argv[token_pos-2];
  j = 0;
  for(i = 0; i < token_pos; i++) {

    if(i == redir_in_symb_pos || i == redir_in_symb_pos+1) {
      input_file = token_holder[redir_in_symb_pos+1];
      continue;
    }

    if(i == redir_out_symb_pos || i == redir_out_symb_pos+1) {
      output_file = token_holder[redir_out_symb_pos+1];
      continue;
    }
    argv[j] = token_holder[i];
    printf("String in argv[%d] : %s\n", j,  argv[j]);
    j++;
  }

  if(input_symb == '<' && output_symb == '>') {
    printf("Input File: %s\n", input_file);
    printf("Output File: %s\n", output_file);

    pid_t pid = fork();
    int in_fd, out_fd;

    in_fd = open(input_file, O_RDONLY);
    out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if(pid == 0) {

      if(in_fd < 0) {
        perror("Input File Error");
        return;
      }

      if(out_fd < 0) {
        perror("Output File Error");
        return;
      }

      if(dup2(in_fd, STDIN_FILENO) < 0) {
        perror("input dup2() error");
        return;
      }

      if(close(in_fd) < 0) {
        perror("input close() error");
        return;
      }

      if(dup2(out_fd, STDOUT_FILENO) < 0) {
        perror("output dup2() error");
        return;
      }

      if(close(out_fd) < 0) {
        perror("output close() error");
        return;
      }

      execvp(argv[0], argv);
      exit(127);

    } else {
      waitpid(pid, 0, 0);
    }

    return;
  }

  if(input_symb == '<') {

    printf("Input File: %s\n", input_file);

    pid_t pid = fork();

    int fd = open(input_file, O_RDONLY);

    if(pid == 0) {

      if(fd < 0) {
        perror("open() error");
        return;
      }

      if(dup2(fd, STDIN_FILENO) < 0) {
        perror("dup2() error");
        return;
      }

      if(close(fd) < 0) {
        perror("close() error");
        return;
      }

      execvp(argv[0], argv);
      exit(127);

    } else {
      waitpid(pid, 0, 0);
    }
    return;

  }

  if(output_symb == '>') {

    printf("Output File: %s\n", output_file);

    pid_t pid = fork();

    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if(pid == 0) {

      if(fd < 0) {
        perror("open() error");
        return;
      }

      if(dup2(fd, STDOUT_FILENO) < 0) {
        perror("dup2() error");
        return;
      }

      if(close(fd) < 0) {
        perror("close() error");
        return;
      }

      execvp(argv[0], argv);
      exit(127);

    } else {
      waitpid(pid, 0, 0);
    }
    return;
  }
	return;

}

void run_regular_program(char** token_holder, int token_pos) {

	int i;
  char* argv[token_pos];
  for(i = 0; i < token_pos; i++) {
    argv[i] = token_holder[i];
  }

	pid_t pid = fork();

  if(pid == 0) {

    if(execvp(token_holder[0], argv) < 0) {
			perror("Error");
			exit(127);
		}

  } else {
    waitpid(pid, 0, 0);
  }
}

void my_pushd(char *passed_path) {

	// check if stack is full
	if(stack_pointer > 3) {
		printf("Error: The directory stack is full.\n");
		return;
	}

	char path[SIZE];
	strcpy(path, passed_path);

	// get current working directory
	char cwd[SIZE];
	getcwd(cwd, sizeof(cwd));

	// push cwd onto stack
	char stack_cwd[SIZE];
	strcpy(stack_cwd, cwd);
	stack[stack_pointer] = stack_cwd;
	printf("%s\n", stack[stack_pointer]);
	stack_pointer++;

	// cd to new cwd
	if(passed_path[0] != '/') {
		strcat(cwd, "/");
		strcat(cwd, path);
		if(chdir(cwd) < 0) {
			perror("Error");
			return;
		}
	} else {
		strcat(cwd, passed_path);
		if(chdir(cwd) < 0) {
			perror("Error");
			return;
		}
	}
	printf("%s\n", cwd);
}

void my_popd() {

	// check if stack is empty
	if(stack_pointer < 0) {
		printf("Error: The directory stack is empty.\n");
		return;
	}

	printf("Stack Pointer: %d\n", stack_pointer);

	// pop path off stack

	printf("%s\n", stack[stack_pointer-1]);
	stack_pointer--;

	// cd to path

}

void my_cd(char *passed_path) {

	printf("passed path: %s\n", passed_path);

	if(!strcmp(passed_path, "..\0")) {
		chdir(".");
	}

	char path[SIZE];
	strcpy(path, passed_path);

	char cwd[SIZE];
	getcwd(cwd, sizeof(cwd));
	if(passed_path[0] != '/') {
		strcat(cwd, "/");
		strcat(cwd, path);
		if(chdir(cwd) < 0) {
			perror("Error");
			return;
		}
	} else {
		strcat(cwd, passed_path);
		if(chdir(cwd) < 0) {
			perror("Error");
			return;
		}
	}
	printf("%s\n", cwd);
}

void my_exit(int code) {
	exit(code);
}

void my_pwd() {
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	fprintf(stdout, "%s\n", cwd);
}

void print_directory_stack(){
	printf("Directory Stack:\n");
	int i;
	for(i = stack_pointer-1; i >= 0; i--) {
		printf("%s\n", stack[i]);
	}
}
