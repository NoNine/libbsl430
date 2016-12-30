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

#ifndef __BSL430_PROGRAM_H__
#define __BSL430_PROGRAM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct titxt_header_s {
    uint32_t segments;
} titxt_header_t;

typedef struct titxt_segment_s {
    uint32_t address;
    uint32_t size;
    uint8_t  data[0];
} titxt_segment_t;


int bsl430_parse_ti_txt(uint8_t *txt, uint32_t size, uint8_t *buf, uint32_t bufsize);
int bsl430_program(titxt_header_t *header);

#ifdef __cplusplus
}
#endif

#endif  /* __BSL430_PROGRAM_H__ */
