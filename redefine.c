#include "redefine.h"

void *MALLOC(size_t n) {
  void *p = malloc(n);
  if (!p) {
    fprintf(stderr, "\033[31mMALLOC\033[0m %s:%d\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  return p;
}
