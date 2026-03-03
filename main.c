#include "shell.h"
#include "signals.h"

int main(void) {
  setup_signal_handler();
  shelf_loop();
  return 0;
}