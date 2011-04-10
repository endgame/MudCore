#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "options.h"

int main(int argc, char* argv[]) {
  options_init(argc - 1, argv + 1);
  options_deinit();
  return 0;
}
