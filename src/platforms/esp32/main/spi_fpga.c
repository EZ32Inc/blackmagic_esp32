#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timing.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "freertos/semphr.h"

#include "general.h"
#include "spi_fpga.h"

#include "adiv5.h"
#include "../../../../main/esp32jtag_common.h"

//extern spi_device_handle_t gbl_spi_h1;

uint8_t reverse_bits8(uint8_t v)
{
    v = (v & 0xF0) >> 4 | (v & 0x0F) << 4;
    v = (v & 0xCC) >> 2 | (v & 0x33) << 2;
    v = (v & 0xAA) >> 1 | (v & 0x55) << 1;
    return v;
}

uint32_t reverse_bits32(uint32_t v)
{
    v = (v >> 16) | (v << 16);
    v = ((v & 0xFF00FF00U) >> 8) | ((v & 0x00FF00FFU) << 8);
    v = ((v & 0xF0F0F0F0U) >> 4) | ((v & 0x0F0F0F0FU) << 4);
    v = ((v & 0xCCCCCCCCU) >> 2) | ((v & 0x33333333U) << 2);
    v = ((v & 0xAAAAAAAAU) >> 1) | ((v & 0x55555555U) << 1);
    return v;
}

//uint8_t rx[8];
//Imagine rx[0] to rx[6] combined as a 56-bit series of long data, assume bit 0 is bit7 of rx[0] and bit 55 is bit 0 of rx[6].
//Skip bit 0 to 1
//skip bit 2 to bit 9 too
//Skip bit 10 which is TRN
//bit 11 to 13 is ack[0:2],  ack is type uint8_t.
//bit 14 to 45 is bit 0 to bit 31 of data, data is type uint32_t
//bit 46 is parity which is type bool
static void extract_ack_data_parity(const uint8_t rx[7], uint8_t *ack, uint32_t *data, bool *parity)
{
    uint64_t bits = 0;

    // Combine rx[0]..rx[6] into 56-bit value
    for (int i = 0; i < 7; i++) {
        bits = (bits << 8) | rx[i];
    }

    // Now bits[55:0] = bit stream
    // Bit numbering: 55 = first (rx[0] bit7), 0 = last (rx[6] bit0)

    // Extract ack (bits 11-13 => ack[0:2])
    *ack = ( (bits >> (56 - 14)) & 1 )        // bit 11 → ack[0]
         | ( (bits >> (56 - 13)) & 1 ) << 1   // bit 12 → ack[1]
         | ( (bits >> (56 - 12)) & 1 ) << 2;  // bit 13 → ack[2]

    // Extract data (bits 14-45 -> data[0:31])
    *data = (bits >> (56 - 46)) & 0xFFFFFFFF;
    *data = reverse_bits32(*data);

    // Extract parity (bit 46)
    *parity = (bits >> (56 - 47)) & 1;
}

//nbit<=32bit, wr nbit
void spi_dp_wr_nbit(uint32_t data_in, uint8_t nbit)
{
    if (nbit>32 || nbit==0){
       ESP_LOGE("SPI2JTAG_TEST", "Error input nbit, nbit>32 || nbit==0: %d",nbit);
       return;
    }

    uint8_t tx[8];
    uint8_t rx[8];
    uint8_t i=0, j=0;
    uint32_t data = reverse_bits32(data_in); //to send out data from LSB to MSB
    ESP_LOGD("spi_dp_wr_nbit", "data_in=0x%08lx n=%d",data_in,nbit);
    tx[i++] = ((nbit-1) | FLAG_DIO_WR) & (~FLAG_NORMAL_46B);
    while(j<nbit){
        tx[i++] = data>>24;
        data = data<<8;
        j += 8;
    }
    spi_device2_transfer_data(tx,rx,i);
}
void spi_dp_wr32bit(uint32_t data_in)
{
	//dp->seq_out(0xffffffffU, 32U);
	//dp->seq_out(0x0fffffffU, 32U);

    uint8_t tx[8];
    uint8_t rx[8];
    uint8_t i=0;
    uint32_t data = reverse_bits32(data_in); //to send out data from LSB to MSB
    ESP_LOGD("spi_dp_wr32bit", "data_in=0x%08lx", data_in);
    tx[i++] = (31 | FLAG_DIO_WR) & (~FLAG_NORMAL_46B);
    tx[i++] = data>>24;
    tx[i++] = data>>16;
    tx[i++] = data>>8;
    tx[i++] = data;
    spi_device2_transfer_data(tx,rx,i);
}
bool spi_dp_seq_in_parity_32bit(uint32_t *data)
{
    uint8_t tx[6];
    uint8_t rx[6];

    tx[0] = 33 & (~FLAG_NORMAL_46B); //34 bit and the very last biy bit 0 is TRN
    ESP_LOGD("seq_in_parity_32bit", "tx[0]=0x%02x", reverse_bits8(tx[0]));
    spi_device2_transfer_data(tx,rx,6);

    //extract data
    uint32_t bits=0;
    for (int i = 1; i < 5; i++) {
        bits = (bits << 8) | rx[i];
    }
    bits = reverse_bits32(bits);
    *data = bits;
    uint8_t parity = __builtin_popcount(bits)&1;
    bool check_parity = (parity != (rx[5]>>7));

    ESP_LOGD("spi_dp_seq_in_parity_32bit", "data=0x%08lx, parity=%d check_parity=%d ", 
            *data, parity, check_parity?1:0);

    return check_parity;
}

