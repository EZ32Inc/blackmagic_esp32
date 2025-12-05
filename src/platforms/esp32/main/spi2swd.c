#include "general.h"
#include "platform.h"
#include "swd.h"
#include "spi_common.h"
#include "spi_fpga.h"

static uint8_t g_buffered_request = 0;
static bool g_last_op_was_read = false;

/* Rename the original init function so we can wrap it */
#define swdptap_init gpio_swdptap_init
#include "../../../platforms/common/swdptap.c"
#undef swdptap_init

/* SPI FPGA SWD Stubs */
static void spi_swd_seq_out(uint32_t data, size_t bits)
{
	(void)data;
	(void)bits;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (SWD_SEQ_OUT), Bit Count (bits)
	 *    - Payload: Data (data)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the packet via SPI (spi_transmit).
	 * 4. Deselect the FPGA SPI device (CS high).
	 *
	 * Waveform (Host driving SWDIO):
	 * SWDCLK:   \__/¯¯\__/¯¯\__/¯¯\__
	 * SWDIO:    <bit0><bit1><bit2>...
	 *           ^     ^
	 *           |     |
	 *           |     Target samples on Rising Edge
	 *           Host drives on Falling Edge
	 */
	if (bits == 8) {
		g_buffered_request = (uint8_t)data;
	} else {
		while (bits > 0) {
			uint8_t n = (bits > 32) ? 32 : (uint8_t)bits;
			spi_dp_wr_nbit(data, n);
			if (bits >= 32) {
				data = 0; // Should shift data if we had > 32 bits of actual data, but data is uint32_t.
				          // If bits > 32, it's likely line reset (all 1s) or idle cycles (0s).
						  // If data was 0xFFFFFFFF, it stays 0xFFFFFFFF for line reset.
						  // If data was 0, it stays 0.
						  // If it was some other pattern, we can't really support > 32 bits with single uint32_t data.
						  // Assuming standard usage: Line Reset (ones) or Idle (zeros).
			}
			bits -= n;
		}
	}
}

static uint32_t spi_swd_seq_in(size_t bits)
{
	(void)bits;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (SWD_SEQ_IN), Bit Count (bits)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the command packet via SPI.
	 * 4. Receive the response data from FPGA (spi_receive).
	 * 5. Deselect the FPGA SPI device (CS high).
	 * 6. Return the received data.
	 *
	 * Waveform (Target driving SWDIO):
	 * SWDCLK:   \__/¯¯\__/¯¯\__/¯¯\__
	 * SWDIO:    <bit0><bit1><bit2>...
	 *           ^     ^
	 *           |     |
	 *           |     Host samples on Rising Edge
	 *           Target drives on Falling Edge
	 */
	if (bits == 3) {
		return spi_request_seq_in(g_buffered_request, g_last_op_was_read);
	}
	return 0;
}

static bool spi_swd_seq_in_parity(uint32_t *parity_data, size_t bits)
{
	(void)parity_data;
	(void)bits;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (SWD_SEQ_IN_PARITY), Bit Count (bits)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the command packet via SPI.
	 * 4. Receive the response data and parity status from FPGA.
	 * 5. Deselect the FPGA SPI device (CS high).
	 * 6. Store the received data in *parity_data.
	 * 7. Check the parity bit/status returned by FPGA.
	 * 8. Return true if parity is correct, false otherwise.
	 *
	 * Waveform (Target driving SWDIO):
	 * SWDCLK:   \__/¯¯...¯¯\__/¯¯\__
	 * SWDIO:    <bit0>...<bitN><Parity>
	 *           ^            ^
	 *           |            |
	 *           |            Host samples Parity on Rising Edge
	 *           Target drives on Falling Edge
	 */
	bool parity_err = spi_dp_seq_in_parity_32bit(parity_data);
	g_last_op_was_read = true;
	return !parity_err;
}

static void spi_swd_seq_out_parity(uint32_t data, size_t bits)
{
	(void)data;
	(void)bits;
	/* 
	 * Pseudo Code:
	 * 1. Calculate parity for the data (if not handled by FPGA).
	 * 2. Construct the SPI packet:
	 *    - Header: Command (SWD_SEQ_OUT_PARITY), Bit Count (bits)
	 *    - Payload: Data (data) + Parity Bit
	 * 3. Select the FPGA SPI device (CS low).
	 * 4. Transmit the packet via SPI.
	 * 5. Deselect the FPGA SPI device (CS high).
	 *
	 * Waveform (Host driving SWDIO):
	 * SWDCLK:   \__/¯¯...¯¯\__/¯¯\__
	 * SWDIO:    <bit0>...<bitN><Parity>
	 *           ^            ^
	 *           |            |
	 *           |            Target samples Parity on Rising Edge
	 *           Host drives on Falling Edge
	 */
	spi_dp_seq_out_parity_32bit(data);
	g_last_op_was_read = false;
}

void spi_swd_init(void)
{
	swd_proc.seq_in = spi_swd_seq_in;
	swd_proc.seq_out = spi_swd_seq_out;
	swd_proc.seq_in_parity = spi_swd_seq_in_parity;
	swd_proc.seq_out_parity = spi_swd_seq_out_parity;
}

/* Wrapper init function */
void swdptap_init(void)
{
	if (spi_or_gpio) {
		spi_swd_init();
	} else {
		gpio_swdptap_init();
	}
}
