#include "general.h"
#include "platform.h"
#include "jtagtap.h"
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
	/* TODO: Implement SPI JTAG TMS sequence */
}

static void spi_jtag_tdi_tdo_seq(uint8_t *data_out, bool final_tms, const uint8_t *data_in, size_t bits)
{
	(void)data_out;
	(void)final_tms;
	(void)data_in;
	(void)bits;
	/* TODO: Implement SPI JTAG TDI/TDO sequence */
}

static void spi_jtag_tdi_seq(bool final_tms, const uint8_t *data_in, size_t bits)
{
	(void)final_tms;
	(void)data_in;
	(void)bits;
	/* TODO: Implement SPI JTAG TDI sequence */
}

static bool spi_jtag_next(bool tms, bool tdi)
{
	(void)tms;
	(void)tdi;
	/* TODO: Implement SPI JTAG next */
	return false;
}

static void spi_jtag_reset(void)
{
	/* TODO: Implement SPI JTAG reset */
}

static void spi_jtag_cycle(bool tms, bool tdi, size_t cycles)
{
	(void)tms;
	(void)tdi;
	(void)cycles;
	/* TODO: Implement SPI JTAG cycle */
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
