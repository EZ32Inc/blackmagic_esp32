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
#include "rtt_if.h"
#include "target.h"

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <esp_log.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define RTT_PORT 9090
#define RTT_RX_BUFFER_SIZE 1024

static const char *TAG = "RTT_IF";
static int rtt_server_socket = -1;
static int rtt_client_socket = -1;

// Simple ring buffer for RX (Host -> Target)
static uint8_t rtt_rx_buffer[RTT_RX_BUFFER_SIZE];
static size_t rtt_rx_head = 0;
static size_t rtt_rx_tail = 0;

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

	ESP_LOGI(TAG, "RTT Server listening on port %d", port);
	return sock;
}

int rtt_if_init(void)
{
	rtt_server_socket = setup_socket(RTT_PORT);
	return (rtt_server_socket >= 0) ? 0 : -1;
}

int rtt_if_exit(void)
{
	if (rtt_client_socket >= 0) {
		close(rtt_client_socket);
		rtt_client_socket = -1;
	}
	if (rtt_server_socket >= 0) {
		close(rtt_server_socket);
		rtt_server_socket = -1;
	}
	return 0;
}

static void handle_new_connection(void)
{
	struct sockaddr_in source_addr;
	socklen_t addr_len = sizeof(source_addr);
	int sock = accept(rtt_server_socket, (struct sockaddr *)&source_addr, &addr_len);
	
	if (sock >= 0) {
		if (rtt_client_socket >= 0) {
			ESP_LOGW(TAG, "New RTT connection, closing old one");
			close(rtt_client_socket);
		}
		rtt_client_socket = sock;
		
		// Set non-blocking
		int flags = fcntl(rtt_client_socket, F_GETFL, 0);
		fcntl(rtt_client_socket, F_SETFL, flags | O_NONBLOCK);
		
		ESP_LOGI(TAG, "RTT Client connected");
		
		// Reset RX buffer
		rtt_rx_head = 0;
		rtt_rx_tail = 0;
	}
}

/* Called from rtt.c to send data to the host (Target -> Host) */
uint32_t rtt_write(const uint32_t channel, const char *buf, uint32_t len)
{
	(void)channel; // We only support one channel (or mix them) for now
	
	if (rtt_client_socket < 0)
		return len; // Pretend we sent it to avoid blocking target

	ssize_t sent = send(rtt_client_socket, buf, len, 0);
	if (sent < 0) {
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			return 0;
		} else {
			ESP_LOGE(TAG, "RTT send failed: errno %d", errno);
			close(rtt_client_socket);
			rtt_client_socket = -1;
			return 0;
		}
	}
	return (uint32_t)sent;
}

/* Called from rtt.c to get data from host (Host -> Target) */
int32_t rtt_getchar(const uint32_t channel)
{
	(void)channel;
	
	if (rtt_rx_head == rtt_rx_tail)
		return -1; // Empty
		
	uint8_t c = rtt_rx_buffer[rtt_rx_tail];
	rtt_rx_tail = (rtt_rx_tail + 1) % RTT_RX_BUFFER_SIZE;
	return c;
}

/* Called from rtt.c to check if data is available */
bool rtt_nodata(const uint32_t channel)
{
	(void)channel;
	return (rtt_rx_head == rtt_rx_tail);
}

/* Called from main loop to poll for data from host and new connections */
void poll_rtt_host(void)
{
	if (rtt_server_socket < 0)
		return;

	// Check for new connections
	handle_new_connection();

	// Check for incoming data from client
	if (rtt_client_socket >= 0) {
		// Try to fill the buffer
		size_t space_available;
		if (rtt_rx_head >= rtt_rx_tail)
			space_available = RTT_RX_BUFFER_SIZE - 1 - (rtt_rx_head - rtt_rx_tail);
		else
			space_available = rtt_rx_tail - rtt_rx_head - 1;

		if (space_available > 0) {
			// We can read contiguous bytes up to the end of the buffer or the tail
			size_t contiguous_space = (rtt_rx_head >= rtt_rx_tail) ? (RTT_RX_BUFFER_SIZE - rtt_rx_head) : (rtt_rx_tail - rtt_rx_head - 1);
			// Wrap around handling is simpler if we just read one chunk at a time
			
			ssize_t len = recv(rtt_client_socket, &rtt_rx_buffer[rtt_rx_head], contiguous_space, 0);
			if (len > 0) {
				rtt_rx_head = (rtt_rx_head + len) % RTT_RX_BUFFER_SIZE;
			} else if (len == 0) {
				ESP_LOGI(TAG, "RTT Client disconnected");
				close(rtt_client_socket);
				rtt_client_socket = -1;
			} else {
				if (errno != EWOULDBLOCK && errno != EAGAIN) {
					ESP_LOGE(TAG, "RTT recv failed: errno %d", errno);
					close(rtt_client_socket);
					rtt_client_socket = -1;
				}
			}
		}
	}
}