void spi_dp_seq_out_parity_32bit(uint32_t data_in)
{
    uint8_t tx[8];
    uint8_t rx[8];
    uint8_t i=0;
    uint32_t data = reverse_bits32(data_in); //to send out data from LSB to MSB
    ESP_LOGD("seq_out_parity_32bit", "data_in=0x%08lx", data_in);
    tx[i++] = (33 | FLAG_DIO_WR) & (~FLAG_NORMAL_46B); //34bit :TRN+32-bit data+Parity
    tx[i++] = data>>25; //BIT0 of SWD data is TRN bit, because last is a request and TRN needs to be following bit
    tx[i++] = data>>17;
    tx[i++] = data>>9;
    tx[i++] = data>>1;
    tx[i++] = ((data & 1)<<7) | ((__builtin_popcount(data) & 1)<<6); //last bit of data + parity
    spi_device2_transfer_data(tx,rx,i);
}
void spi_dp_line_reset()//adiv5_debug_port_s *dp)
{
	//dp->seq_out(0xffffffffU, 32U);
	//dp->seq_out(0x0fffffffU, 32U);
#if 1
    spi_dp_wr32bit(0xffffffff);
    spi_dp_wr32bit(0x0fffffff);
#else
    uint8_t tx[10];
    uint8_t rx[10];
    uint8_t i=0;
    ESP_LOGD("SPI2JTAG_TEST", "spi_dp_line_reset:generate 32 SWDCLKs and write");
    tx[i++] = (63 | FLAG_DIO_WR) & (~FLAG_NORMAL_46B);
    for(int i =0;i<8;++i)
        tx[i++] = 0xff;
    spi_device2_transfer_data(tx,rx,i);
#endif
}
bool spi_firmware_dp_low_write(/*adiv5_debug_port_s *dp, */const uint16_t addr, const uint32_t data_in)
{
    //(void)dp;
    uint32_t data = reverse_bits32(data_in);
	uint8_t request = reverse_bits8(make_packet_request(ADIV5_LOW_WRITE, addr));
    uint8_t tx[8];
    uint8_t rx[8];
    uint8_t i=0;
	//dp->seq_out(request, 8);
	//const uint8_t res = dp->seq_in(3);
	//dp->seq_out_parity(data, 32);
	//dp->seq_out(0, 8);
    ////printf("low_wr ACK=%d\n",res);

    ESP_LOGD("SPI2JTAG_TEST", "spi_firmware_dp_low_write() begin, generate 46 SWDCLKs + extra 8-bit=54 swd_clks");
    ESP_LOGD("SPI2JTAG_TEST", "request=0x%02x",request);
    i=0;

    uint8_t parity = __builtin_popcount(data) & 1U;
    //uint8_t tmp = FLAG_NORMAL_46B & (~FLAG_SWD_RD_NWR);
    uint8_t tmp = FLAG_NORMAL_46B;
    tx[i++] = (tmp & 0xc0) | (request>>2);
    tmp = ((request<<6) | EXTRA_8BIT_FLAG) & (~FLAG_SWD_EN_DISABLE); //Stop + Park + TRN + ACK[0:2] + TRN: Total 7-bit
    tmp |= (data>>31) & 0x1; //MSB of wr data added to LSB
    tx[i++] = tmp;
    tx[i++] = data>>23;
    tx[i++] = data>>15;
    tx[i++] = data>>7;
    tx[i++] = (data&0xfe) | parity;
    tx[i++] = 0;//extra 8 bit
    spi_device2_transfer_data(tx,rx,i);
    ESP_LOGD("spi_firmware_dp_low_write","tx=0x%02x %02x %02x %02x %02x %02x %02x",
            tx[0],tx[1], tx[2], tx[3], tx[4], tx[5], tx[6]);
    ESP_LOGD("spi_firmware_dp_low_write","rx=0x%02x %02x %02x %02x %02x %02x %02x",
            rx[0],rx[1], rx[2], rx[3], rx[4], rx[5], rx[6]);

    tmp = reverse_bits8(rx[1]<<3) & 0x7; 

	return tmp != SWD_ACK_OK;
}

