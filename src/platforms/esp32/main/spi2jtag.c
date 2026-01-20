#include "general.h"
#include "platform.h"
#include "jtagtap.h"
#include "spi_common.h"

#include "spi_common.h"

extern bool SPI_nGPIO;

/* Rename the original init function so we can wrap it */
#define jtagtap_init gpio_jtagtap_init
#define jtag_proc gpio_jtag_proc
#include "../../../platforms/common/jtagtap.c"
#undef jtagtap_init
#undef jtag_proc

jtag_proc_s jtag_proc;

/* SPI FPGA JTAG Commands (Placeholders) */
#define JTAG_CMD_TMS_SEQ      0x50
#define JTAG_CMD_SCAN_IO      0x51
#define JTAG_CMD_SCAN_OUT     0x52
#define JTAG_CMD_CYCLE        0x53
#define JTAG_CMD_RESET        0x54

#include "spi_fpga.h"
#include <string.h>
#include "esp_log.h"

static uint8_t current_tms = 0;
static uint8_t current_tdi = 0;
//static uint8_t current_tck = 0;

//tdi_out set to tdi&1, tms_out set to tms&1, generate bits of tck_out
//each spi clk will generate one tck and with same tms and tdi
static void spi_jtag_tmstdi_seq(uint8_t tms, uint8_t tdi, size_t bits)
{
    PLATFORM_JTAG_YIELD(); // Prevent watchdog timeout
	if (bits == 0) return;

	uint8_t tx[68];
	uint8_t rx[68];
	uint8_t tms_tdi = ((tms & 1) << 1) | (tdi & 1);
	uint8_t data_out = 0;
	uint32_t i = 0;
	
	// Construct the byte with 4 sets of (TMS, TDI)
	for (i = 0; i < 4; ++i) {
		data_out <<= 2;
		data_out |= tms_tdi;
	}

	size_t bits_left = bits;

    i=0;
	while (bits_left >= 256) {
		tx[i++] = 255; // counter (256 - 1)

#if 0 //def TEST_TMS_TDI
		tx[i++] = 0x99;
		tx[i++] = 0x66;
		tx[i++] = 0x66;
		tx[i++] = 0x99;
		tx[i++] = 0x66;
		tx[i++] = 0x99;
#endif
        
		for (uint32_t j = i; j < 66; ++j) {
			tx[j] = data_out;
		}
		spi_device2_transfer_data(tx, rx, 66);
		bits_left -= 256;
	}

	if (bits_left == 0)
    {
        //spi_device2_transfer_data(tx, rx, 1);
		return;
    }

    i=0;
	tx[i++] = bits_left - 1; // counter - 1
	
#if 0 //def TEST_TMS_TDI
    //not correct for bits_left <24
    if(bits_left>24){
        tx[i++] = 0x99;
        tx[i++] = 0x66;
        tx[i++] = 0x66;
        tx[i++] = 0x99;
        tx[i++] = 0x66;
        tx[i++] = 0x99;
    }
#endif

    // Calculate full bytes needed (4 bits per byte)
    size_t full_bytes = (bits_left+3) / 4;
    if(bits_left % 4 == 0){
        full_bytes ++;
    }

    for (; i < full_bytes+1; ++i) {
		tx[i] = data_out;
	}
	
    spi_device2_transfer_data(tx, rx, i);
    //printf("spi_jtag_tmstdi_seq(): bits=%d full_bytes=%d\n", bits, full_bytes);
}

//tdi_out set to 1, tms_out set to tms&1, generate bits of tck_out
//Mimic jtagtap_tms_seq()
static void spi_jtag_tms_seq(uint32_t tms, size_t bits)
{
    PLATFORM_JTAG_YIELD(); // Prevent watchdog timeout
	if (bits == 0) return;

    uint8_t tx[16]; // Sufficient for 32 bits (needs ~1+8 bytes)
    uint8_t rx[16];
    
    tx[0] = bits - 1;
    size_t num_bytes = (bits + 3) / 4;
    
    for (size_t i = 0; i < num_bytes; ++i) {
        uint8_t val = 0;
        for (int j = 0; j < 4; ++j) {
            size_t bit_idx = i * 4 + j;
            if (bit_idx < bits) {
                uint8_t tms_bit = (tms >> bit_idx) & 1;
                uint8_t tdi_bit = 1;
                uint8_t pair = (tms_bit << 1) | tdi_bit;
                val |= (pair << (6 - 2 * j));
            }
        }
        tx[1 + i] = val;
    }
    
    if(bits % 4 ==0){
        num_bytes++;
    }

    // Send data
    spi_device2_transfer_data(tx, rx, 1 + num_bytes);
    
    // Update global state
    current_tdi = 1;
    current_tms = (tms >> (bits - 1)) & 1;
}

