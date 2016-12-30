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
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "bsl430"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bsl430-platform.h"
#include "bsl430.h"

#define HEAD    0x80
#define INITFCS 0xFFFF
#define ACK     0x00

#define CHAR_TIMEOUT    10  /* ms */
#define RESP_TIMEOUT   100  /* ms */

#define STATE_INTERVAL  20  /* ms */

/*
 * The minimum time delay before sending new characters
 * after characters have been received from the MSP430 BSL is 1.2 ms.
 */
#define BSL430_SENDING_DELAY    5   /* ms */

#define BSL430_ADDR_LOW     0xC400
#define BSL430_ADDR_HIGH    0xFFFF

/* D1...Dn */
#define BSL430_MAX_DATA_SIZE    256
/* CMD + AL AM AH + D1...Dn */
#define BSL430_MAX_PAYLOADSIZE  (1 + 3 + BSL430_MAX_DATA_SIZE)
/* ACK + Header + NL NH + Payload + CKL CKH */
#define BSL430_MAX_FRAME_SIZE   (1 + 1 + 2 + BSL430_MAX_PAYLOADSIZE + 2)

#define BSL430_CMD_RX_DATA_BLOCK    0x10
#define BSL430_CMD_RX_PASSWORD      0x11
#define BSL430_CMD_MASS_ERASE       0x15
#define BSL430_CMD_CRC_CHECK        0x16
#define BSL430_CMD_LOAD_PC          0x17
#define BSL430_CMD_TX_DATA_BLOCK    0x18
#define BSL430_CMD_TX_BSL_VERSION   0x19
#define BSL430_CMD_RX_DATA_BLOCK_F  0x1B
#define BSL430_CMD_CHANGE_BAUDRATE  0x52

#define BSL430_RESP_DATA            0x3A
#define BSL430_RESP_MSG             0x3B

typedef struct bsl430_frame_s {
    uint16_t len;
    uint8_t  payload[BSL430_MAX_PAYLOADSIZE];

    uint16_t fcs;
} bsl430_frame_t;

static int bsl430_frame_send(bsl430_frame_t *frame);
static int bsl430_frame_recv(bsl430_frame_t *frame, int resp, uint16_t timeout);

int bsl430_enter(int entry_seq)
{
    int status = 0;
    uint32_t version = 0;

    bsl430_gpio_init();

    if (entry_seq) {
        /*                      ___________________
         * RST ________________|
         *            __      ____
         * TST ______|  |____|    |________________
         */
        bsl430_gpio_rst(0);
        bsl430_gpio_tst(0);
        mdelay(STATE_INTERVAL * 2);

        bsl430_gpio_tst(1);
        mdelay(STATE_INTERVAL);
        bsl430_gpio_tst(0);

        mdelay(STATE_INTERVAL * 2);

        bsl430_gpio_tst(1);

        mdelay(STATE_INTERVAL);
        bsl430_gpio_rst(1);

        mdelay(STATE_INTERVAL);
        bsl430_gpio_tst(0);
    }

    /*
     * SLAU610A: UART Protocol Definition
     * Baud rate is configured to start at 9600 baud in half-duplex mode.
     * Start bit, 8 data bits (LSB first), an even parity bit, 1 stop bit.
     */
    bsl430_uart_init(9600, 0);

    mdelay(100);

    /*
     * WORKAROUND:
     * Recover the UART communication in u-boot.
     * PL011 is NOT reset during initialization,
     * it may be in error state.
     */
    status = bsl430_cmd_tx_version(&version);
    if (status != 0 && status != BSL430_MSG_BSL_LOCKED) {
        log("UART recovered!\n");
    }

    return 0;
}

int bsl430_exit(void)
{
    bsl430_uart_term();

    /*     ______      ______
     * RST       |____|
     */
    bsl430_gpio_rst(0);
    mdelay(STATE_INTERVAL);
    bsl430_gpio_rst(1);

    return 0;
}

