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

static void spi_swd_seq_out(uint32_t data_in, size_t nbit)
{
    if (nbit>32 || nbit==0){
       ESP_LOGW("spi_swd_seq_out", "Error input nbit, nbit>32 || nbit==0: %d",nbit);
       return;
    }

    uint8_t len = nbit;
    if(current_dir == SWDIO_STATUS_FLOAT){
        len++;//+ start TRN bit
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

static void spi_swd_seq_out_parity(uint32_t data_in, size_t nbit)
{
    if (nbit > 32 || nbit == 0) {
        ESP_LOGW("spi_swd_seq_out_parity", "Error input nbit, nbit>32 || nbit==0: %d", nbit);
        return;
    }

    uint8_t tx[8] = {0};
    uint8_t rx[8];
    uint8_t i = 0;

    // Mask data_in to ensure we only count bits within nbit
    if (nbit < 32) {
        data_in &= (1UL << nbit) - 1;
    }

    uint8_t parity = __builtin_popcount(data_in) & 1;

    uint8_t total_bits = nbit + 1; // data + parity
    if (current_dir == SWDIO_STATUS_FLOAT) {
        total_bits++; // + TRN
    }

    ESP_LOGD("spi_swd_seq_out_parity", "data_in=0x%08lx %d bits", data_in, total_bits);

    // Header byte
    tx[i++] = ((total_bits - 1) | FLAG_DIO_WR) & (~FLAG_NORMAL_46B);

    int bit_pos = 0;
    int byte_pos = i;
    tx[byte_pos] = 0; // Initialize first data byte

    // 1. TRN (if needed)
    if (current_dir == SWDIO_STATUS_FLOAT) {
        // TRN is 0. Just advance.
        bit_pos++;
    }

    // 2. Data
    for (size_t b = 0; b < nbit; b++) {
        if ((data_in >> b) & 1) {
            tx[byte_pos] |= (1 << (7 - bit_pos));
        }
        bit_pos++;
        if (bit_pos == 8) {
            bit_pos = 0;
            byte_pos++;
            tx[byte_pos] = 0;
        }
    }

    // 3. Parity
    if (parity) {
        tx[byte_pos] |= (1 << (7 - bit_pos));
    }
    bit_pos++;
    if (bit_pos == 8) {
        bit_pos = 0;
        byte_pos++;
    }

    int len_bytes = byte_pos + (bit_pos > 0 ? 1 : 0);

    spi_device2_transfer_data(tx, rx, len_bytes);

    current_dir = SWDIO_STATUS_DRIVE;
}
static uint32_t spi_swd_seq_in(size_t bits)
{
	if (bits == 3) {
		return spi_request_seq_in(g_buffered_request, g_last_op_was_read);
	}
	return 0;
}

static bool spi_swd_seq_in_parity(uint32_t *parity_data, size_t bits)
{
	bool parity_err = spi_dp_seq_in_parity_32bit(parity_data);
	g_last_op_was_read = true;
	return !parity_err;
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

void test_spi_swd_parity(void)
{
    ESP_LOGI("test_spi_swd_parity", "Starting SPI SWD Parity Tests...");

    // Test 1: nbit=32, Drive, Even Parity Data (0xFFFFFFFF has 32 ones -> even -> parity 0)
    // Wait, 0xFFFFFFFF has 32 ones. popcount(0xFFFFFFFF) = 32. 32 & 1 = 0.
    ESP_LOGI("test_spi_swd_parity", "Test 1: nbit=32, Drive, Data=0xFFFFFFFF (Parity 0)");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out_parity(0xFFFFFFFF, 32);

    // Test 2: nbit=32, Drive, Odd Parity Data (0xFFFFFFFE has 31 ones -> odd -> parity 1)
    ESP_LOGI("test_spi_swd_parity", "Test 2: nbit=32, Drive, Data=0xFFFFFFFE (Parity 1)");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out_parity(0xFFFFFFFE, 32);

    // Test 3: nbit=32, Float, Data=0xFFFFFFFF
    ESP_LOGI("test_spi_swd_parity", "Test 3: nbit=32, Float, Data=0xFFFFFFFF");
    current_dir = SWDIO_STATUS_FLOAT;
    spi_swd_seq_out_parity(0xFFFFFFFF, 32);

    // Test 4: nbit=4, Drive, Data=0xF (Parity 0)
    ESP_LOGI("test_spi_swd_parity", "Test 4: nbit=4, Drive, Data=0xF");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out_parity(0xF, 4);

    // Test 5: nbit=9, Drive, Data=0x155 (101010101 -> 5 ones -> parity 1)
    ESP_LOGI("test_spi_swd_parity", "Test 5: nbit=9, Drive, Data=0x155 (Parity 1)");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out_parity(0x155, 9);

    // Test 6: nbit=12, Float, Data=0xABC (1010 1011 1100 -> 6 ones -> parity 0)
    ESP_LOGI("test_spi_swd_parity", "Test 6: nbit=12, Float, Data=0xABC (Parity 0)");
    current_dir = SWDIO_STATUS_FLOAT;
    spi_swd_seq_out_parity(0xABC, 12);

    // Test 7: nbit=21, Drive, Data=0x1FFFFF (21 ones -> parity 1)
    ESP_LOGI("test_spi_swd_parity", "Test 7: nbit=21, Drive, Data=0x1FFFFF (Parity 1)");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out_parity(0x1FFFFF, 21);

    // Test 8: nbit=31, Float, Data=0x7FFFFFFF (31 ones -> parity 1)
    ESP_LOGI("test_spi_swd_parity", "Test 8: nbit=31, Float, Data=0x7FFFFFFF (Parity 1)");
    current_dir = SWDIO_STATUS_FLOAT;
    spi_swd_seq_out_parity(0x7FFFFFFF, 31);

    // Test 9: nbit=4, Drive, Data=0xF with garbage (0xFFFFFF1F -> 5 ones if unmasked, 4 ones if masked -> parity 0)
    // If unmasked: popcount(0xFFFFFF1F) = 29 (odd) -> parity 1.
    // If masked: popcount(0xF) = 4 (even) -> parity 0.
    ESP_LOGI("test_spi_swd_parity", "Test 9: nbit=4, Drive, Data=0xFFFFFF1F (Garbage bits, Parity 0)");
    current_dir = SWDIO_STATUS_DRIVE;
    spi_swd_seq_out_parity(0xFFFFFF1F, 4);

    ESP_LOGI("test_spi_swd_parity", "Tests Completed.");
}
