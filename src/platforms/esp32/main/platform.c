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

//#include "esp/uart.h"

//#include "task.h"
//#include "espressif/esp_wifi.h"
//#include "ssid_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"

#include "spi2jtag.h"

extern esp_err_t spi_master_init(void);

uint32_t swd_delay_cnt = 0;

//#include <dhcpserver.h>

//#define ACCESS_POINT_MODE
#define AP_SSID	 "blackmagic"
#define AP_PSK	 "blackmagic"



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

//#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<SWCLK_PIN) | (1ULL<<SWDIO_PIN) | (1ULL<<TMS_PIN) | (1ULL<<TDI_PIN) | (1ULL<<TDO_PIN) | (1ULL<<TCK_PIN))
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<PIN_RESET_N) | (1ULL<<SPI2JTAG_NJTAG_SWDIO) | (1ULL<<PIN_SPI_OR_GPIO))

void pins_init() {

    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
//        | (1ULL << PIN_RESET_N) | (1ULL << SPI2JTAG_NJTAG_SWDIO) | (1ULL << PIN_SPI_OR_GPIO) | (1ULL << JTAG_SEL_N_PIN);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //TODO: To remove this here and also in FPGA RTL code. Will use SPI only and not GPIO bitbang which is only for test purpose
    ESP_LOGI("ESP32_BMP", "Set PIN_SPI_OR_GPIO GPIO-%d to 1 for SPI and not GPIO bitbang", PIN_SPI_OR_GPIO);
    gpio_set_level(PIN_SPI_OR_GPIO, 1);//SPI and not GPIO bitbang

    ESP_LOGI("ESP32_BMP", "Set SPI2JTAG_NJTAG_SWDIO GPIO-%d to 1 to select SWDIO not JTAG", SPI2JTAG_NJTAG_SWDIO);
    gpio_set_level(SPI2JTAG_NJTAG_SWDIO, 1);

    //reset on-board FPGA
    ESP_LOGI("ESP32_BMP", "Use PIN_RESET_N pin-%d to reset on-board ICE40UP5k FPGA: Set 0, wait then set to 1", PIN_RESET_N);
    gpio_set_level(PIN_RESET_N, 0);
    for(int i =0;i<256;++i) {} //delay some time
    gpio_set_level(PIN_RESET_N, 1);

}
extern spi_device_handle_t gbl_spi_h1;
void platform_init()
{
	pins_init();

	//assert(gdb_if_init() == 0);
	//gdb_if_init();

#if 1
    esp_err_t ret_spi = ESP_OK;
    if(gbl_spi_h1 == NULL){
        ret_spi = spi_master_init();
        if (ret_spi != ESP_OK) {
            ESP_LOGE("ESP32_BMP", "SPI init failed: %s", esp_err_to_name(ret_spi));
            //return;
        }
    }
    else{
        ESP_LOGI("ESP32_BMP", "To run spi2jtag_test()");
        spi2jtag_test();
    }

#endif

}

void platform_srst_set_val(bool assert)
{
    (void)assert;
}

bool platform_srst_get_val(void) { return false; }

const char *platform_target_voltage(void)
{
	return "not supported";
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
	return 0;
}

/* This is a transplanted main() from main.c */
void main_task(void *parameters)
{
	(void) parameters;

	platform_init();

	while (true) {

		volatile struct exception e;
		TRY_CATCH(e, EXCEPTION_ALL) {
			gdb_main();
		}
		if (e.type) {
			gdb_putpacketz("EFF");
			target_list_free();
			morse("TARGET LOST.", 1);
		}
	}

	/* Should never get here */
}

void user_init(void)
{
	xTaskCreate(&main_task, "main", 4*1024, NULL, 2, NULL);
}



void platform_timeout_set(platform_timeout_s *t, uint32_t ms)
{
	t->time = platform_time_ms() + ms;
}

bool platform_timeout_is_expired(const platform_timeout_s *t)
{
	return platform_time_ms() > t->time;
}

//TODO

void platform_nrst_set_val(bool assert)
{
#if 0

	/* We reuse nTRST as nRST. */
	if (assert) {
		gpio_set_mode(TRST_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, TRST_PIN);
		/* Wait until requested value is active. */
		while (gpio_get(TRST_PORT, TRST_PIN))
			gpio_clear(TRST_PORT, TRST_PIN);
	} else {
		gpio_set_mode(TRST_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, TRST_PIN);
		/* Wait until requested value is active .*/
		while (!gpio_get(TRST_PORT, TRST_PIN))
			gpio_set(TRST_PORT, TRST_PIN);
	}
#endif
}

bool platform_nrst_get_val(void)
{
	//return gpio_get(TRST_PORT, TRST_PIN) == 0;
	return true;
}

void platform_target_clk_output_enable(bool enable)
{
	(void)enable;
}