int bsl430_cmd_rx_data_block(uint32_t address, uint8_t *data, uint16_t size)
{
    int status = 0;
    bsl430_frame_t txframe;
    bsl430_frame_t rxframe;
    uint16_t write_size = 0;

    if (address < BSL430_ADDR_LOW || address > BSL430_ADDR_HIGH) {
        log("** Start address out of range.\n");
        return -1;
    }

    if ((address + size) > (BSL430_ADDR_HIGH + 1)) {
        log("** Access out of range.\n");
        return -1;
    }

    if (!data) {
        return -1;
    }

    while (size > 0) {
        write_size = (size > BSL430_MAX_DATA_SIZE)? BSL430_MAX_DATA_SIZE: size;

        memset(&txframe, 0, sizeof(rxframe));
        memset(&rxframe, 0, sizeof(rxframe));

        log("RX_DATA: @%04X %3u Bytes\n", address, write_size);

        txframe.payload[0] = BSL430_CMD_RX_DATA_BLOCK;
        txframe.payload[1] = (uint8_t)(address >>  0 & 0xFF);
        txframe.payload[2] = (uint8_t)(address >>  8 & 0xFF);
        txframe.payload[3] = (uint8_t)(address >> 16 & 0xFF);
        memcpy(&txframe.payload[4], data, write_size);
        txframe.len = 1 + 3 + write_size;

        bsl430_frame_send(&txframe);

        status = bsl430_frame_recv(&rxframe, 1, RESP_TIMEOUT);
        if (status == 0) {
            status = rxframe.payload[1];
        }

        if (status != 0) {
            log("** RX_DATA_BLOCK failed! 0x%02X\n", (uint8_t)status);
            break;
        }

        address += write_size;
        data    += write_size;
        size    -= write_size;
    }

    return status;
}

int bsl430_cmd_rx_password(uint8_t *password, uint16_t len)
{
    int status = 0;
    bsl430_frame_t txframe;
    bsl430_frame_t rxframe;

    if (!password || len != 32) {
        return -1;
    }

    memset(&txframe, 0, sizeof(rxframe));
    memset(&rxframe, 0, sizeof(rxframe));

    txframe.payload[0] = BSL430_CMD_RX_PASSWORD;
    memcpy(&txframe.payload[1], password, len);
    txframe.len = 1 + len;

    bsl430_frame_send(&txframe);

    status = bsl430_frame_recv(&rxframe, 1, RESP_TIMEOUT);
    if (status == 0) {
        status = rxframe.payload[1];
    }

    return status;
}

int bsl430_cmd_crc_check(uint32_t address, uint16_t size, uint16_t *crc)
{
    int status = 0;
    bsl430_frame_t txframe;
    bsl430_frame_t rxframe;

    if (address < BSL430_ADDR_LOW || address > BSL430_ADDR_HIGH) {
        log("** Start address out of range.\n");
        return -1;
    }

    if ((address + size) > (BSL430_ADDR_HIGH + 1)) {
        log("** Access out of range.\n");
        return -1;
    }

    if (!crc) {
        return -1;
    }

    memset(&txframe, 0, sizeof(rxframe));
    memset(&rxframe, 0, sizeof(rxframe));

    txframe.payload[0] = BSL430_CMD_CRC_CHECK;
    txframe.payload[1] = (uint8_t)(address >>  0 & 0xFF);
    txframe.payload[2] = (uint8_t)(address >>  8 & 0xFF);
    txframe.payload[3] = (uint8_t)(address >> 16 & 0xFF);
    txframe.payload[4] = (uint8_t)(size >> 0 & 0xFF);
    txframe.payload[5] = (uint8_t)(size >> 8 & 0xFF);
    txframe.len = 6;

    bsl430_frame_send(&txframe);

    status = bsl430_frame_recv(&rxframe, 1, RESP_TIMEOUT);
    if (status == 0) {
        if (rxframe.payload[0] == BSL430_RESP_DATA) {
            *crc = (uint16_t)rxframe.payload[1] << 0 |
                   (uint16_t)rxframe.payload[2] << 8;
        } else {
            status = rxframe.payload[1];
        }
    }

    return status;
}

