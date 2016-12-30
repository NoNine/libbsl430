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
#define LOG_TAG "bsl430-program"

#include <stdlib.h>
#include <string.h>

#include "bsl430-platform.h"
#include "bsl430.h"
#include "bsl430-program.h"


#define ALIGN(x,a)  __ALIGN_MASK((x),(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

#define TITXT_SEGMENT_ALIGN 8

int bsl430_program(titxt_header_t *header)
{
    int status = 0;
    uint8_t password[] = {
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" \
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" \
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" \
        "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
    };
    uint32_t version = 0;
    uint32_t i = 0;
    titxt_segment_t *segment = NULL;
    uint16_t crc0, crc1;

    bsl430_enter(1);

    status = bsl430_cmd_change_baudrate(115200);
    if (status != 0) {
        log("** Change baudrate failed.\n");
        goto error0;
    }

    status = bsl430_cmd_rx_password(password, 32);
    if (status == BSL430_MSG_PASSWD_ERROR) {
        log("** Password Error! All code FRAM is erased!\n");
        bsl430_cmd_rx_password(password, 32);
    }

    bsl430_cmd_tx_version(&version);
    log("BSL Version: %08X\n", version);

    /* Write code segment and verify. */
    for (i = 0; i < header->segments; i++) {
        crc0 = crc1 = 0;

        if (i == 0) {
            segment = (titxt_segment_t *)((uint8_t *)header + sizeof(titxt_header_t));
        } else {
            segment = (titxt_segment_t *)((uint8_t *)segment +
                                          sizeof(titxt_segment_t) +
                                          ALIGN(segment->size, TITXT_SEGMENT_ALIGN));
        }

        crc0 = bsl430_crc16(segment->data, segment->size, 0xFFFF);

        log("<<< Segment: @%04X %u Bytes, Crc %04X >>>\n", segment->address, segment->size, crc0);

        status = bsl430_cmd_rx_data_block(segment->address, segment->data, segment->size);
        if (status != 0) {
            log("** Programing failed! 0x%02X\n", (uint8_t)status);
            break;
        }

        log("\n");

        status = bsl430_cmd_crc_check(segment->address, segment->size, &crc1);
        if (status != 0) {
            log("** Checking CRC failed!\n");
            break;
        }

        if (crc0 != crc1) {
            log("** CRC mismatch! 0x%04X 0x%04X\n", crc0, crc1);
            status = 1;
            break;
        }
    }

    log("BSL programming %s.\n\n", (status == 0)? "SUCC": "FAIL");

error0:
    bsl430_exit();

    return status;
}

int bsl430_parse_ti_txt(uint8_t *txt, uint32_t size, uint8_t *buf, uint32_t bufsize)
{
    char *txt_copy = NULL;
    char *line = NULL;
    char *next = NULL;
    char *arg = NULL;
    uint32_t segments = 0;
    titxt_header_t *header = NULL;
    titxt_segment_t *segment = NULL;
    uint8_t *data = buf;

    if (txt == NULL || size == 0) {
        return -1;
    }

    if (buf == NULL || bufsize == 0) {
        return -1;
    }

    txt_copy = malloc(size + 1);
    if (txt_copy == NULL) {
        return -1;
    }

    /* It's convenient to parse the whole file and each line as strings. */
    memmove(txt_copy, txt, size);
    txt_copy[size] = '\0';

    line = txt_copy;
    next = txt_copy;

    header = (titxt_header_t *)data;
    data += sizeof(titxt_header_t);

    while (*next) {
        if (*next == '\r' || *next == '\n') {
            *next = '\0';
            /* Parse non-empty line. */
            if (*line) {
                /* printf("line: %s\n", line); */
                if (*line == '@') {
                    /* Start of new segment. */
                    if (segment == NULL) {
                        segment = (titxt_segment_t *)data;
                    } else {
                        data += (sizeof(titxt_segment_t) + ALIGN(segment->size, TITXT_SEGMENT_ALIGN));
                        segment = (titxt_segment_t *)data;
                    }

                    if ((void *)segment >= (void *)(buf + bufsize)) {
                        log("The TI-TXT file is too big!\n");
                        free(txt_copy);
                        return -1;
                    }

                    segment->address = strtoul(line + 1, NULL, 16);
                    segment->size = 0;

                    segments++;
                } else if (*line == 'q') {
                    /* End of TI-TXT file. */
                    header->segments = segments;
                    break;
                } else {
                    /* Data line. Please refer to parse_line(). */
                    while (*line) {
                        while (*line == ' ' || *line == '\t') {
                            line++;
                        }

                        if (*line == '\0') {
                            break;
                        }

                        arg = line;

                        while (*line && (*line != ' ' && *line != '\t')) {
                            line++;
                        }

                        if (*line) {
                            *line++ = '\0';
                        }

                        if ((void *)&segment->data[segment->size] >= (void *)(buf + bufsize)) {
                            log("The TI-TXT file is too big!\n");
                            free(txt_copy);
                            return -1;
                        }

                        segment->data[segment->size++] = (uint8_t)strtoul(arg, NULL, 16);
                    }
                }
            }
            line = next + 1;
        }
        next++;
    }

    free(txt_copy);
    return 0;
}
