#ifndef __LOG_H
#define __LOG_H

#include <cstdio>
using namespace std;

#define LOG_EMERGE  (1)
#define LOG_ALERT   (2)
#define LOG_CRIT    (3)
#define LOG_ERR     (4)
#define LOG_WARNING (5)
#define LOG_NOTICE  (6)
#define LOG_INFO    (7)
#define LOG_DEBUG   (8)

// Only print logs <= LOG_LEVEL
#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_DEBUG
#endif

static inline void log(int level, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

#endif
