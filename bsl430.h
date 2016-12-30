/*
 * Copyright (C) 2016 Whaley Technology Co., Ltd.
 * Min Chen <chen.min@whaley.cn>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * bsl430:
 *      A MSP430 BSL protocol ('5xx, 6xx' UART) host side implementation.
 *
 *      It is verified with MSP430FR2x33 and the UART is used as the
 *      communication interface.
 *
 *      SLAU610A: MSP430FR4xx and MSP430FR2xx Bootloader (BSL)
 *      SLAU319K: MSP430 Programming With the Bootloader (BSL)
 */

#ifndef __BSL430_H__
#define __BSL430_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSL430_MSG_SUCC             0x00
#define BSL430_MSG_FLASH_FAIL       0x01
#define BSL430_MSG_BSL_LOCKED       0x04
#define BSL430_MSG_PASSWD_ERROR     0x05
#define BSL430_MSG_UNKNOWN_CMD      0x07

int bsl430_enter(int entry_seq);
int bsl430_exit(void);

int bsl430_cmd_rx_data_block(uint32_t address, uint8_t *data, uint16_t size);
int bsl430_cmd_rx_password(uint8_t *password, uint16_t len);
int bsl430_cmd_crc_check(uint32_t address, uint16_t size, uint16_t *crc);
int bsl430_cmd_tx_data_block(uint32_t address, uint16_t size, uint8_t *buf);
int bsl430_cmd_tx_version(uint32_t *version);
int bsl430_cmd_change_baudrate(uint32_t baudrate);

uint16_t bsl430_crc16_add(uint8_t b, uint16_t acc);
uint16_t bsl430_crc16(const uint8_t *data, int len, uint16_t acc);

#ifdef __cplusplus
}
#endif

#endif /* __BSL430_H__ */
