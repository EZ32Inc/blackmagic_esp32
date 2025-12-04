/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2023  Black Sphere Technologies Ltd.
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
#include "platform.h"
#include "swo.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define SWO_UART_NUM UART_NUM_1
#define SWO_BUF_SIZE 1024

static const char *TAG = "SWO_UART";
static bool swo_uart_initialized = false;

// Forward declaration
void swo_send_data(const uint8_t *buf, size_t len);

void swo_uart_init(uint32_t baudrate)
{
	if (swo_uart_initialized) {
		uart_set_baudrate(SWO_UART_NUM, baudrate);
		return;
	}

	uart_config_t uart_config = {
		.baud_rate = baudrate,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};
	
	ESP_LOGI(TAG, "Initializing SWO UART on pin %d at %ld baud", TRACESWO_PIN, baudrate);

	ESP_ERROR_CHECK(uart_driver_install(SWO_UART_NUM, SWO_BUF_SIZE * 2, 0, 0, NULL, 0));
	ESP_ERROR_CHECK(uart_param_config(SWO_UART_NUM, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(SWO_UART_NUM, TRACESWO_DUMMY_TX, TRACESWO_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	swo_uart_initialized = true;
}

void swo_uart_deinit(void)
{
	if (swo_uart_initialized) {
		uart_driver_delete(SWO_UART_NUM);
		swo_uart_initialized = false;
	}
}

uint32_t swo_uart_get_baudrate(void)
{
	uint32_t baudrate;
	uart_get_baudrate(SWO_UART_NUM, &baudrate);
	return baudrate;
}

void poll_swo_capture(void)
{
	if (!swo_uart_initialized)
		return;

	uint8_t data[128];
	int length = 0;
	ESP_ERROR_CHECK(uart_get_buffered_data_len(SWO_UART_NUM, (size_t*)&length));
	
	if (length > 0) {
		length = uart_read_bytes(SWO_UART_NUM, data, sizeof(data), 0);
		if (length > 0) {
			swo_send_data(data, length);
		}
	}
}

// Stubs for Manchester encoding (not supported yet)
void swo_manchester_init(void) {}
void swo_manchester_deinit(void) {}

void swo_init(swo_coding_e swo_mode, uint32_t baudrate, uint32_t itm_stream_bitmask)
{
	(void)itm_stream_bitmask;
	if (swo_mode == swo_nrz_uart) {
		swo_uart_init(baudrate);
	}
}

void swo_deinit(bool deallocate)
{
	(void)deallocate;
	swo_uart_deinit();
}
