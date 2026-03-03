#include <stdio.h>
#include <stdlib.h>

#include "exec.h"
#include "input.h"
#include "shell.h"

pid_t child_pid = -1;

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
