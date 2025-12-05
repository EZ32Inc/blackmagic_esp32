#include "general.h"
#include "platform.h"
#include "swd.h"
#include "spi_common.h"

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
	return false;
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
