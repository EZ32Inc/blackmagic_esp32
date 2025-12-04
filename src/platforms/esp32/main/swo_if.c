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
#include "target.h"

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <esp_log.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define SWO_PORT 9091

static const char *TAG = "SWO_IF";
static int swo_server_socket = -1;
static int swo_client_socket = -1;

static int setup_socket(uint16_t port)
{
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		return -1;
	}

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in dest_addr;
	dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);

	if (bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
		ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
		close(sock);
		return -1;
	}

	if (listen(sock, 1) != 0) {
		ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
		close(sock);
		return -1;
	}

	// Set non-blocking
	int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);

	ESP_LOGI(TAG, "SWO Server listening on port %d", port);
	return sock;
}

int swo_if_init(void)
{
	swo_server_socket = setup_socket(SWO_PORT);
	return (swo_server_socket >= 0) ? 0 : -1;
}

static void handle_new_connection(void)
{
	struct sockaddr_in source_addr;
	socklen_t addr_len = sizeof(source_addr);
	int sock = accept(swo_server_socket, (struct sockaddr *)&source_addr, &addr_len);
	
	if (sock >= 0) {
		if (swo_client_socket >= 0) {
			ESP_LOGW(TAG, "New SWO connection, closing old one");
			close(swo_client_socket);
		}
		swo_client_socket = sock;
		
		// Set non-blocking
		int flags = fcntl(swo_client_socket, F_GETFL, 0);
		fcntl(swo_client_socket, F_SETFL, flags | O_NONBLOCK);
		
		ESP_LOGI(TAG, "SWO Client connected");
	}
}

void swo_send_data(const uint8_t *buf, size_t len)
{
	if (swo_client_socket < 0)
		return;

	ssize_t sent = send(swo_client_socket, buf, len, 0);
	if (sent < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
			ESP_LOGE(TAG, "SWO send failed: errno %d", errno);
			close(swo_client_socket);
			swo_client_socket = -1;
		}
	}
}

/* Called from main loop to poll for new connections */
void poll_swo_host(void)
{
	if (swo_server_socket < 0)
		return;

	// Check for new connections
	handle_new_connection();
	
	// Also poll for UART data and send it
	extern void poll_swo_capture(void);
	poll_swo_capture();
}
