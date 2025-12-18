#include "general.h"
#include "platform.h"
#include "jtagtap.h"
#include "spi_common.h"

#include "spi_common.h"

/* Rename the original init function so we can wrap it */
#define jtagtap_init gpio_jtagtap_init
#include "../../../platforms/common/jtagtap.c"
#undef jtagtap_init

/* SPI FPGA JTAG Stubs */
static void spi_jtag_tms_seq(uint32_t tms, size_t bits)
{
	(void)tms;
	(void)bits;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (JTAG_TMS_SEQ), Bit Count (bits)
	 *    - Payload: TMS Data (tms)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the packet via SPI.
	 * 4. Deselect the FPGA SPI device (CS high).
	 *
	 * Waveform:
	 * TCK:      \__/¯¯\__/¯¯\__/¯¯\__
	 * TMS:      <bit0><bit1><bit2>...
	 *           ^     ^
	 *           |     |
	 *           |     Target samples on Rising Edge
	 *           Host drives on Falling Edge
	 * TDI:      (Keep Low or Last State)
	 */
}

static void spi_jtag_tdi_tdo_seq(uint8_t *data_out, bool final_tms, const uint8_t *data_in, size_t bits)
{
	(void)data_out;
	(void)final_tms;
	(void)data_in;
	(void)bits;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (JTAG_SCAN_IO), Bit Count (bits), Final TMS (final_tms)
	 *    - Payload: TDI Data (data_in)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the packet via SPI.
	 * 4. Receive TDO Data from FPGA.
	 * 5. Deselect the FPGA SPI device (CS high).
	 * 6. Store received TDO Data to *data_out.
	 *
	 * Waveform:
	 * TCK:      \__/¯¯\__/¯¯...¯¯\__/¯¯\__
	 * TMS:      <Low> <Low> ...<Low> <final_tms>
	 * TDI:      <bit0><bit1>...<bitN>
	 * TDO:      <bit0><bit1>...<bitN>
	 *           ^     ^
	 *           |     |
	 *           |     Target samples TDI/TMS on Rising Edge
	 *           Target drives TDO on Falling Edge
	 */
}

static void spi_jtag_tdi_seq(bool final_tms, const uint8_t *data_in, size_t bits)
{
	(void)final_tms;
	(void)data_in;
	(void)bits;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (JTAG_SCAN_OUT), Bit Count (bits), Final TMS (final_tms)
	 *    - Payload: TDI Data (data_in)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the packet via SPI.
	 * 4. Deselect the FPGA SPI device (CS high).
	 *
	 * Waveform:
	 * TCK:      \__/¯¯\__/¯¯...¯¯\__/¯¯\__
	 * TMS:      <Low> <Low> ...<Low> <final_tms>
	 * TDI:      <bit0><bit1>...<bitN>
	 *           ^     ^
	 *           |     |
	 *           |     Target samples TDI/TMS on Rising Edge
	 */
}

static bool spi_jtag_next(bool tms, bool tdi)
{
	(void)tms;
	(void)tdi;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (JTAG_CYCLE), Cycles (1)
	 *    - Payload: TMS (tms), TDI (tdi)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the packet via SPI.
	 * 4. Receive TDO bit (if needed, usually returned in status).
	 * 5. Deselect the FPGA SPI device (CS high).
	 *
	 * Waveform:
	 * TCK:      \__/¯¯\__
	 * TMS:      <tms>
	 * TDI:      <tdi>
	 * TDO:      <bit>
	 */
	return false;
}

static void spi_jtag_reset(void)
{
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (JTAG_RESET)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the packet via SPI.
	 * 4. Deselect the FPGA SPI device (CS high).
	 *
	 * Waveform:
	 * TCK:      \__/¯¯\__/¯¯\__/¯¯\__/¯¯\__/¯¯\__ (At least 5 clocks)
	 * TMS:      <High><High><High><High><High>
	 */
}

static void spi_jtag_cycle(bool tms, bool tdi, size_t cycles)
{
	(void)tms;
	(void)tdi;
	(void)cycles;
	/* 
	 * Pseudo Code:
	 * 1. Construct the SPI packet:
	 *    - Header: Command (JTAG_CYCLE), Cycles (cycles)
	 *    - Payload: TMS (tms), TDI (tdi)
	 * 2. Select the FPGA SPI device (CS low).
	 * 3. Transmit the packet via SPI.
	 * 4. Deselect the FPGA SPI device (CS high).
	 *
	 * Waveform:
	 * TCK:      \__/¯¯...¯¯\__/¯¯\__
	 * TMS:      <tms> ... <tms>
	 * TDI:      <tdi> ... <tdi>
	 */
}

void spi_jtag_init(void)
{
	jtag_proc.jtagtap_reset = spi_jtag_reset;
	jtag_proc.jtagtap_next = spi_jtag_next;
	jtag_proc.jtagtap_tms_seq = spi_jtag_tms_seq;
	jtag_proc.jtagtap_tdi_tdo_seq = spi_jtag_tdi_tdo_seq;
	jtag_proc.jtagtap_tdi_seq = spi_jtag_tdi_seq;
	jtag_proc.jtagtap_cycle = spi_jtag_cycle;
	jtag_proc.tap_idle_cycles = 1;
}

/* Wrapper init function */
void jtagtap_init(void)
{
	if (spi_or_gpio) {
		spi_jtag_init();
	} else {
		gpio_jtagtap_init();
	}
}
