/**
 * \file timing_alt.h
 *
 * \brief Portable interface to timeouts and to the CPU cycle counter
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#ifndef MBEDTLS_TIMING_ALT_H
#define MBEDTLS_TIMING_ALT_H

#include "mbedtls/private_access.h"

#include "mbedtls/build_info.h"

#include <stdint.h>


#if defined(MBEDTLS_TIMING_ALT)

/**
 * \brief          timer structure
 */
struct mbedtls_timing_hr_time
{
    uint64_t MBEDTLS_PRIVATE(timer_ms);
};

/**
 * \brief          Context for mbedtls_timing_set/get_delay()
 */
typedef struct mbedtls_timing_delay_context
{
    struct mbedtls_timing_hr_time   MBEDTLS_PRIVATE(timer);
    uint32_t                        MBEDTLS_PRIVATE(int_ms);
    uint32_t                        MBEDTLS_PRIVATE(fin_ms);
} mbedtls_timing_delay_context;

#endif

#endif /* timing_alt.h */
