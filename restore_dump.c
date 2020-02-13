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
#include <string.h>
#include <unistd.h>
#include <nfc/nfc.h>
#include <sys/stat.h>
#include "logging.h"
#include "nfc_utils.h"

static void print_usage(const char *executable) {
    printf("Usage: %s <dump.bin> [-h] [-v] [-t x4k|512]\n", executable);
    printf("\nNecessary arguments:\n");
    printf("  <dump.bin>   path to the dump file\n");
    printf("\nOptions:\n");
    printf("  -h           show this help message\n");
    printf("  -v           enable verbose - print debugging data\n");
    printf("  -t x4k|512   select SRIX4K or SRI512 tag type [default: x4k]\n");
}

int main(int argc, char *argv[], char *envp[]) {
    // Options
    uint16_t eeprom_size = SRIX4K_EEPROM_SIZE;
    uint8_t eeprom_blocks_amount = SRIX4K_EEPROM_BLOCKS;

    // Parse options
    int opt = 0;
    while ((opt = getopt(argc, argv, "hvt:")) != -1) {
        switch (opt) {
            case 'v':
                set_verbose(true);
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
        lerror("You need to specify a path for <dump.bin>.\n\n");
        print_usage(argv[0]);
        exit(1);
    }

    // Initialize NFC
    nfc_context *context = NULL;
    nfc_device *reader = NULL;
    nfc_init(&context);
    if (context == NULL) {
        lerror("Unable to init libnfc. Exiting...\n");
        exit(1);
    }

    // Display libnfc version
    lverbose("libnfc version: %s\n", nfc_version());

    // Search for readers
    lverbose("Searching for readers... ");
    nfc_connstring connstrings[MAX_DEVICE_COUNT] = {};
    size_t num_readers = nfc_list_devices(context, connstrings, MAX_DEVICE_COUNT);
    lverbose("found %zu.\n", num_readers);

    // Check if no readers are available
    if (num_readers == 0) {
        lerror("No readers available. Exiting...\n");
        close_nfc(context, reader);
        exit(1);
    }

    // Print out readers
    for (unsigned int i = 0; i < num_readers; i++) {
        if (i == num_readers - 1) {
            lverbose("└── ");
        } else {
            lverbose("├── ");
        }
        lverbose("[%d] %s\n", i, connstrings[i]);
    }
    lverbose("Opening %s...\n", connstrings[0]);

    // Open first reader
    reader = nfc_open(context, connstrings[0]);
    if (reader == NULL) {
        lerror("Unable to open NFC device. Exiting...\n");
        close_nfc(context, reader);
        exit(1);
    }

    // Set opened NFC device to initiator mode
    if (nfc_initiator_init(reader) < 0) {
        lerror("nfc_initiator_init => %s\n", nfc_strerror(reader));
        close_nfc(context, reader);
        exit(1);
    }

    lverbose("NFC reader: %s\n", nfc_device_get_name(reader));

    nfc_target target_key[MAX_TARGET_COUNT];

    /*
     * This is a known bug from libnfc.
     * To read ISO14443B2SR you have to initiate first ISO14443B to configure internal registers.
     *
     * https://github.com/nfc-tools/libnfc/issues/436#issuecomment-326686914
     */
    lverbose("Searching for ISO14443B targets... found %d.\n", nfc_initiator_list_passive_targets(reader, nmISO14443B, target_key, MAX_TARGET_COUNT));

    lverbose("Searching for ISO14443B2SR targets...");
    int ISO14443B2SR_targets = nfc_initiator_list_passive_targets(reader, nmISO14443B2SR, target_key, MAX_TARGET_COUNT);
    lverbose(" found %d.\n", ISO14443B2SR_targets);

    // Check for tags
    if (ISO14443B2SR_targets == 0) {
        printf("Waiting for tag...\n");

        // Infinite select for tag
        if (nfc_initiator_select_passive_target(reader, nmISO14443B2SR, NULL, 0, target_key) <= 0) {
            lerror("nfc_initiator_select_passive_target => %s\n", nfc_strerror(reader));
            close_nfc(context, reader);
            exit(1);
        }
    }

    char *file_path = argv[optind];
    uint8_t *dump_bytes = malloc(sizeof(uint8_t) * eeprom_size);

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
    if (fread(dump_bytes, eeprom_size, 1, fp) != 1) {
        lerror("Error encountered while reading file. Exiting...\n");
        exit(1);
    }
    fclose(fp);

    // Read EEPROM
    uint8_t *eeprom_bytes = malloc(sizeof(uint8_t) * eeprom_size);
    lverbose("Reading %d blocks...\n", eeprom_blocks_amount);
    for (int i = 0; i < eeprom_blocks_amount; i++) {
        uint8_t *current_block = eeprom_bytes + (i * 4);
        uint8_t block_bytes_read = nfc_srix_read_block(reader, current_block, i);

        // Check for errors
        if (block_bytes_read != 4) {
            lerror("Error while reading block %d. Exiting...\n", i);
            lverbose("Received %d bytes instead of 4.\n", block_bytes_read);
            close_nfc(context, reader);
            exit(1);
        }
    }

    // Preview write
    bool is_equal = true;
    for (uint8_t i = 7; i < eeprom_blocks_amount; i++) {
        uint32_t dump_block = dump_bytes[(i*4)+0] << 24u | dump_bytes[(i*4)+1] << 16u | dump_bytes[(i*4)+2] << 8u | dump_bytes[(i*4)+3];
        uint32_t eeprom_block = eeprom_bytes[(i*4)+0] << 24u | eeprom_bytes[(i*4)+1] << 16u | eeprom_bytes[(i*4)+2] << 8u | eeprom_bytes[(i*4)+3];

        if (dump_block != eeprom_block) {
            is_equal = false;
            printf("[%02X] %08X -> %08X\n", i, dump_block, eeprom_block);
        }
    }

    if (!is_equal) {
        // Ask for confirmation
        printf("This action is irreversible.\n");
        printf("Are you sure? [Y/N] ");
        char c = 'n';
        scanf(" %c", &c);
        if (c != 'Y' && c != 'y') {
            printf("Exiting...\n");
            exit(0);
        }

        for (uint8_t i = 7; i < eeprom_blocks_amount; i++) {
            uint32_t dump_block = dump_bytes[(i*4)+0] << 24u | dump_bytes[(i*4)+1] << 16u | dump_bytes[(i*4)+2] << 8u | dump_bytes[(i*4)+3];
            uint32_t eeprom_block = eeprom_bytes[(i*4)+0] << 24u | eeprom_bytes[(i*4)+1] << 16u | eeprom_bytes[(i*4)+2] << 8u | eeprom_bytes[(i*4)+3];

            if (dump_block != eeprom_block) {
                nfc_write_block(reader, dump_block, i);
            }
        }
    } else {
        printf("Tag already restored.\n");
    }

    // Close NFC
    close_nfc(context, reader);

    return 0;
}