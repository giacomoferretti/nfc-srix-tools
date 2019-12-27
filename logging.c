#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint-gcc.h>
#include "logging.h"

bool verbose_status = false;

void set_verbose(bool setting) {
    verbose_status = setting;
}

int lverbose(const char * restrict format, ...) {
    if (!verbose_status) {
        return 0;
    }

    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}

int lerror(const char * restrict format, ...) {
    printf(RED);
    printf("ERROR: ");
    printf(RESET);
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}

int lwarning(const char * restrict format, ...) {
    printf(YELLOW);
    printf("WARNING: ");
    printf(RESET);
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}