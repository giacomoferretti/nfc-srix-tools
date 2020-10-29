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

#ifndef __NFC_SRIX_UTILS_H__
#define __NFC_SRIX_UTILS_H__

/* Macros */
#define MAX_DEVICE_COUNT 16
#define MAX_TARGET_COUNT 1
#define MAX_RESPONSE_LEN 10
#define SRIX4K_EEPROM_SIZE 512
#define SRIX4K_EEPROM_BLOCKS 128
#define SRI512_EEPROM_SIZE 64
#define SRI512_EEPROM_BLOCKS 16
#define SR_GET_UID_COMMAND 0x0B
#define SR_READ_BLOCK_COMMAND 0x08
#define SR_WRITE_BLOCK_COMMAND 0x09

/* Constants */
extern const nfc_modulation nmISO14443B;
extern const nfc_modulation nmISO14443B2SR;

/* Logging */
void log_command_sent(const uint8_t *command, size_t num_bytes);
void log_command_received(const uint8_t *command, size_t num_bytes);

/* Commands */
size_t nfc_transceive_bytes(nfc_device *reader, const uint8_t *tx_data, size_t tx_size, uint8_t *rx_data);
size_t nfc_srix_get_uid(nfc_device *reader, uint8_t *rx_data);
size_t nfc_srix_read_block(nfc_device *reader, uint8_t *rx_data, uint8_t block);
size_t nfc_srix_write_block(nfc_device *reader, uint8_t *rx_data, uint8_t block, const uint8_t *data);
void nfc_write_block(nfc_device *pnd, uint32_t block, uint8_t block_num);
void nfc_write_block_bytes(nfc_device *pnd, uint8_t *block, uint8_t block_num);

/* Utilities */
char *srix_get_block_type(uint8_t block_num);
uint32_t eeprom_bytes_to_block(uint8_t *dump, uint8_t block);
void close_nfc(nfc_context *context, nfc_device *reader);

#endif // __NFC_SRIX_UTILS_H__
