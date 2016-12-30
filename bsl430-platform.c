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
#define LOG_TAG "bsl430-platform"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <termios.h>

#include "hi_board.h"
#include "hi_unf_gpio.h"

#include "bsl430-platform.h"

#define PMRPC_UART_PORT "/dev/ttyAMA2"

static int fd = -1;

static int uart_set_speed(int fd, int speed);
static int uart_set_attribute(int fd, int databits, int stopbits, char parity);

int bsl430_uart_init(int baudrate, int parity)
{
    int status = 0;

    fd = open(PMRPC_UART_PORT, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        log("Open UART failed! %s\n", strerror(errno));
        return -1;
    }

    status  = uart_set_speed(fd, baudrate);
    status |= uart_set_attribute(fd, 8, 1, (parity == 0)? 'E': (parity == 1)? 'O': 'N');

    if (status != 0) {
        log("Config UART failed!\n");
        close(fd);
        fd = -1;
    }

    return status;
}

int bsl430_uart_term(void)
{
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    return 0;
}

int bsl430_uart_readb(uint16_t timeout)
{
    int status = 0;
    uint8_t c;

    (void)timeout;

    if (fd < 0) {
        return -1;
    }

    status = read(fd, &c, 1);
    if (status <= 0 && errno != 0) {
        log("Read UART error! %s\n", strerror(errno));
    }

    return (status == 1)? c: -1;
}

int bsl430_uart_writeb(uint8_t c)
{
    int status = 0;

    if (fd < 0) {
        return -1;
    }

    /*
     * The FIFO depth of MSP430 UART is ONE.
     * So add some delay between two bytes to avoid overflow.
     */
    usleep(200);

    status = write(fd, &c, 1);
    if (status <= 0) {
        log("Write UART error! %s\n", strerror(errno));
    }

    return (status == 1)? 0: -1;
}

int bsl430_uart_clear(void)
{
    while (bsl430_uart_readb(10) != -1) ;
    return 0;
}

int bsl430_gpio_init(void)
{
    HI_SYS_Init();
    HI_UNF_GPIO_Init();

    HI_UNF_GPIO_SetDirBit(HI_BOARD_RST_GPIONUM, HI_BOARD_GPIO_OUT);
    HI_UNF_GPIO_SetDirBit(HI_BOARD_TST_GPIONUM, HI_BOARD_GPIO_OUT);

    return 0;
}

int bsl430_gpio_term(void)
{
    return 0;
}

int bsl430_gpio_rst(int level)
{
    return HI_UNF_GPIO_WriteBit(HI_BOARD_RST_GPIONUM,
                                (level == 0)? HI_BOARD_GPIO_LOW: HI_BOARD_GPIO_HIGH);
}

int bsl430_gpio_tst(int level)
{
    return HI_UNF_GPIO_WriteBit(HI_BOARD_TST_GPIONUM,
                                (level == 0)? HI_BOARD_GPIO_LOW: HI_BOARD_GPIO_HIGH);
}

static int uart_set_speed(int fd, int speed)
{
    uint32_t i;
    struct termios option;

    /* baud speed reference table */
    int speed_arr[] = {B230400, B115200, B57600, B38400, B19200, B9600,
        B4800, B2400, B1200, B600, B300, B110};
    int name_arr[]  = {230400,  115200,  57600,  38400,  19200,  9600,
        4800,  2400,  1200,  600,  300,  110};

    if (fd < 0) {
        return -1;
    }

    /* 0 means default setting 115200 */
    if (!speed) {
        speed = 115200;
    }

    if (tcgetattr(fd, &option)) {
        log("tcgetattr\n");
        return -1;
    }

    for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
        if (speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&option, speed_arr[i]);
            cfsetospeed(&option, speed_arr[i]);
            if (tcsetattr(fd, TCSANOW, &option)) {
                log("tcsetattr\n");
                return -1;
            }

            return 0;
        }
    }

    /* unsupport baud speed */
    log("only support speed of 230400 115200 57600 38400 19200 "
          "9600 4800 2400 1200 600 300 110 but [%d]\n", speed);
    return -1;
}

static int uart_set_attribute(int fd, int databits, int stopbits, char parity)
{
    struct termios option;

    if (fd < 0) {
        return -1;
    }

    /* get old attribute */
    if (tcgetattr(fd, &option)) {
        log("tcgetattr\n");
        return -1;
    }

    /* set general attribute */
    option.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    option.c_oflag &= ~OPOST; /*Output*/
    option.c_iflag &= ~(ICRNL | IXON); /* ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON) */

    option.c_cflag &= ~CSIZE;
    /* set data bits */
    switch (databits) {
    case 7:
        option.c_cflag |= CS7;
        break;
    case 0: /* default setting */
    case 8:
        option.c_cflag |= CS8;
        break;
    default:
        log("only support data size of 7 or 8\n");
        return -1;
    }

    /* set parity attribute */
    switch (parity) {
    case 0: /* default setting */
    case 'n':
    case 'N':
        option.c_cflag &= ~PARENB;   /* Clear parity enable */
        option.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O':
        option.c_cflag |= (PARODD | PARENB);  /* Enable odd parity */
        option.c_iflag |= INPCK;             /* enable parity checking */
        break;
    case 'e':
    case 'E':
        option.c_cflag |= PARENB;
        option.c_cflag &= ~PARODD;   /* Enable even parity */
        option.c_iflag |= INPCK;       /* enable parity checking */
        break;
    default:
        log("only support parity of n/N o/O e/E\n");
        return -1;
    }

    /* set stop bits */
    switch (stopbits) {
    case 0: /* default setting */
    case 1:
        option.c_cflag &= ~CSTOPB;
        break;
    case 2:
        option.c_cflag |= CSTOPB;
        break;
    default:
        log("only support stop bits of 1 or 2\n");
        return -1;
    }

    option.c_cc[VTIME] = 1; /* time out value(100ms) */
    option.c_cc[VMIN] = 0; /* each char */

    tcflush(fd, TCIOFLUSH); /* update the option and do it now */
    if (tcsetattr(fd, TCSANOW, &option) != 0) {
        log("tcsetattr\n");
        return -1;
    }

    return 0;
}