static void spi_jtag_tdi_tdo_seq(uint8_t *data_out, bool final_tms, const uint8_t *data_in, size_t bits)
{
    if (bits == 0) return;

    uint8_t tx[68];
    uint8_t rx[68];
    size_t bits_left = bits;
    size_t current_bit_index = 0;

    while (bits_left > 0) {
        size_t chunk_bits = (bits_left > 256) ? 256 : bits_left;
        
        uint32_t i = 0;
        tx[i++] = chunk_bits - 1;

        // Calculate number of data bytes needed (4 cycles per byte)
        size_t num_data_bytes = (chunk_bits + 3) / 4;
        
        // Fill data bytes
        for (size_t byte_idx = 0; byte_idx < num_data_bytes; ++byte_idx) {
            uint8_t byte_val = 0;
            for (int cycle = 0; cycle < 4; ++cycle) {
                size_t bit_in_chunk = byte_idx * 4 + cycle;
                if (bit_in_chunk >= chunk_bits) {
                    break; 
                }
                
                size_t total_bit_pos = current_bit_index + bit_in_chunk;
                
                // Determine TMS
                bool tms = false;
                if (final_tms && (total_bit_pos == bits - 1)) {
                    tms = true;
                }
                
                // Determine TDI
                bool tdi = false;
                if (data_in) {
                    if (data_in[total_bit_pos / 8] & (1 << (total_bit_pos % 8))) {
                        tdi = true;
                    }
                }
                
                // Pack into byte: Cycle 0 at Bits 7:6, Cycle 3 at Bits 1:0
                // TMS is Bit 1, TDI is Bit 0 of the pair.
                uint8_t tms_tdi = ((tms ? 1 : 0) << 1) | (tdi ? 1 : 0);
                byte_val |= (tms_tdi << (6 - (cycle * 2)));
            }
            tx[i++] = byte_val;
        }
        
        // Add one extra byte as seen in spi_jtag_tmstdi_seq
        //if(bits>=4){
        if(bits % 4 == 0){
            tx[i++] = 0;
        }
        
        spi_device2_transfer_data(tx, rx, i);
        
        // Process RX to extract TDO
        if (data_out) {
            for (size_t byte_idx = 0; byte_idx < num_data_bytes; ++byte_idx) {
                uint8_t rx_byte = rx[1 + byte_idx];
                for (int cycle = 0; cycle < 4; ++cycle) {
                    size_t bit_in_chunk = byte_idx * 4 + cycle;
                    if (bit_in_chunk >= chunk_bits) break;
                    
                    size_t total_bit_pos = current_bit_index + bit_in_chunk;
                    
                    // Extract TDO from Bit 0 of the pair
                    uint8_t pair = (rx_byte >> (6 - (cycle * 2))) & 0x3;
                    bool tdo = (pair & 1); 
                    
                    if (tdo) {
                        data_out[total_bit_pos / 8] |= (1 << (total_bit_pos % 8));
                    } else {
                        data_out[total_bit_pos / 8] &= ~(1 << (total_bit_pos % 8));
                    }
                }
            }
        }
        
        bits_left -= chunk_bits;
        current_bit_index += chunk_bits;
    }
    
    // Update global state
    current_tms = final_tms ? 1 : 0;
    if (data_in) {
        current_tdi = (data_in[(bits - 1) / 8] & (1 << ((bits - 1) % 8))) ? 1 : 0;
    //} else {
    //    current_tdi = 0;
    }
}