uint32_t spi_firmware_dp_low_read(/*adiv5_debug_port_s *dp,*/ const uint16_t addr)
{
    /*
	const uint8_t request = make_packet_request(ADIV5_LOW_READ, addr);
	dp->seq_out(request, 8);
	const uint8_t res = dp->seq_in(3);
	uint32_t data = 0;
	dp->seq_in_parity(&data, 32);
	dp->seq_out(0, 8U);
    //printf("low_rd ACK=%d\n",res);
	return res == SWD_ACK_OK ? data : 0;
    */
    //(void)dp;
    uint32_t data = 0;
	uint8_t request = reverse_bits8(make_packet_request(ADIV5_LOW_READ, addr));
    uint8_t tx[8];
    uint8_t rx[8];
    uint8_t i=0;

    ESP_LOGD("spi_firmware_dp_low_read", "generate 46 SWDCLKs + extra 8-bit=54 swd_clks request=0x%02x", request);
    i=0;

    //uint8_t parity = __builtin_popcount(data) & 1U;
    //uint8_t tmp = FLAG_NORMAL_46B & (~FLAG_SWD_RD_NWR);
    uint8_t tmp = FLAG_NORMAL_46B;
    tx[i++] = (tmp & 0xc0) | (request>>2);
    tx[i++] = ((request<<6) | EXTRA_8BIT_FLAG) & (~FLAG_SWD_EN_DISABLE); //Stop + Park + TRN + ACK[0:2]: Total 6-bit
    tx[i++] = 0;//data>>23;
    tx[i++] = 0;//data>>15;
    tx[i++] = 0;//data>>7;
    tx[i++] = 0;//(data&0xfe) | parity;
    tx[i++] = 0;//extra 8 bit
    spi_device2_transfer_data(tx,rx,i);
    ESP_LOGD("spi_firmware_dp_low_read","tx=0x%02x %02x %02x %02x %02x %02x %02x",
            tx[0],tx[1], tx[2], tx[3], tx[4], tx[5], tx[6]);
    ESP_LOGD("spi_firmware_dp_low_read","rx=0x%02x %02x %02x %02x %02x %02x %02x",
            rx[0],rx[1], rx[2], rx[3], rx[4], rx[5], rx[6]);

    uint8_t ack;
    bool parity;
    extract_ack_data_parity(rx, &ack, &data, &parity);
    ESP_LOGD("spi_firmware_dp_low_read","ack=0x%x, data=0x%08lx, parity=%s", ack, data, parity? "1":"0");

    //tmp = reverse_bits8(rx[1]<<3) & 0x7; 

	return ack != SWD_ACK_OK;

}
uint8_t spi_request_seq_in(uint8_t request, bool last_time_rd)
{
    uint8_t req = reverse_bits8(request);
    uint8_t tx[2];
    uint8_t rx[2];

    //tx[0]
    if(last_time_rd) {
        tx[0] = FLAG_NORMAL_46B | FLAG_1STBIT_TRN; //13-bit wr
        tx[0] |= (req >>3);
    }
    else{
        tx[0] = FLAG_NORMAL_46B;//12-bit wr
        tx[0] |= (req >>2);
    }

    //tx[1]
    if(last_time_rd){
        tx[1] = req << 5;//hi 2-bit
        tx[1] |= FLAG_SWD_EN_DISABLE>>1;
    }
    else{
        tx[1] = req << 6;//hi 2-bit
        tx[1] |= FLAG_SWD_EN_DISABLE;
    }

    spi_device2_transfer_data(tx,rx,2);

    //get ack from rx
    uint8_t ack = rx[1];
    if(last_time_rd)
        ack = ack <<4;
    else
        ack = ack <<3;

    ack = reverse_bits8(ack) &0x7;
    ESP_LOGD("spi_request_seq_in", "ack=0x%02x last_time_rd=%d", ack, last_time_rd? 1 :0);
    return ack;
}

