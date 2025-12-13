#include "general.h"
#include "platform.h"
#include "swd.h"
#include "spi_common.h"
#include "spi_fpga.h"

#include "esp_log.h"

bool spi_or_gpio = false ; //true;

static uint8_t g_buffered_request = 0;
static bool g_last_op_was_read = false;

/* Rename the original init function so we can wrap it */
#define swdptap_init gpio_swdptap_init
#include "../../../platforms/common/swdptap.c"
#undef swdptap_init

static swdio_status_t current_dir = SWDIO_STATUS_DRIVE;

/* SPI FPGA SWD Stubs */
static void spi_swd_seq_out(uint32_t data_in, size_t nbit)
{
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
    if (nbit>32 || nbit==0){
       ESP_LOGW("spi_swd_seq_out", "Error input nbit, nbit>32 || nbit==0: %d",nbit);
       return;
    }

    uint8_t len = nbit;
    if(current_dir == SWDIO_STATUS_FLOAT){
        len++;//and start TRN bit
    }

    uint8_t tx[8];
    uint8_t rx[8];
    uint8_t i=0, j=0;
    uint32_t data = reverse_bits32(data_in); //to send out data from LSB to MSB

    uint8_t carry = 0;
    if (current_dir == SWDIO_STATUS_FLOAT) {
        if (nbit == 32) {
            carry = (data & 1) << 7;
        }
        data = data >> 1;
    }

    ESP_LOGD("spi_dp_wr_nbit", "data_in=0x%08lx n=%d first bit is TRN=%s",data_in, nbit, current_dir == SWDIO_STATUS_FLOAT ? "true": "false");
    tx[i++] = ((len-1) | FLAG_DIO_WR) & (~FLAG_NORMAL_46B);
    while(j<len){
        tx[i] = data>>24;
        if (current_dir == SWDIO_STATUS_FLOAT && j == 32) {
             tx[i] |= carry;
        }
        i++;
        data = data<<8;
        j += 8;
    }
    spi_device2_transfer_data(tx,rx,i);

    current_dir = SWDIO_STATUS_DRIVE;
    return;
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

void test_spi_swd(void)
{
    ESP_LOGI("test_spi_swd", "Starting SPI SWD Tests...");

    // Test 1: nbit=32, Drive
    ESP_LOGI("test_spi_swd", "Test 1: nbit=32, Drive");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out(0xFFFFFFFF, 32);

    // Test 2: nbit=32, Float
    ESP_LOGI("test_spi_swd", "Test 2: nbit=32, Float");
    current_dir = SWDIO_STATUS_FLOAT;
    spi_swd_seq_out(0xFFFFFFFF, 32);

    // Test 3: nbit=21, Drive
    ESP_LOGI("test_spi_swd", "Test 3: nbit=21, Drive");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out(0x001FFFFF, 21);

    // Test 4: nbit=21, Float
    ESP_LOGI("test_spi_swd", "Test 4: nbit=21, Float");
    current_dir = SWDIO_STATUS_FLOAT;
    spi_swd_seq_out(0x001FFFFF, 21);

    ESP_LOGI("test_spi_swd", "Tests Completed.");
}