//mimic jtagtap_tdi_seq()
static void spi_jtag_tdi_seq(bool final_tms, const uint8_t *data_in, size_t bits)
{
    spi_jtag_tdi_tdo_seq(NULL, final_tms, data_in, bits);
}

static bool spi_jtag_next(bool tms, bool tdi)
{
    uint8_t data_in = tdi ? 1 : 0;
    uint8_t data_out = 0;
    spi_jtag_tdi_tdo_seq(&data_out, tms, &data_in, 1);
    //printf("spi_jtag_next: data_out=0x%x\n", data_out);
    return (data_out & 1) ? true : false;
}

static void spi_jtag_reset(void)
{
#if 0 //def TRST_PORT
    //TODO to eb added
	if (platform_hwversion() == 0) {
		gpio_clear(TRST_PORT, TRST_PIN);
		for (volatile size_t i = 0; i < 10000U; i++)
			continue;
		gpio_set(TRST_PORT, TRST_PIN);
	}
#endif
	jtagtap_soft_reset();
}

static void spi_jtag_cycle(bool tms, bool tdi, size_t cycles)
{

	if (cycles == 0) return;

    current_tdi = tdi ? 1 : 0;
    current_tms = tms ? 1 : 0;

    spi_jtag_tmstdi_seq(current_tms, current_tdi, cycles);
}

//extern esp_err_t set_cfga(bool use_portc, bool use_porta, bool njtag_swdio, bool swd_gpio);

void spi_jtag_init(void)
{
	//platform_target_clk_output_enable(true);
	//TMS_SET_MODE();

    ESP_LOGI("spi2jtag", "To do spi_jtag_init()");
    //SPI_nGPIO = true;
    //set_cfga(true, false, false, false);

	jtag_proc.jtagtap_reset = spi_jtag_reset;
	jtag_proc.jtagtap_next = spi_jtag_next;
	jtag_proc.jtagtap_tms_seq = spi_jtag_tms_seq;
	jtag_proc.jtagtap_tdi_tdo_seq = spi_jtag_tdi_tdo_seq;
	jtag_proc.jtagtap_tdi_seq = spi_jtag_tdi_seq;
	jtag_proc.jtagtap_cycle = spi_jtag_cycle;
	jtag_proc.tap_idle_cycles = 1;

	/* Ensure we're in JTAG mode. Start by issuing a complete SWD reset of at least 50 reset cycles */
    //vTaskDelay(30 / portTICK_PERIOD_MS);
	spi_jtag_cycle(true, false, 51U);
	/* Having achieved reset, try the deprecated 16-bit SWD-to-JTAG sequence */
	spi_jtag_tms_seq(ADIV5_SWD_TO_JTAG_SELECT_SEQUENCE, 16U);
	/* Next, to complete that sequence, do a full 50+ cycle reset again */
	spi_jtag_cycle(true, false, 51U);
	/*
	 * For parts that implement the old sequence, we're done.. however, for parts that do not, we
	 * now need to do SWD-to-Dormant-State
	 */
	spi_jtag_tms_seq(ADIV5_SWD_TO_DORMANT_SEQUENCE, 16U);
	/* Having achieved this state, we now have to signal we want to change states with the alert sequence */
	spi_jtag_tms_seq(0xffU, 8U); /* 8 reset cycles used to ensure the target's in a happy place */
	/* 128-bit Selection Alert sequence */
	spi_jtag_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_0, 32U);
	spi_jtag_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_1, 32U);
	spi_jtag_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_2, 32U);
	spi_jtag_tms_seq(ADIV5_SELECTION_ALERT_SEQUENCE_3, 32U);
	/*
	 * Now ask for JTAG please
	 * We combine the last two sequences in a single spi_jtag_tms_seq as an optimization
	 *
	 * Send 4 SWCLKTCK cycles with SWDIOTMS LOW
	 * Send the required 8 bit activation code sequence on SWDIOTMS
	 *
	 * The bits are shifted out to the right, so we shift the second sequence left by the size of the first sequence
	 * The first sequence is 4 bits and the second 8 bits, totaling 12 bits in the combined sequence
	 */
	spi_jtag_tms_seq(ADIV5_ACTIVATION_CODE_ARM_JTAG_DP << 4U, 12U);
	/* At this point we are definitely in JTAG mode - let the scan logic reset the state machine into a good state. */
}