//#define my_ICE_SPI_CS_PIN 21
int spi2jtag_test(){
    int ret = 0;
    
    //if (spi_device_acquire_bus(gbl_spi_h1, portMAX_DELAY) != ESP_OK) {
    //    ESP_LOGE("FPGA_LOADER", "Failed to acquire SPI bus");
    //    return ret;
    //}

    //gpio_set_level(my_ICE_SPI_CS_PIN,0);
#if 0
    spi_dp_line_reset();

    ret = (int)spi_firmware_dp_low_write(0x10c,0x11223344);
    ret = (int)spi_firmware_dp_low_read(0x10c);
#endif
#if 1
    uint8_t tx[16];
    uint8_t rx[16];
    uint8_t i=0;

    ESP_LOGI("SPI2JTAG_TEST", "test01 enerate 21 SWDCLKs and write");
    tx[i++] = (20 | FLAG_DIO_WR) & (~FLAG_NORMAL_46B);
    tx[i++] = 0xff;
    tx[i++] = 0x55;
    tx[i++] = 0x0f;
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test02 generate 35 SWDCLKs and read, last bit is TRN");
    i=0;
    tx[i++] = 34 & (~FLAG_NORMAL_46B);
    tx[i++] = 0xff;
    tx[i++] = 0x55;
    tx[i++] = 0xaa;
    tx[i++] = 0x0f;
    tx[i++] = 0xf0;
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test03 begin, generate 12 SWDCLKs, RD next");
    i=0;
    tx[i++] = FLAG_NORMAL_46B;
    tx[i++] = 0 | FLAG_SWD_EN_DISABLE;
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test04 begin, generate 13 SWDCLKs, RD next");
    i=0;
    tx[i++] = FLAG_NORMAL_46B | FLAG_1STBIT_TRN;
    tx[i++] = 0x0 | (FLAG_SWD_EN_DISABLE>>1);
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test05 begin, generate 13 SWDCLKs, WR next");
    i=0;
    tx[i++] = FLAG_NORMAL_46B | FLAG_1STBIT_TRN;
    tx[i++] = 0x0 | (FLAG_SWD_EN_DISABLE>>1);
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test06 begin, 34 SWDCLKs and write!");
    i=0;
    tx[i++] = (33 | FLAG_DIO_WR) & (~FLAG_NORMAL_46B);
    tx[i++] = 0xff;
    tx[i++] = 0x55;
    tx[i++] = 0xaa;
    tx[i++] = 0x0f;
    tx[i++] = 0xf0;
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test07 begin,  generate 12 SWDCLKs");
    i=0;
    tx[i++] = FLAG_NORMAL_46B;
    tx[i++] = 0x0 | FLAG_SWD_EN_DISABLE;
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test08 begin, 34 SWDCLKs and read!");
    i=0;
    tx[i++] = 33 & (~FLAG_NORMAL_46B);
    tx[i++] = 0xff;
    tx[i++] = 0x55;
    tx[i++] = 0xaa;
    tx[i++] = 0x0f;
    tx[i++] = 0xf0;
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test09 begin, generate 12 SWDCLKs, RD next, with extra 8-bit");
    i=0;
    tx[i++] = FLAG_NORMAL_46B | FLAG_SWD_RD_NWR;
    tx[i++] = (0x0 | EXTRA_8BIT_FLAG) &(~FLAG_SWD_EN_DISABLE);
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test10 begin, 34+8 SWDCLKs and read!");
    i=0;
    tx[i++] = 0xff;
    tx[i++] = 0x55;
    tx[i++] = 0xaa;
    tx[i++] = 0x0f;
    tx[i++] = 0xf0;
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test11 begin, generate 12 SWDCLKs, WR next, with extra 8-bit");
    i=0;
    tx[i++] = FLAG_NORMAL_46B & (~FLAG_SWD_RD_NWR);
    tx[i++] = (0x0 | EXTRA_8BIT_FLAG) &(~FLAG_SWD_EN_DISABLE);
    spi_device2_transfer_data(tx,rx,i);

    ESP_LOGD("SPI2JTAG_TEST", "test12 begin, 34+8 SWDCLKs and write!");
    i=0;
    tx[i++] = 0xff;
    tx[i++] = 0x55;
    tx[i++] = 0xaa;
    tx[i++] = 0x0f;
    tx[i++] = 0xf0;
    spi_device2_transfer_data(tx,rx,i);
#endif
    //spi_device_release_bus(gbl_spi_h1);
    return ret;
}

