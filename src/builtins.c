#include <stdio.h>
#include <unistd.h>

#include "builtins.h"

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
  (void)args;
  printf("Welcome to shelf! This project is currently being worked on, and \ndoesn't really have any features at all. In fact, there is no \nreason for you to be using this right now :)\n");
  printf("Right now, I have the following built in functions: \n");
  for (int i = 0; i <= num_builtin_funcs; i++) {
    printf("%d. %s\n", i + 1, shelf_builtin[i]);
  }
  return 1;
}

int shelf_exit(char **args) {
  (void)args;
  return 0;
}

int (*shelf_builtin_funcs[]) (char **) = {
  &shelf_cd,
  &shelf_help,
  &shelf_exit
};

int num_builtin_funcs = sizeof(shelf_builtin) / sizeof(shelf_builtin[0]) - 1;
