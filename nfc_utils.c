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

#include <nfc/nfc.h>
#include "nfc_utils.h"
#include "logging.h"

const nfc_modulation nmISO14443B = {
        .nmt = NMT_ISO14443B,
        .nbr = NBR_106,
};

const nfc_modulation nmISO14443B2SR = {
        .nmt = NMT_ISO14443B2SR,
        .nbr = NBR_106,
};

void log_command_sent(const uint8_t *command, size_t num_bytes) {
    if (verbosity_level < 2) {
        return;
    }

    printf("TX >> ");
    for (unsigned int i = 0; i < num_bytes; i++) {
        printf("%02X ", command[i]);
    }
    printf("\n");
}

void log_command_received(const uint8_t *command, size_t num_bytes) {
    if (verbosity_level < 2) {
        return;
    }

    if (num_bytes > MAX_RESPONSE_LEN) {
        return;
    }

    printf("RX << ");
    for (unsigned int i = 0; i < num_bytes; i++) {
        printf("%02X ", command[i]);
    }
    printf("\n");
}

size_t nfc_transceive_bytes(nfc_device *reader, const uint8_t *tx_data, size_t tx_size, uint8_t *rx_data) {
    log_command_sent(tx_data, tx_size);

    size_t rx_size = nfc_initiator_transceive_bytes(reader, tx_data, tx_size, rx_data, sizeof(rx_data), 0);

    if (rx_data != NULL) {
        log_command_received(rx_data, rx_size);
    }

    return rx_size;
}

size_t nfc_srix_get_uid(nfc_device *reader, uint8_t *rx_data) {
    uint8_t cmd[1] = {SR_GET_UID_COMMAND};
    return nfc_transceive_bytes(reader, cmd, sizeof(cmd), rx_data);
}

size_t nfc_srix_read_block(nfc_device *reader, uint8_t *rx_data, uint8_t block) {
    uint8_t cmd[2] = {SR_READ_BLOCK_COMMAND};
    cmd[1] = block;
    return nfc_transceive_bytes(reader, cmd, sizeof(cmd), rx_data);
}

size_t nfc_srix_write_block(nfc_device *reader, uint8_t *rx_data, uint8_t block, const uint8_t *data) {
    uint8_t cmd[6] = {SR_WRITE_BLOCK_COMMAND};
    cmd[1] = block;
    cmd[2] = data[0];
    cmd[3] = data[1];
    cmd[4] = data[2];
    cmd[5] = data[3];
    return nfc_transceive_bytes(reader, cmd, sizeof(cmd), rx_data);
}

void nfc_write_block(nfc_device *pnd, uint32_t block, uint8_t block_num) {
    uint8_t bytes[4] = {};
    bytes[0] = block >> 24u;
    bytes[1] = block >> 16u;
    bytes[2] = block >> 8u;
    bytes[3] = block >> 0u;

    printf("Writing block %02X... ", block_num);
    nfc_srix_write_block(pnd, NULL, block_num, bytes);
    printf("Done!\n");
}

void nfc_write_block_bytes(nfc_device *pnd, uint8_t *block, uint8_t block_num) {
    printf("Writing block %02X... ", block_num);
    nfc_srix_write_block(pnd, NULL, block_num, block);
    printf("Done!\n");
}

char *srix_get_block_type(uint8_t block_num) {
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

uint32_t eeprom_bytes_to_block(uint8_t *dump, uint8_t block) {
    return (dump[(block*4)] << 24u) + (dump[(block*4)+1] << 16u) + (dump[(block*4)+2] << 8u) + dump[(block*4)+3];
}

void close_nfc(nfc_context *context, nfc_device *reader) {
    if (reader != NULL) nfc_close(reader);
    if (context != NULL) nfc_exit(context);
}