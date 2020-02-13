/*
 * Copyright 2019-2020 Giacomo Ferretti
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
#include <stdbool.h>
#include <string.h>
#include <nfc/nfc.h>
#include <sys/stat.h>
#include "logging.h"
#include "nfc_utils.h"

static void print_usage(const char *executable) {
    printf("Usage: %s <dump.bin> [-h] [-v] [-c 1|2] [-t x4k|512]\n", executable);
    printf("\nNecessary arguments:\n");
    printf("  <dump.bin>   path to the dump file\n");
    printf("\nOptions:\n");
    printf("  -h           show this help message\n");
    printf("  -v           enable verbose - print debugging data\n");
    printf("  -c 1|2       erint on one or two columns [default: 1]\n");
    printf("  -t x4k|512   select SRIX4K or SRI512 tag type [default: x4k]\n");
}

int main(int argc, char *argv[], char *envp[]) {
    // Options
    int print_columns = 1;
    uint32_t eeprom_size = SRIX4K_EEPROM_SIZE;
    uint32_t eeprom_blocks_amount = SRIX4K_EEPROM_BLOCKS;

    // Parse options
    int opt = 0;
    while ((opt = getopt(argc, argv, "hvc:t:")) != -1) {
        switch (opt) {
            case 'v':
                set_verbose(true);
                break;
            case 'c':
                print_columns = (int) strtol(optarg, NULL, 10);
                break;
            case 't':
                if (strcmp(optarg, "512") == 0) {
                    eeprom_size = SRI512_EEPROM_SIZE;
                    eeprom_blocks_amount = SRI512_EEPROM_BLOCKS;
                }
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
        lwarning("Invalid number of columns. Input is %d, but must be either 1 or 2.\nUsing default value.\n", print_columns);
        print_columns = 1;
    }

    char *file_path = argv[optind];
    uint8_t *eeprom_bytes = malloc(sizeof(uint8_t) * eeprom_size);

    // Open file
    lverbose("Reading \"%s\"...\n", file_path);
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        lerror("Cannot open \"%s\". Exiting...\n", file_path);
        exit(1);
    }

    // Check file size
    int fd = fileno(fp);
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        lerror("Error doing fstat. Exiting...\n");
        exit(1);
    }
    if (file_stat.st_size < eeprom_size) {
        lerror("File wrong size, expected %llu but read %llu. Exiting...\n", eeprom_size, file_stat.st_size);
        exit(1);
    }

    // Read file
    fseek(fp, 0, SEEK_SET);
    if (fread(eeprom_bytes, eeprom_size, 1, fp) != 1) {
        lerror("Error encountered while reading file. Exiting...\n");
        exit(1);
    }
    fclose(fp);

    if (print_columns == 1) { // Single column print
        for (int i = 0; i < eeprom_blocks_amount; i++) {
            uint8_t *block = eeprom_bytes + (i * 4);
            printf("[%02X] %02X %02X %02X %02X" DIM " --- %s\n" RESET, i, block[0], block[1], block[2], block[3], srix_get_block_type(i));
        }
    } else { // Double column print
        for (int i = 0; i < eeprom_blocks_amount; i += 2) {
            uint8_t *block_1 = eeprom_bytes + (i * 4);
            uint8_t *block_2 = eeprom_bytes + ((i+1) * 4);
            printf(DIM "%19s --- " RESET "[%02X] %02X %02X %02X %02X  %02X %02X %02X %02X [%02X]" DIM " --- %s\n" RESET,
                    srix_get_block_type(i), i, block_1[0], block_1[1], block_1[2], block_1[3], block_2[0], block_2[1], block_2[2], block_2[3], i+1, srix_get_block_type(i+1));
        }
    }

    return 0;
}