int bsl430_cmd_tx_data_block(uint32_t address, uint16_t size, uint8_t *buf)
{
    int status = 0;
    bsl430_frame_t txframe;
    bsl430_frame_t rxframe;
    uint16_t read_size = 0;

    if (address < BSL430_ADDR_LOW || address > BSL430_ADDR_HIGH) {
        log("** Start address out of range.\n");
        return -1;
    }

    if ((address + size) > (BSL430_ADDR_HIGH + 1)) {
        log("** Access out of range.\n");
        return -1;
    }

    if (!buf) {
        return -1;
    }

    while (size > 0) {
        read_size = (size > BSL430_MAX_DATA_SIZE)? BSL430_MAX_DATA_SIZE: size;

        memset(&txframe, 0, sizeof(rxframe));
        memset(&rxframe, 0, sizeof(rxframe));

        log("TX_DATA: @%04X %3u Bytes\n", address, read_size);

        txframe.payload[0] = BSL430_CMD_TX_DATA_BLOCK;
        txframe.payload[1] = (uint8_t)(address >>  0 & 0xFF);
        txframe.payload[2] = (uint8_t)(address >>  8 & 0xFF);
        txframe.payload[3] = (uint8_t)(address >> 16 & 0xFF);
        txframe.payload[4] = (uint8_t)(read_size >> 0 & 0xFF);
        txframe.payload[5] = (uint8_t)(read_size >> 8 & 0xFF);
        txframe.len = 6;

        bsl430_frame_send(&txframe);

        status = bsl430_frame_recv(&rxframe, 1, RESP_TIMEOUT);
        if (status == 0) {
            if (rxframe.payload[0] == BSL430_RESP_DATA) {
                memcpy(buf, &rxframe.payload[1], read_size);
            } else {
                status = rxframe.payload[1];
            }
        }

        if (status != 0) {
            log("** TX_DATA_BLOCK failed! 0x%02X\n", (uint8_t)status);
            break;
        }

        address += read_size;
        buf     += read_size;
        size    -= read_size;
    }

    return status;
}

int bsl430_cmd_tx_version(uint32_t *version)
{
    int status = 0;
    bsl430_frame_t txframe;
    bsl430_frame_t rxframe;

    if (!version) {
        return -1;
    }

    memset(&txframe, 0, sizeof(rxframe));
    memset(&rxframe, 0, sizeof(rxframe));

    txframe.payload[0] = BSL430_CMD_TX_BSL_VERSION;
    txframe.len = 1;

    bsl430_frame_send(&txframe);

    status = bsl430_frame_recv(&rxframe, 1, RESP_TIMEOUT);
    if (status == 0) {
        if (rxframe.payload[0] == BSL430_RESP_DATA) {
            *version = (uint32_t)rxframe.payload[1] << 24 |
                       (uint32_t)rxframe.payload[2] << 16 |
                       (uint32_t)rxframe.payload[3] <<  8 |
                       (uint32_t)rxframe.payload[4] <<  0;
        } else {
            status = rxframe.payload[1];
        }
    }

    return status;
}

int bsl430_cmd_change_baudrate(uint32_t baudrate)
{
    int status = 0;
    uint8_t index;
    bsl430_frame_t txframe;
    bsl430_frame_t rxframe;

    if (baudrate == 9600) {
        index = 0x02;
    } else if (baudrate == 19200) {
        index = 0x03;
    } else if (baudrate == 38400) {
        index = 0x04;
    } else if (baudrate == 57600) {
        index = 0x05;
    } else if (baudrate == 115200) {
        index = 0x06;
    } else {
        return -1;
    }

    memset(&txframe, 0, sizeof(rxframe));
    memset(&rxframe, 0, sizeof(rxframe));

    txframe.payload[0] = BSL430_CMD_CHANGE_BAUDRATE;
    txframe.payload[1] = index;
    txframe.len = 2;

    bsl430_frame_send(&txframe);

    status = bsl430_frame_recv(&rxframe, 0, RESP_TIMEOUT);
    if (status == 0) {
        log("Change baudrate to %d.\n", baudrate);
        bsl430_uart_init(baudrate, 0);
    }

    return status;
}

/*
 * CRC-CCITT (0xFFFF) polynomial ^16 + ^12 + ^5 + 1
 *
 * This CRC-CCITT routine is from Contiki OS,
 * whose author is Adam Dunkels <adam@sics.se>.
 */
