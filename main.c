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

int num_builtin_funcs;
pid_t child_pid = -1;  // track the currently running child process
int is_piping = 0;
char* piped_line;

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

  if(!tokens) {
    perror("shelf: memory allocation error\n");
    exit(EXIT_FAILURE);
  }

  char *p = line;
  while (*p) {
    while (*p && is_delim(*p)) p++; // delimits \t\r\n\v\f\a
    
    if (*p == '\0') break;

    if (position >= bufsize - 1) {
      bufsize += SHELF_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if(!tokens) {
        perror("shelf: memory allocation error: token buffer size could not be reallocated\n");
        exit(EXIT_FAILURE);
      }
    }
    
    if(*p == '\"') {
      p++;
      tokens[position] = p;
      
      while (*p && *p != '\"') p++;
      if (*p == '\"') {
        *p = '\0';
        p++;
      }
    } else if(*p == '|') {
      tokens[position] = "|";
      p++;
    } else {
      tokens[position] = p;

      while (*p && !is_delim(*p) && *p != '|') p++;
      if(*p != '\0' && *p != '|') {
        *p = '\0';
        p++;
      } else if (*p == '|') {
        *p = '\0';
      }
    }
    position++;
  }
  tokens[position] = NULL;
  return tokens;
}

int shelf_launch(char **args) {
  int status;
  pid_t pid = fork();

  if(pid < 0) {
    perror("shelf: could not duplicate shelf; could not run fork()");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    signal(SIGINT, SIG_DFL); // resets signal handlers in child so it gets default SIGINT behavior
    execvp(args[0], args);
    perror("shelf: ");
    exit(EXIT_FAILURE); // only runs if execvp fails
  } else {
    child_pid = pid;
    waitpid(pid, &status, WUNTRACED);
    child_pid = -1;  // clear the child PID when done
  }
  return 1;
}

int shelf_launch_pipeline(char ***cmds, int ncmds) {
  int prev_read = -1;
  pid_t *pids = malloc(sizeof(pid_t) * ncmds);

  if (!pids) {
    perror("shelf: memory allocation error");
    return 1;
  }

  for (int i = 0; i < ncmds; i++) {
    int fd[2] = {-1, -1};

    if (i < ncmds - 1 && pipe(fd) == -1) {
      perror("shelf: could not create pipe");
      free(pids);
      return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
      perror("shelf: could not duplicate shelf; could not run fork()");
      if (fd[0] != -1) close(fd[0]); // closing the pipe
      if (fd[1] != -1) close(fd[1]);
      free(pids);
      return 1;
    }

    if (pid == 0) {
      signal(SIGINT, SIG_DFL); // restores default ctrl+c behavior

      if (prev_read != -1) {
        dup2(prev_read, STDIN_FILENO); // if not first command, connects previous pipe to stdin
      }

      if (i < ncmds - 1) {
        dup2(fd[1], STDOUT_FILENO); // if not last command, connects current pipe to stdout
      }

      // closing unused fds
      if (prev_read != -1) close(prev_read);
      if (fd[0] != -1) close(fd[0]);
      if (fd[1] != -1) close(fd[1]);

      execvp(cmds[i][0], cmds[i]); // executes command
      perror("shelf: "); // reaches here if execvp fails
      _exit(EXIT_FAILURE);
    }

    pids[i] = pid;
    child_pid = pid;

    // closing previous fds and setting up for next pipe
    if (prev_read != -1) close(prev_read);
    if (fd[1] != -1) close(fd[1]);
    prev_read = fd[0];
  }

  if (prev_read != -1) close(prev_read);

  for (int i = 0; i < ncmds; i++) {
    int status;
    waitpid(pids[i], &status, WUNTRACED);
  }

  child_pid = -1;
  free(pids);
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

  int pipe_count = 0;
  for (int i = 0; args[i] != NULL; i++) {
    if (strcmp(args[i], "|") == 0) { // looking for pipes and counting each one
      pipe_count++;
    }
  }

  if (pipe_count > 0) {
    int ncmds = pipe_count + 1;
    char ***cmds = malloc(sizeof(char **) * ncmds); // array where each element is a pointer to an array of strings (pointer to a pointer to a pointer)

    if (!cmds) {
      perror("shelf: memory allocation error");
      return 1;
    }

    int cmd_index = 0;
    cmds[cmd_index++] = args; // first pointer to the start of args, increments cmd_index after

    for (int i = 0; args[i] != NULL; i++) {
      if (strcmp(args[i], "|") == 0) {
        if (i == 0 || args[i + 1] == NULL || strcmp(args[i + 1], "|") == 0) {
          perror("shelf: invalid pipe syntax");
          free(cmds);
          return 1;
        }

        args[i] = NULL;
        cmds[cmd_index++] = &args[i + 1]; // setting next array element to memory address after '|'
      }
    }

    int launch_status = shelf_launch_pipeline(cmds, ncmds);
    free(cmds);
    return launch_status;
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