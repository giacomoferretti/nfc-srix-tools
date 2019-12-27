#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define RESET "\033[0m"

bool verbose_status;
void set_verbose(bool);
int lverbose(const char * restrict, ...);
int lerror(const char * restrict, ...);
int lwarning(const char * restrict, ...);

#endif // LOGGING_H