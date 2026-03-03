#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins.h"
#include "exec.h"
#include "shell.h"

int shelf_launch(char **args) {
  int status;
  pid_t pid = fork();

  if (pid < 0) {
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
    child_pid = -1; // clear the chip PID when done
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

      if (prev_read != -1) {
        close(prev_read);
      }
      if (fd[0] != -1) {
        close(fd[0]);
      }
      if (fd[1] != -1) {
        close(fd[1]);
      }

      execvp(cmds[i][0], cmds[i]); // executes command
      perror("shelf: "); // reaches here if execvp fails
      _exit(EXIT_FAILURE);
    }

    pids[i] = pid;
    child_pid = pid;

    if (prev_read != -1) {
      close(prev_read);
    }
    if (fd[1] != -1) {
      close(fd[1]);
    }
    prev_read = fd[0];
  }

  if (prev_read != -1) {
    close(prev_read);
  }

  for (int i = 0; i < ncmds; i++) {
    int status;
    waitpid(pids[i], &status, WUNTRACED);
  }

  child_pid = -1;
  free(pids);
  return 1;
}

int shelf_execute(char **args) {
  if (args[0] == NULL) {
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
      return shelf_builtin_funcs[i](args);
    }
  }

  return shelf_launch(args);
}
