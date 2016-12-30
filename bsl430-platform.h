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

#ifndef __BSL430_PLATFORM_H__
#define __BSL430_PLATFORM_H__

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BSL430_LOG_CONSOLE
#include <stdio.h>

#define log(...)    printf(LOG_TAG ": " __VA_ARGS__)
#if LOG_NDEBUG
#define debug(...)  do { if (0) { printf(LOG_TAG ": " __VA_ARGS__); } } while (0)
#else
#define debug(...)  printf(LOG_TAG ": " __VA_ARGS__)
#endif

#else
#include <utils/Log.h>

#define log(...)    ALOGI(__VA_ARGS__)
#define debug(...)  ALOGV(__VA_ARGS__)

#endif

#define mdelay(a)   usleep((a) * 1000)

int bsl430_uart_init(int baudrate, int parity);
int bsl430_uart_term(void);
int bsl430_uart_readb(uint16_t timeout);
int bsl430_uart_writeb(uint8_t c);
int bsl430_uart_clear(void);

int bsl430_gpio_init(void);
int bsl430_gpio_term(void);
int bsl430_gpio_rst(int level);
int bsl430_gpio_tst(int level);

#ifdef __cplusplus
}
#endif

#endif  /* __BSL430_PLATFORM_H__ */