/* Wrapper init function */
void jtagtap_init(void)
{
	if (SPI_nGPIO) {
		spi_jtag_init();
	} else {
		gpio_jtagtap_init();
		jtag_proc = gpio_jtag_proc;
	}
}

void test_spi_jtag_tmstdi_seq(void)
{
    ESP_LOGI("test_spi", "Starting spi_jtag_tmstdi_seq hardware test");
#if 0
    // Test 1: 1 bit, TMS=1, TDI=0
    // Should send 1 byte (header=0) + 1 byte data
    ESP_LOGI("test_spi", "Test 1: 1 bit, TMS=1, TDI=0");
    spi_jtag_tmstdi_seq(1, 0, 1);

    // Test 2: 4 bits, TMS=0, TDI=1
    // Should send 1 byte (header=3) + 1 byte data
    ESP_LOGI("test_spi", "Test 2: 4 bits, TMS=0, TDI=1");
    spi_jtag_tmstdi_seq(0, 1, 4);

    // Test 3: 5 bits, TMS=1, TDI=1
    // Should send 1 byte (header=4) + 2 bytes data
    ESP_LOGI("test_spi", "Test 3: 5 bits, TMS=1, TDI=1");
    spi_jtag_tmstdi_seq(1, 1, 5);

    // Test 4.0: 8 bits, TMS=0, TDI=0
    // Should send 1 byte (header=7) + 2 bytes data
    ESP_LOGI("test_spi", "Test 4.0: 8 bits, TMS=0, TDI=0");
    spi_jtag_tmstdi_seq(0, 0, 8);

    ESP_LOGI("test_spi", "Test 4.1: 32 bits, TMS=0, TDI=0");
    spi_jtag_tmstdi_seq(0, 0, 32);

    ESP_LOGI("test_spi", "Test 4.2: 35 bits, TMS=0, TDI=0");
    spi_jtag_tmstdi_seq(0, 0, 35);

    ESP_LOGI("test_spi", "Test 4.3: 64 bits, TMS=0, TDI=0");
    spi_jtag_tmstdi_seq(0, 0, 64);
#endif

    ESP_LOGI("test_spi", "Test 4.4: 256 bits, TMS=1, TDI=0");
    spi_jtag_tmstdi_seq(1, 0, 256);

    ESP_LOGI("test_spi", "Test 4.5: 122 bits, TMS=0, TDI=1");
    spi_jtag_tmstdi_seq(0, 1, 122);

    ESP_LOGI("test_spi", "Test 4.6: 131 bits, TMS=0, TDI=1");
    spi_jtag_tmstdi_seq(1, 1, 131);

    ESP_LOGI("test_spi", "Test 4.7: 119 bits, TMS=0, TDI=1");
    spi_jtag_tmstdi_seq(0, 1, 119);

    ESP_LOGI("test_spi", "Test 4.8: 139 bits, TMS=0, TDI=1");
    spi_jtag_tmstdi_seq(1, 0, 139);

    ESP_LOGI("test_spi", "Test 4.9: 229 bits, TMS=0, TDI=1");
    spi_jtag_tmstdi_seq(0, 1, 229);

    // Test 5: 256 bits, TMS=1, TDI=0
    // Should send 1 byte (header=255) + 64 bytes data
    ESP_LOGI("test_spi", "Test 5: 25 bits, TMS=1, TDI=0");
    spi_jtag_tmstdi_seq(1, 0, 25);

    // Test 6: 257 bits, TMS=0, TDI=1
    // Should send:
    // 1. 1 byte (header=255) + 64 bytes data
    // 2. 1 byte (header=0) + 1 byte data
    ESP_LOGI("test_spi", "Test 6: 257 bits, TMS=0, TDI=1");
    spi_jtag_tmstdi_seq(0, 1, 257);

    ESP_LOGI("test_spi", "Finished spi_jtag_tmstdi_seq hardware test");
}
