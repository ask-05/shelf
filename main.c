#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <ctype.h>

#define SHELF_TOK_BUFSIZE 64
#define SHELF_TOK_DELIM " \t\r\n\a"

int num_builtin_funcs;
pid_t child_pid = -1;  // track the currently running child process

int is_delim(char c) {
  return isspace((unsigned char)c) || c == '\a';
}

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

  char *p = line;
  while (*p) {
    while (*p && is_delim(*p)) p++; // delimits \t\r\n\v\f\a
    
    if (*p == '\0') break;
    
    if(*p == '\"') {
      p++;
      tokens[position] = p;
      
      while (*p && *p != '\"') p++;
      if (*p == '\"') {
        *p = '\0';
        p++;
      }
    } else {
      tokens[position] = p;
      
      while (*p && !is_delim(*p)) p++;
      if(*p != '\0') {
        *p = '\0';
        p++;
      }
    }

    if (position >= bufsize) {
      bufsize += SHELF_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if(!tokens) {
        perror("shelf: memory allocation error: token buffer size could not be reallocated\n");
        exit(EXIT_FAILURE);
      }
    }
    position++;
    // token = strtok(NULL, SHELF_TOK_DELIM); // NULL is parameter so strtok moves on to next string
  }
  tokens[position] = NULL;
  return tokens;
}

int shelf_launch(char **args) {
  int status;
  pid_t pid = fork();
  child_pid = pid;
  pid_t wpid;

  if(pid < 0) {
    perror("shelf: could not duplicate shelf; could not run fork()");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    signal(SIGINT, SIG_DFL); // resets signal handlers in child so it gets default SIGINT behavior
    execvp(args[0], args);
    perror("shelf: ");
    exit(EXIT_FAILURE); // only runs if execvp fails
  } else {
    // child_pid = pid;  // track the child process
    wpid = waitpid(pid, &status, WUNTRACED);
    child_pid = -1;  // clear the child PID when done
  }
  return 1;
}

char *shelf_builtin[] = {
  "cd",
  "help",
  "exit",
};

int shelf_cd(char **args) {
  if (args[0] == NULL) {
    perror("shelf: expected argument for command cd");
  } else {
    if (chdir(args[0]) != 0) {
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

static void handler(int sig) {
  // If a child process is running, send SIGINT to it
  if (child_pid > 0) {
    kill(child_pid, SIGINT);
    write(STDOUT_FILENO, "\n", 1);
  } else {
    write(STDOUT_FILENO, "\b\b\033[J", 6); // moves cursor two steps back and removes everything after cursor
  }
}

int main(int argc, char **argv) {
  struct sigaction sa;

  // setup SIGINT handler
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask); // removes leftover garbage data from sa_mask
  sa.sa_flags = SA_RESTART;

  if(sigaction(SIGINT, &sa, NULL) == -1) {
    perror("Couldn't handle CTRL+C");
  }

  shelf_loop(); // main loop
  return 0;
}