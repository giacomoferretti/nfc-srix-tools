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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "logging.h"

#define SRIX4K_EEPROM_SIZE 512
#define SRI512_EEPROM_SIZE 64

static void print_usage(const char *executable) {
    printf("Usage: %s <dump.bin> [-h] [-c 1|2] [-t x4k|512]\n", executable);
    printf("\nNecessary arguments:\n");
    printf("  <dump.bin>   Path to the dump file\n");
    printf("\nOptions:\n");
    printf("  -h           Shows this help message\n");
    printf("  -v           Enables verbose - print debugging data\n");
    printf("  -c 1|2       Print on one or two columns [default: 1]\n");
    printf("  -t x4k|512   Select SRIX4K or SRI512 tag type [default: x4k]\n");
}

#define USAGE "Usage: %s <dump.bin> [-c 1|2] [-t x4k|512]\n"

char *get_block_type(int block_num) {
    if (block_num < 5) {
        return "Resettable OTP bits";
    } else if (block_num < 7) {
        return "Count down counter";
    } else if (block_num < 16) {
        return "Lockable EEPROM";
    } else {
        return "EEPROM";
    }
}

int main(int argc, char *argv[], char *envp[]) {
    // Options
    int print_columns = 1;

    // Parse options
    int opt = 0;
    while ((opt = getopt(argc, argv, "hvc:")) != -1) {
        switch (opt) {
            case 'v':
                set_verbose(true);
                break;
            case 'c':
                print_columns = atoi(optarg);
                break;
            default:
            case 'h':
                print_usage(argv[0]);
                exit(0);
        }
    }

    // Check arguments
    if ((argc - optind) < 1) {
        print_usage(argv[0]);
        exit(1);
    }

    // Check columns
    if (print_columns != 1 && print_columns != 2) {
        lwarning("Invalid number of columns. Input is %d, but must be either 1 or 2.\n", print_columns);
        print_columns = 1;
    }

    lverbose("Reading \"%s\"...\n", argv[optind]);

    // Load file
    FILE *fp = fopen(argv[optind], "r");
    if (fp == NULL) {
        lerror("Cannot open \"%s\". Exiting...\n", argv[optind]);
        exit(1);
    }

    // Check file size
    int fd = fileno(fp);
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        lerror("Error doing fstat. Exiting...\n");
        exit(1);
    }
    if (file_stat.st_size != SRIX4K_EEPROM_SIZE) {
        lerror("File wrong size, expected %llu but read %llu. Exiting...\n", SRIX4K_EEPROM_SIZE, file_stat.st_size);
        exit(1);
    }

    // Read file
    unsigned char buffer[SRIX4K_EEPROM_SIZE];
    fgets(buffer, SRIX4K_EEPROM_SIZE + 1, fp);
    fclose(fp);

    if (print_columns == 1) {
        // Single column print
        int block_num = 0;
        for (int i = 0; i < SRIX4K_EEPROM_SIZE; i += 4) {
            printf("[%02X]> %02X %02X %02X %02X --- %s\n", block_num, buffer[i], buffer[i+1], buffer[i+2],
                   buffer[i+3], get_block_type(block_num));

            block_num++;
        }
    } else {
        // Double column print
        int block_num = 0;
        for (int i = 0; i < SRIX4K_EEPROM_SIZE; i += 8) {
            printf("%19s --- [%02X]> %02X %02X %02X %02X  %02X %02X %02X %02X <[%02X] --- %s\n",
                   get_block_type(block_num), block_num, buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3],
                   buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7], block_num + 1,
                   get_block_type(block_num + 1));

            block_num += 2;
        }
    }

    return 0;
}