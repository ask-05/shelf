#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "shell.h"
#include "signals.h"

static void handler(int sig) {
  (void)sig;

  if (child_pid > 0) {
    kill(child_pid, SIGINT);
    write(STDOUT_FILENO, "\n", 1);
  } else {
    write(STDOUT_FILENO, "\b\b\033[J", 6); // moves cursor two steps back and removes everything after cursor
  }
}

void setup_signal_handler(void) {
  struct sigaction sa;

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask); // removes leftover garbage data from sa_mask
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("Couldn't handle CTRL+C");
  }
}
