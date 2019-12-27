#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <nfc/nfc.h>
#include "logging.h"

#define MAX_FRAME_LEN 10
#define SRIX4K_EEPROM_SIZE 512
#define SRIX4K_EEPROM_BLOCKS 128
#define SRI512_EEPROM_SIZE 64
#define SRI512_EEPROM_BLOCKS 16

// SRIX4K and SRI512 commands
static __uint8_t get_uid_command[1] = { 0x0B };
static __uint8_t read_block_command[2] = { 0x08 };
static __uint8_t write_block_command[6] = { 0x09 };

static const nfc_modulation nmISO14443B = {
        .nmt = NMT_ISO14443B,
        .nbr = NBR_106,
};

static const nfc_modulation nmISO14443B2SR = {
        .nmt = NMT_ISO14443B2SR,
        .nbr = NBR_106,
};

static void print_usage(const char *executable) {
    printf("Usage: %s [-h] [-v] [-a] [-s] [-u] [-r] [-t x4k|512] [-o dump.bin]\n", executable);
    printf("\nOptions:\n");
    printf("  -h           Shows this help message\n");
    printf("  -v           Enables verbose - print debugging data\n");
    printf("  -a           Enables -s and -u flags together\n");
    printf("  -s           Prints system block\n");
    printf("  -u           Prints UID\n");
    printf("  -r           Fix read direction\n");
    printf("  -t x4k|512   Select SRIX4K or SRI512 tag type [default: x4k]\n");
    printf("  -o dump.bin  Dump EEPROM to file\n");
}

