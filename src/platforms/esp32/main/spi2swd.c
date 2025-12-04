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
	/* TODO: Implement SPI SWD sequence out */
}

static uint32_t spi_swd_seq_in(size_t bits)
{
	(void)bits;
	/* TODO: Implement SPI SWD sequence in */
	return 0;
}

static bool spi_swd_seq_in_parity(uint32_t *parity_data, size_t bits)
{
	(void)parity_data;
	(void)bits;
	/* TODO: Implement SPI SWD sequence in with parity */
	return false;
}

static void spi_swd_seq_out_parity(uint32_t data, size_t bits)
{
	(void)data;
	(void)bits;
	/* TODO: Implement SPI SWD sequence out with parity */
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
