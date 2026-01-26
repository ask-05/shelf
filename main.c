#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#define SHELF_TOK_BUFSIZE 64
#define SHELF_TOK_DELIM " \t\r\n\a"

int num_builtin_funcs;

char *shelf_read_line(void) {
  char *line = NULL;
  ssize_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1){
    if (feof(stdin)) {
      exit(EXIT_SUCCESS); 
    } else  {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

char **shelf_split_line(char *line) {
  int bufsize = SHELF_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if(!tokens) {
    perror("shelf: memory allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, SHELF_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += SHELF_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if(!tokens) {
        perror("shelf: memory allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, SHELF_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int shelf_launch(char **args) {
  int status;
  pid_t pid = fork();
  pid_t wpid;

  if(pid < 0) {
    perror("shelf: could not duplicate shelf; could not run fork()");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    execvp(args[0], args);
    perror("shelf: ");
    exit(EXIT_FAILURE); // only runs if execvp fails
  } else {
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while(!WIFEXITED(status) && !WIFSTOPPED(status));
  }
  return 1;
}

char *shelf_builtin[] = {
  "cd",
  "help",
  "exit",
};

int shelf_cd(char **args) {
  if (args[1] == NULL) {
    perror("shelf: expected argument for command cd");
  } else {
    if (chdir(args[1]) != 0) {
      perror("shelf");
    }
  }
  return 1;
}

int shelf_help(char **args) {
  printf("Welcome to shelf! This project is currently being worked on, and \ndoesn't really have any features at all. In fact, there is no \nreason for you to be using this right now :)\n");
  printf("Right now, I have the following built in functions: \n");
  for(int i = 0; i <= num_builtin_funcs; i++) {
    printf("%d. %s\n", i+1, shelf_builtin[i]);
  }
  return 1;
}

int shelf_exit(char **args) {
  return 0;
}

int (*shelf_builtin_funcs[]) (char **) = {
  &shelf_cd,
  &shelf_help,
  &shelf_exit
};

int num_builtin_funcs = sizeof(shelf_builtin)/sizeof(shelf_builtin[0]) -1;

int shelf_execute(char **args) {
  if(args[0] == NULL) {
    perror("shelf: empty command");
    return 1;
  }

  for (int i = 0; i <= num_builtin_funcs; i++) {
    if (strcmp(args[0], shelf_builtin[i]) == 0) {
      return shelf_builtin_funcs[i] (args);
    }
  }

  return shelf_launch(args);
}

void shelf_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        
        line = shelf_read_line();
        args = shelf_split_line(line);
        status = shelf_execute(args);

        free(line);
        free(args);

    } while (status);
}

int main(int argc, char **argv) {
  shelf_loop();
  return 0;
}