uint16_t bsl430_crc16_add(uint8_t b, uint16_t acc)
{
    acc  = (unsigned char)(acc >> 8) | (acc << 8);
    acc ^= b;
    acc ^= (unsigned char)(acc & 0xff) >> 4;
    acc ^= (acc << 8) << 4;
    acc ^= ((acc & 0xff) << 4) << 1;
    return acc;
}

uint16_t bsl430_crc16(const uint8_t *data, int len, uint16_t acc)
{
    int i;

    for(i = 0; i < len; ++i) {
        acc = bsl430_crc16_add(*data, acc);
        ++data;
    }
    return acc;
}

static int bsl430_frame_send(bsl430_frame_t *frame)
{
    int i;

    mdelay(BSL430_SENDING_DELAY);

    bsl430_uart_writeb(HEAD);

    bsl430_uart_writeb((uint8_t)(frame->len >> 0 & 0x00FF));
    bsl430_uart_writeb((uint8_t)(frame->len >> 8 & 0x00FF));

    for (i = 0; i < frame->len; i++) {
        bsl430_uart_writeb(frame->payload[i]);
    }

    frame->fcs = bsl430_crc16(frame->payload, frame->len, INITFCS);

    bsl430_uart_writeb((uint8_t)(frame->fcs >> 0 & 0x00FF));
    bsl430_uart_writeb((uint8_t)(frame->fcs >> 8 & 0x00FF));

    return 0;
}

static int bsl430_frame_recv(bsl430_frame_t *frame, int resp, uint16_t timeout)
{
    int status = 0;
    int i;
    int c = -1;
    uint8_t nl, nh;
    uint16_t len;
    uint8_t ckl, ckh;
    uint16_t cks;

    c = bsl430_uart_readb(timeout);
    if ((uint8_t)c != ACK) {
        log("** Wrong ACK. 0x%02x\n", (uint8_t)c);
        status = (uint8_t)c;
        goto err_exit;
    }

    if (!resp) {
        return 0;
    }

    /* Header */
    c = bsl430_uart_readb(timeout);
    if ((uint8_t)c != HEAD) {
        log("** Wrong head or timeout.\n");
        status = -1;
        goto err_exit;
    }

    /* NL NH */
    c = bsl430_uart_readb(CHAR_TIMEOUT);
    if (c == -1) {
        log("** NL timeout.\n");
        status = -1;
        goto err_exit;
    }
    nl = (uint8_t)c;

    c = bsl430_uart_readb(CHAR_TIMEOUT);
    if (c == -1) {
        log("** NH timeout.\n");
        status = -1;
        goto err_exit;
    }
    nh = (uint8_t)c;

    len = ((uint16_t)nh << 8) |
          ((uint16_t)nl << 0);

    if (len > BSL430_MAX_PAYLOADSIZE) {
        log("** Wrong N. %u\n", len);
        status = -1;
        goto err_exit;
    }

    /* Response */
    for (i = 0; i < len; i++) {
        c = bsl430_uart_readb(CHAR_TIMEOUT);
        if (c == -1) {
            log("** Response data timeout. %d\n", i);
            status = -1;
            goto err_exit;
        }
        frame->payload[i] = (uint8_t)c;
    }

    /* CKL CKH */
    c = bsl430_uart_readb(CHAR_TIMEOUT);
    if (c == -1) {
        log("** CKL timeout.\n");
        status = -1;
        goto err_exit;
    }
    ckl = (uint8_t)c;

    c = bsl430_uart_readb(CHAR_TIMEOUT);
    if (c == -1) {
        log("** CKH timeout.\n");
        status = -1;
        goto err_exit;
    }
    ckh = (uint8_t)c;

    cks = ((uint16_t)ckh << 8) |
          ((uint16_t)ckl << 0);

    frame->fcs = bsl430_crc16(frame->payload, len, INITFCS);
    if (frame->fcs != cks) {
        log("** CKS error.\n");
        status = -1;
        goto err_exit;
    }

    frame->len = len;

    return 0;

err_exit:
    /*
     * Clear all subsequence characters for frame SYNC recovery.
     */
    mdelay(RESP_TIMEOUT);
    bsl430_uart_clear();

    return status;
}
