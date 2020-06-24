#ifndef REDEFINE_H
#define REDEFINE_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(expr)                                                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      extern int errno;                                                        \
      fprintf(stderr, "\033[31mASSERT\033[0m %s:%d \033[33m%s\033[0m\n",       \
              __FILE__, __LINE__, strerror(errno));                            \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ELOG(expr)                                                             \
  do {                                                                         \
    fprintf(stderr, "\033[31mELOG\033[0m %s:%d \033[33m%s\033[0m\n", __FILE__, \
            __LINE__, expr);                                                   \
  } while (0)

#define FREE(p)                                                                \
  do {                                                                         \
    free(p);                                                                   \
    p = NULL;                                                                  \
  } while (0)

void *MALLOC(size_t n);

#endif /* REDEFINE_H */