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
#include "general.h"
#include "gdb_if.h"
#include "version.h"

#include "gdb_packet.h"
#include "gdb_main.h"
#include "target.h"
#include "exception.h"
#include "gdb_packet.h"
#include "morse.h"
#include "timing.h"

#include <assert.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <esp_timer.h>

#include "platform.h"
//#include "esp/uart.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "espressif/esp_wifi.h"
//#include "ssid_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint32_t swd_delay_cnt = 0;


//#include <dhcpserver.h>

//#define ACCESS_POINT_MODE
#define AP_SSID	 "blackmagic"
#define AP_PSK	 "blackmagic"

//TODO: To cehck
uint32_t target_clk_divider = 0;

/* Values for STM32F103 at 72 MHz */
#define USED_SWD_CYCLES 22
#define CYCLES_PER_CNT 10
void platform_max_frequency_set(uint32_t freq)
{
	int rcc_ahb_frequency = 240*1024*1024;
	int divisor = rcc_ahb_frequency - USED_SWD_CYCLES * freq;
	if (divisor < 0) {
		swd_delay_cnt = 0;
		return;
	}
	divisor /= 2;
	swd_delay_cnt = divisor/(CYCLES_PER_CNT * freq);
	if ((swd_delay_cnt * (CYCLES_PER_CNT * freq)) < (unsigned int)divisor)
		swd_delay_cnt++;
}

uint32_t platform_max_frequency_get(void)
{
	int rcc_ahb_frequency = 240*1024*1024;
	uint32_t ret = rcc_ahb_frequency;
	ret /= USED_SWD_CYCLES + CYCLES_PER_CNT * swd_delay_cnt;
	return ret;
}



//  | (1<<MY_DEBUG_PIN)
#define GPIO_OUTPUT_PIN_SEL  ((1<<SWCLK_PIN) | (1<<SWDIO_PIN) | (1<<TMS_PIN) | (1<<TDI_PIN) | (1<<TDO_PIN) | (1<<TCK_PIN))

void pins_init() {

    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

void platform_init()//int argc, char **argv)
{


	pins_init();

	if (gdb_if_init() != 0) {
		// DEBUG_ERROR("Failed to initialize GDB interface\n");
	}
}

//TODO To be implemented
void platform_srst_set_val(bool assert)
{
	(void)assert;
}

bool platform_srst_get_val(void) { return false; }

const char *platform_target_voltage(void)
{
	return "3.3V";
}

uint32_t platform_time_ms(void)
{
	//return xTaskGetTickCount() / portTICK_PERIOD_MS;
	int64_t time_milli=esp_timer_get_time()/1000;
	return((uint32_t)time_milli);
}

#define vTaskDelayMs(ms)	vTaskDelay((ms)/portTICK_PERIOD_MS)

void platform_delay(uint32_t ms)
{
	vTaskDelayMs(ms);
}

int platform_hwversion(void)
{
	return 140;
}
#if 0
/* This is a transplanted main() from main.c */
void main_task(void *parameters)
{
	(void) parameters;

	platform_init();//0, NULL);

	while (true) {
#if 0 //TODO: TO be checked!
		volatile struct exception e;
		TRY_CATCH(e, EXCEPTION_ALL) {
			gdb_main();
		}
		if (e.type) {
			gdb_putpacketz("EFF");
			target_list_free();
			morse("TARGET LOST.", 1);
		}
#else
        //TODO
        //gdb_main(); 
        platform_delay(10);//
#endif
	}

	/* Should never get here */
}

void user_init(void)
{
	xTaskCreate(&main_task, "main", 4*1024, NULL, 2, NULL);
}
#endif

//TODO To be checked these 2 funcs
void platform_timeout_set(platform_timeout_s *t, uint32_t ms)
{
	t->time = platform_time_ms() + ms;
}

bool platform_timeout_is_expired(const platform_timeout_s *t)
{
	return platform_time_ms() > t->time;
}

void platform_nrst_set_val(bool assert)
{
	if (assert) {
		gpio_set_direction(NRST_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(NRST_PIN, 0);
	} else {
		gpio_set_direction(NRST_PIN, GPIO_MODE_INPUT);
		gpio_set_pull_mode(NRST_PIN, GPIO_PULLUP_ONLY);
	}
}

bool platform_nrst_get_val(void)
{
	return gpio_get_level(NRST_PIN) == 0;
}

void platform_target_clk_output_enable(bool enable)
{
	(void)enable;
}


/*
 * Write the bootloader flag and reboot.
 * The platform_init() will see this and reboot a second time into ST BootROM.
 * If BMPBootloader is enabled, then it will see this and initialize its DFU.
 */
void platform_request_boot(void)
{
	esp_restart();
}

#ifdef PLATFORM_HAS_POWER_SWITCH
bool platform_target_get_power(void)
{
	return true;
}

bool platform_target_set_power(const bool power)
{
	(void)power;
	return true;
}

/*
 * A dummy implementation of platform_target_voltage_sense as the
 * blackpill-f4 has no ability to sense the voltage on the power pin.
 * This function is only needed for implementations that allow the target
 * to be powered from the debug probe.
 */
uint32_t platform_target_voltage_sense(void)
{
	return 3300;
}
#endif

void platform_ospeed_update(const uint32_t frequency)
{
	(void)frequency;
}

bool platform_spi_init(const spi_bus_e bus)
{
	(void)bus;
	return true;
}

bool platform_spi_deinit(const spi_bus_e bus)
{
	(void)bus;
	return false;
}

bool platform_spi_chip_select(const uint8_t device_select)
{
	(void)device_select;
	return true;
}

uint8_t platform_spi_xfer(const spi_bus_e bus, const uint8_t value)
{
	(void)bus;
	return value;
}

const char *serial_no = "ESP32";