static char *get_block_type(int block_num) {
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

static void log_command_sent(const __uint8_t *command, const size_t num_bytes) {
    if (!verbose_status) {
        return;
    }

    printf("TX >> ");
    for (int i = 0; i < num_bytes; i++) {
        printf("%02X ", command[i]);
    }
    printf("\n");
}

static void log_command_received(const uint8_t *command, const size_t num_bytes) {
    if (!verbose_status) {
        return;
    }

    printf("RX << ");
    for (int i = 0; i < num_bytes; i++) {
        printf("%02X ", command[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[], char *envp[]) {
    bool print_system_block = false;
    bool print_uid = false;
    bool fix_read_direction = false;
    char *output_path = NULL;

    // Parse options
    int opt = 0;
    while ((opt = getopt(argc, argv, "hvasuro:")) != -1) {
        switch (opt) {
            case 'v':
                set_verbose(true);
                break;
            case 'a':
                print_system_block = true;
                print_uid = true;
                break;
            case 's':
                print_system_block = true;
                break;
            case 'u':
                print_uid = true;
                break;
            case 'r':
                fix_read_direction = true;
                break;
            case 'o':
                output_path = optarg;
                break;
            default:
            case 'h':
                print_usage(argv[0]);
                exit(0);
        }
    }

    // Initialize libnfc and set the nfc_context
    nfc_context *context;
    nfc_init(&context);
    if (context == NULL) {
        lerror("Unable to init libnfc. Exiting...\n");
        exit(1);
    }

    // Display libnfc version
    lverbose("Using libnfc version: %s\n", nfc_version());

    /*
     * Open, using the first available NFC device which can be in order of selection:
     *   - default device specified using environment variable or
     *   - first specified device in libnfc.conf (/etc/nfc) or
     *   - first specified device in device-configuration directory (/etc/nfc/devices.d) or
     *   - first auto-detected (if feature is not disabled in libnfc.conf) device
     */
    nfc_device *pnd = nfc_open(context, NULL);

    // Check if no readers are available
    if (pnd == NULL) {
        lerror("Unable to open NFC device. Exiting...\n");
        nfc_exit(context);
        exit(1);
    }

    // Set opened NFC device to initiator mode
    if (nfc_initiator_init(pnd) < 0) {
        lerror("nfc_initiator_init => %s\n", nfc_strerror(pnd));
        nfc_close(pnd);
        nfc_exit(context);
        exit(1);
    }

    lverbose("NFC reader: %s opened\n", nfc_device_get_name(pnd));

    nfc_target ant[1];

    /*
     * This is a known bug from libnfc.
     * To read ISO14443B2SR you have to initiate first ISO14443B to configure internal registers.
     *
     * https://github.com/nfc-tools/libnfc/issues/436#issuecomment-326686914
     */
    lverbose("Searching for ISO14443B targets... found %d.\n", nfc_initiator_list_passive_targets(pnd, nmISO14443B, ant, 1));

    lverbose("Searching for ISO14443B2SR targets...");
    int ISO14443B2SR_targets = nfc_initiator_list_passive_targets(pnd, nmISO14443B2SR, ant, 1);
    lverbose(" found %d.\n", ISO14443B2SR_targets);

    // Check for tags
    nfc_target nt;
    if (ISO14443B2SR_targets == 0) {
        printf("Waiting for tag...\n");

        // Infinite select for tag
        if (nfc_initiator_select_passive_target(pnd, nmISO14443B2SR, NULL, 0, &nt) <= 0) {
            lerror("nfc_initiator_select_passive_target => %s\n", nfc_strerror(pnd));
            nfc_close(pnd);
            nfc_exit(context);
            exit(1);
        }
    }

    // Get UID command
    __uint8_t uid_bytes_read = 0;
    __uint8_t uid_rx_bytes[MAX_FRAME_LEN] = {};
    log_command_sent(get_uid_command, sizeof(get_uid_command));
    if ((uid_bytes_read = nfc_initiator_transceive_bytes(pnd, get_uid_command, sizeof(get_uid_command), uid_rx_bytes, sizeof(uid_rx_bytes), 0)) < 0) {
        lerror("nfc_initiator_transceive_bytes => %s\n", nfc_strerror(pnd));
        nfc_close(pnd);
        nfc_exit(context);
        exit(1);
    }
    log_command_received(uid_rx_bytes, uid_bytes_read);

    // Check for errors
    if (uid_bytes_read != 8) {
        lerror("Error while reading UID. Exiting...\n");
        lverbose("Received %d bytes instead of 8.\n", uid_bytes_read);
        nfc_close(pnd);
        nfc_exit(context);
        exit(1);
    }

    // Print UID
    if (print_uid) {
        printf("UID: ");
        for (int i = uid_bytes_read - 1; i >= 0 ; i--) {
            printf("%02X", uid_rx_bytes[i]);
        }
        printf("\n");

        // Convert UID to unsigned int64
        __uint64_t uid_uint64 = (__uint64_t) uid_rx_bytes[7] << 56u | (__uint64_t) uid_rx_bytes[6] << 48u |
                                (__uint64_t) uid_rx_bytes[5] << 40u | (__uint64_t) uid_rx_bytes[4] << 32u |
                                uid_rx_bytes[3] << 24u | uid_rx_bytes[2] << 16u |
                                uid_rx_bytes[1] << 8u | uid_rx_bytes[0];

        // Convert uint64 to binary char array
        char uid_binary[65] = {};
        for (int i = 0; i < sizeof(uid_uint64); i++) {
            __uint8_t tmp = (uid_uint64 >> (sizeof(uid_uint64) - 1 - i) * 8u) & 0xFFu;
            sprintf(uid_binary + i * 8 + 0, "%c", tmp & 0x80u ? '1' : '0');
            sprintf(uid_binary + i * 8 + 1, "%c", tmp & 0x40u ? '1' : '0');
            sprintf(uid_binary + i * 8 + 2, "%c", tmp & 0x20u ? '1' : '0');
            sprintf(uid_binary + i * 8 + 3, "%c", tmp & 0x10u ? '1' : '0');
            sprintf(uid_binary + i * 8 + 4, "%c", tmp & 0x08u ? '1' : '0');
            sprintf(uid_binary + i * 8 + 5, "%c", tmp & 0x04u ? '1' : '0');
            sprintf(uid_binary + i * 8 + 6, "%c", tmp & 0x02u ? '1' : '0');
            sprintf(uid_binary + i * 8 + 7, "%c", tmp & 0x01u ? '1' : '0');
        }
        uid_binary[64] = 0;

        printf(" ⤷ Prefix: %02X\n", uid_rx_bytes[7]);
        printf(" ⤷ IC manufacturer code: %02X", uid_rx_bytes[6]);
        switch (uid_rx_bytes[6]) {
            case 0x02:
                printf(" (STMicroelectronics)\n");
                break;
            default:
                printf(" (unknown)\n");
        }

        // Print 6bit IC code
        char ic_code[10] = {};
        strncpy(ic_code, uid_binary + 16, 6);
        printf(" ⤷ IC code: %s\n", ic_code);

        // Print 42bit unique serial number
        char unique_serial_number[50] = {};
        strncpy(unique_serial_number, uid_binary + 22, 42);
        printf(" ⤷ 42bit unique serial number: %s\n", unique_serial_number);
    }

    // Read EEPROM
    lverbose("Reading %d blocks...\n", SRIX4K_EEPROM_BLOCKS);
    __uint8_t eeprom_bytes[SRIX4K_EEPROM_SIZE] = {};

    for (int i = 0; i < SRIX4K_EEPROM_BLOCKS; i++) {
        // Set target block to command
        read_block_command[1] = i;

        __uint8_t *current_block = eeprom_bytes + (i * 4);
        __uint8_t block_bytes_read = 0;

        log_command_sent(read_block_command, sizeof(read_block_command));
        if ((block_bytes_read = nfc_initiator_transceive_bytes(pnd, read_block_command, sizeof(read_block_command), current_block, sizeof(current_block), 0)) < 0) {
            lerror("nfc_initiator_transceive_bytes => %s\n", nfc_strerror(pnd));
            nfc_close(pnd);
            nfc_exit(context);
            exit(1);
        }
        log_command_received(current_block, block_bytes_read);

        // Check for errors
        if (block_bytes_read != 4) {
            lerror("Error while reading block %d. Exiting...\n", i);
            lverbose("Received %d bytes instead of 4.\n", block_bytes_read);
            nfc_close(pnd);
            nfc_exit(context);
            exit(1);
        }

        printf("[%02X]> ", i);

        if (fix_read_direction) {
            for (int j = block_bytes_read - 1; j >= 0 ; j--) {
                printf("%02X ", current_block[j]);
            }
        } else {
            for (int j = 0; j < block_bytes_read; j++) {
                printf("%02X ", current_block[j]);
            }
        }
        printf("--- %s\n", get_block_type(i));
    }

    if (print_system_block) {
        // Set target block to command
        read_block_command[1] = 0xFF;

        __uint8_t block_bytes_read = 0;
        __uint8_t block_rx_bytes[MAX_FRAME_LEN] = {};

        log_command_sent(read_block_command, sizeof(read_block_command));
        if ((block_bytes_read = nfc_initiator_transceive_bytes(pnd, read_block_command, sizeof(read_block_command), block_rx_bytes, sizeof(block_rx_bytes), 0)) < 0) {
            lerror("nfc_initiator_transceive_bytes => %s\n", nfc_strerror(pnd));
            nfc_close(pnd);
            nfc_exit(context);
            exit(1);
        }
        log_command_received(block_rx_bytes, block_bytes_read);

        // Check for errors
        if (block_bytes_read != 4) {
            lerror("Error while reading system block. Exiting...\n");
            lverbose("Received %d bytes instead of 4.\n", block_bytes_read);
            nfc_close(pnd);
            nfc_exit(context);
            exit(1);
        }

        printf("System block: ");
        if (fix_read_direction) {
            for (int j = block_bytes_read - 1; j >= 0 ; j--) {
                printf("%02X ", block_rx_bytes[j]);
            }
        } else {
            for (int i = 0; i < block_bytes_read; i++) {
                printf("%02X ", block_rx_bytes[i]);
            }
        }
        printf("\n");

        __uint32_t a = block_rx_bytes[3] << 24u | block_rx_bytes[2] << 16u | block_rx_bytes[1] << 8u | block_rx_bytes[0];

        printf(" ⤷ CHIP_ID: %02X\n", block_rx_bytes[0]);
        printf(" ⤷ ST reserved: %02X%02X\n", block_rx_bytes[1], block_rx_bytes[2]);

        printf(" ⤷ OTP_Lock_Reg:\n");
        for (u_int8_t i = 24; i < 32; i++) {
            printf("    ⤷ b%d = %d - ", i, (a >> i) & 1u);
            if (i == 24) {
                printf("Block 07 and 08 are ");
            } else {
                printf("Block %02X is ", i - 16);
            }

            if (((a >> i) & 1u) == 0) {
                printf(RED);
                printf("LOCKED\n");
                printf(RESET);
            } else {
                printf(GREEN);
                printf("unlocked\n");
                printf(RESET);
            }
        }
    }

    // Dump to file
    if (output_path != NULL) {
        FILE *fp = fopen(output_path, "w");
        fwrite(eeprom_bytes, 1, 512, fp);
        fclose(fp);

        printf("Written dump to \"%s\".\n", output_path);
    }

    // Close NFC
    nfc_close(pnd);
    nfc_exit(context);

    return 0;
}