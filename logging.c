/*
 * Copyright 2019 Giacomo Ferretti
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
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