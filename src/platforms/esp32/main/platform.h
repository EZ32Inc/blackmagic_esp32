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

#define BOARD_IDENT             "ESP32 Black Magic Probe"
#define PLATFORM_IDENT          " (ESP32))"

#undef PRIx32
#define PRIx32 "x"

#undef SCNx32
#define SCNx32 "x"

#define NO_USB_PLEASE

#define SET_RUN_STATE(state)
#define SET_IDLE_STATE(state)
#define SET_ERROR_STATE(state)
#define DEBUG(x, ...) do { ; } while (0)
//#define DEBUG printf

#include "timing.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "../../../../../../main/common.h"

#include <freertos/FreeRTOS.h>

#define TMS_SET_MODE() do { } while (0)

#define TDO_PIN GPIO_NUM_2 //(15) 
#define TDI_PIN GPIO_NUM_3 //(4) 

//Not really used now!
#define TMS_PIN GPIO_NUM_46 //(6) 
#define TCK_PIN GPIO_NUM_47 //(5) 

//these two already defined in common.h
//#define SPI2JTAG_RESET_N     GPIO_NUM_42
//#define SPI2JTAG_GPIO39      GPIO_NUM_39

#undef PLATFORM_HAS_TRACESWO 
#define TRACESWO_PIN 13
// Workaround for driver
#define TRACESWO_DUMMY_TX 19

// ON ESP32 we dont have the PORTS, this is dummy value until code is corrected
#define SWCLK_PORT  0

#define SWDIO_PIN 2 // (15)
#define SWCLK_PIN 3 //(4)

// Allow debugging by toggling the pin
//#define MY_DEBUG_PIN (12)

#ifndef SPI2JTAG 
#define SPI2JTAG 1
#endif

extern uint32_t swd_delay_cnt;

extern void test_spi();

#ifdef SPI2JTAG
#define gpio_set_val(port, pin, value) do {	\
        ESP_LOGE("gpio_set_val", "ERROR: SPI2JTAG defined but we are doing gpio_set_level"); \
	} while (0);
#else
#define gpio_set_val(port, pin, value) do {	\
		gpio_set_level(pin, value);		\
		/*sdk_os_delay_us(2);	*/	\
	} while (0);
#endif

#define gpio_set(port, pin) gpio_set_val(port, pin, 1)
#define gpio_clear(port, pin) gpio_set_val(port, pin, 0)

#ifdef SPI2JTAG
static inline char gpio_get_a(unsigned char pin)
{
    ESP_LOGE("gpio_get", "ERROR: SPI2JTAG defined but we are doing gpio_get_level");
    //(void)port;  // prevent unused warning
    (void)pin;   // prevent unused warning
    return 0;
}
#define gpio_get(port, pin) gpio_get_a(pin)
#else
#define gpio_get(port, pin) gpio_get_level(pin)
#endif


// TODO https://esp-idf.readthedocs.io/en/v2.0/api/peripherals/gpio.html#_CPPv216gpio_pull_mode_t
// GPIO_FLOATING
// 		gpio_enable(SWDIO_PIN, GPIO_INPUT);	
#define SWDIO_MODE_FLOAT() do {			\
		gpio_set_direction(SWDIO_PIN, GPIO_MODE_INPUT);		\
		gpio_set_pull_mode(SWDIO_PIN, GPIO_FLOATING);		\
	} while (0)

 //gpio_enable(SWDIO_PIN, GPIO_OUTPUT);		

#define SWDIO_MODE_DRIVE() do {				\
           gpio_set_direction(SWDIO_PIN, GPIO_MODE_OUTPUT);		\
	} while (0)

//#define PLATFORM_HAS_DEBUG  1/
//#define ENABLE_DEBUG 1
#endif
