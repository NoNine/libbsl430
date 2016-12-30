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

#define LOG_NDEBUG 0
#define LOG_TAG "bsl430_test"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bsl430-program.h"

#define PROGRAM_NAME "bsl430_test"
#define VERSION "$Revision 1.00 $"

#define log(...)    printf(LOG_TAG ": " __VA_ARGS__)

#define BSL430_MAX_FW_SIZE  (64 * 1024)
/* 16KB is enough for MSP430FR2633. */
#define BSL430_MAX_CODE_SIZE    (32 * 1024)

static uint8_t txt_buf[BSL430_MAX_FW_SIZE];
static uint8_t segments_buf[BSL430_MAX_CODE_SIZE];

static void bsl430_test_version(void);
static void bsl430_test_help(void);

static int bsl430_test_program(const char *filename);

int main(int argc, char** argv)
{
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        bsl430_test_help();
    }

    return bsl430_test_program(argv[1]);
}

static void bsl430_test_version(void)
{
    printf("--------------------------------------------------\n");
    printf("| " VERSION PROGRAM_NAME " (" __DATE__ " " __TIME__ ")\n");
    printf("| libbsl430 test code.\n");
    printf("| Dec 2016, No.9\n");
    printf("--------------------------------------------------\n");
    return;
}

static void bsl430_test_help(void)
{
    bsl430_test_version();

    printf(
"Usage: " PROGRAM_NAME " <TI-TXT File>\n"
"\n"
"libbsl430 test code.\n"
"      --help                 show help.\n");

    exit(EXIT_SUCCESS);
}

static int bsl430_test_program(const char *filename)
{
    int status = 0;
    int fd = -1;
    uint8_t *buf = NULL;
    off_t size = 0;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        log("Openning file error (%s). %s\n", filename, strerror(errno));
        return -1;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        log("Getting file size error. %s\n", strerror(errno));
        goto error0;
    }

    log("File size: %u Byte.\n", (uint32_t)size);
    if (size > BSL430_MAX_FW_SIZE) {
        log("FW size error.\n");
        goto error0;
    }

    lseek(fd, 0, SEEK_SET);

    memset(txt_buf, 0, sizeof(txt_buf));
    memset(segments_buf, 0, sizeof(segments_buf));

    if (read(fd, txt_buf, (size_t)size) != (ssize_t)size) {
        log("Reading file error. %s\n", strerror(errno));
        goto error0;
    }

    /* Parse the TI-TXT image and program. */
    status = bsl430_parse_ti_txt(txt_buf, (uint32_t)size, segments_buf, sizeof(segments_buf));
    if (status == 0) {
        status = bsl430_program((titxt_header_t *)segments_buf);
    } else {
        log("Parsing TI-TXT file error.\n");
    }

    close(fd);

    return status;

error0:
    close(fd);
    return -1;
}
