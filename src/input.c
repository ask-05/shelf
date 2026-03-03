#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "input.h"

#define SHELF_TOK_BUFSIZE 64

static int is_delim(char c) {
  return isspace((unsigned char)c) || c == '\a';
}

char *shelf_read_line(void) {
  char *line = NULL;
  size_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

char **shelf_split_line(char *line) {
  int bufsize = SHELF_TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));

  if (!tokens) {
    perror("shelf: memory allocation error\n");
    exit(EXIT_FAILURE);
  }

  char *p = line;
  while (*p) {
    while (*p && is_delim(*p)) { // delimits \t\r\n\v\f\a
      p++;
    }

    if (*p == '\0') {
      break;
    }

    if (position >= bufsize - 1) {
      bufsize += SHELF_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        perror("shelf: memory allocation error: token buffer size could not be reallocated\n");
        exit(EXIT_FAILURE);
      }
    }

    if (*p == '"') {
      p++;
      tokens[position] = p;

      while (*p && *p != '"') {
        p++;
      }
      if (*p == '"') {
        *p = '\0';
        p++;
      }
    } else if (*p == '|') {
      tokens[position] = "|";
      p++;
    } else {
      tokens[position] = p;

      while (*p && !is_delim(*p) && *p != '|') {
        p++;
      }
      if (*p != '\0' && *p != '|') {
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
