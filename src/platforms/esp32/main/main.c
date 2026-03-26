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

/* Provides main entry point.  Initialise subsystems and enter GDB
 * protocol loop.
 */

#include "general.h"
#include "gdb_if.h"
#include "gdb_main.h"
#include "target.h"
#include "exception.h"
#include "gdb_packet.h"
#include "morse.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "platform.h"

#include "command.h"
#ifdef ENABLE_RTT
#include "rtt.h"
#endif

//unsigned short gdb_port = 4242; //same as stlink st-util GDB

static void bmp_poll_loop(void)
{
	SET_IDLE_STATE(false);
	while (gdb_target_running && cur_target) {
		gdb_poll_target();

		// Check again, as `gdb_poll_target()` may
		// alter these variables.
		if (!gdb_target_running || !cur_target)
			break;
		char c = gdb_if_getchar_to(0);
		if (c == '\x03' || c == '\x04')
			target_halt_request(cur_target);
#ifdef ENABLE_RTT
		else if (rtt_enabled)
			poll_rtt(cur_target);
#endif
		platform_pace_poll();
#ifdef ENABLE_RTT
		poll_rtt_host();
#endif
#ifdef PLATFORM_HAS_TRACESWO
		poll_swo_host();
#endif
        //vTaskDelay(pdMS_TO_TICKS(1));
	}

	SET_IDLE_STATE(true);
	const gdb_packet_s *const packet = gdb_packet_receive();
	// If port closed and target detached, stay idle
	if (packet->data[0] != '\x04' || cur_target) {
		SET_IDLE_STATE(false);
	}
	gdb_main(packet);
}

#if CONFIG_BMDA == 1
int main(int argc, char **argv)
{
	platform_init(argc, argv);
#else
int esp32_main(void)
{

#endif
#ifdef ENABLE_RTT
	rtt_if_init();
#endif
#ifdef PLATFORM_HAS_TRACESWO
	swo_if_init();
#endif

	platform_max_frequency_set(AEL_BMP_DEFAULT_FREQUENCY_HZ);

	while (true) {
		TRY (EXCEPTION_ALL) {
			bmp_poll_loop();
		}
		CATCH () {
		default:
			gdb_put_packet_error(0xffU);
			target_list_free();
			gdb_outf("Uncaught exception: %s\n", exception_frame.msg);
			morse("TARGET LOST.", true);
		}
#if CONFIG_BMDA == 1
		if (shutdown_bmda)
			break;
#endif
        //vTaskDelay(pdMS_TO_TICKS(1));
	}

	target_list_free();
	return 0;
}
// extern 
//void set_gdb_socket(int socket);
//void set_gdb_listen(int socket);

void gdb_application_thread(void *pvParameters)
{
    esp32_main();
}
