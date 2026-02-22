/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PLATFORM_H
#define __PLATFORM_H

//#define BOARD_IDENT             "ESP32 Black Magic Probe"
#define PLATFORM_IDENT          " (ESP32))"

#undef PRIx32
#define PRIx32 "lx"

#undef SCNx32
#define SCNx32 "lx"

#define NO_USB_PLEASE

#define SET_RUN_STATE(state)
#define SET_IDLE_STATE(state)
#define SET_ERROR_STATE(state)
#define DEBUG(x, ...) do { ; } while (0)
//#define DEBUG printf

#include "timing.h"
#include "driver/gpio.h"

#include <freertos/FreeRTOS.h>

#define TMS_SET_MODE() do { } while (0)
/*
P3:
IO PIN      I/O         JTAG        SWDIO           ESP32 GPIO
IO08        Input       TDO         Voltage in      GPIO15
IO09        Inout       TMS         SWDIO           GPIO41
IO10        Inout       TCK         SWCLK           GPIO47
IO11        Inout       TDI         NRST out        GPIO40  
*/
#define SWCLK_PIN (47)
#ifndef SWDIO_PIN
#define SWDIO_PIN (41)
#endif
#ifndef SWDIO_RDnWR_PIN
#define SWDIO_RDnWR_PIN (45)
#endif

#define TCK_PIN (SWCLK_PIN)
#define TMS_PIN (SWDIO_PIN)
#define TDI_PIN (40)
#define TDO_PIN (15)
//#define NRST_PIN (6)

#define PLATFORM_HAS_TRACESWO 1 
#define TRACESWO_PIN 13
// Workaround for driver
#define TRACESWO_DUMMY_TX 19

// ON ESP32 we dont have the PORTS, this is dummy value until code is corrected
//#define SWCLK_PORT  0

// Allow debugging by toggling the pin
//#define MY_DEBUG_PIN (12)

extern uint32_t swd_delay_cnt;

// Uncomment the following line to enable Option 2 (Fast GPIO via direct register writes)
#define ESP32S3_FASTGPIO 1

#ifdef ESP32S3_FASTGPIO
#include "soc/gpio_reg.h"

#define gpio_set(port, pin) do { \
    if ((pin) < 32) REG_WRITE(GPIO_OUT_W1TS_REG, (uint32_t)(1ULL << ((pin) & 31))); \
    else REG_WRITE(GPIO_OUT1_W1TS_REG, (uint32_t)(1ULL << (((pin) - 32) & 31))); \
} while (0)

#define gpio_clear(port, pin) do { \
    if ((pin) < 32) REG_WRITE(GPIO_OUT_W1TC_REG, (uint32_t)(1ULL << ((pin) & 31))); \
    else REG_WRITE(GPIO_OUT1_W1TC_REG, (uint32_t)(1ULL << (((pin) - 32) & 31))); \
} while (0)

#define gpio_get(port, pin) (((pin) < 32) ? (REG_READ(GPIO_IN_REG) >> ((pin) & 31)) & 1 : (REG_READ(GPIO_IN1_REG) >> (((pin) - 32) & 31)) & 1)

#define gpio_set_val(port, pin, value) do { \
    if (value) gpio_set(port, pin); \
    else gpio_clear(port, pin); \
} while(0)

#else // Option 1: Use ESP-IDF gpio_set_level

#define gpio_set_val(port, pin, value) do {	\
		gpio_set_level(pin, value);		\
	} while (0);

#define gpio_set(port, pin) gpio_set_val(port, pin, 1)
#define gpio_clear(port, pin) gpio_set_val(port, pin, 0)
#define gpio_get(port, pin) gpio_get_level(pin)

#endif

// TODO https://esp-idf.readthedocs.io/en/v2.0/api/peripherals/gpio.html#_CPPv216gpio_pull_mode_t
// GPIO_FLOATING
// 		gpio_enable(SWDIO_PIN, GPIO_INPUT);	
#define SWDIO_MODE_FLOAT() do {			\
		gpio_set_direction(SWDIO_PIN, GPIO_MODE_INPUT);		\
		gpio_set_pull_mode(SWDIO_PIN, GPIO_FLOATING);		\
        gpio_set_level(SWDIO_RDnWR_PIN, 1); \
	} while (0)

 //gpio_enable(SWDIO_PIN, GPIO_OUTPUT);		

#define SWDIO_MODE_DRIVE() do {				\
           gpio_set_direction(SWDIO_PIN, GPIO_MODE_OUTPUT);		\
           gpio_set_level(SWDIO_RDnWR_PIN, 0); \
	} while (0)

//#define PLATFORM_HAS_DEBUG  1/
//#define ENABLE_DEBUG 1
int rtt_if_init(void);
int rtt_if_exit(void);
void poll_rtt_host(void);

int swo_if_init(void);
void poll_swo_host(void);

#endif
