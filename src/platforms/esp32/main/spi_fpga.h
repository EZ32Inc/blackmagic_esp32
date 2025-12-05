#ifndef SPI_FPGA_H
#define SPI_FPGA_H

#define RDnWR 0x20
#define EXTRA8_FLAG 0x20 //bit 5 of 2nd byte
#define FIRST_BYTE_WR 0x81
#define FIRST_BYTE_RD 0xa1

#define FLAG_NORMAL_46B 0x80
#define FLAG_DIO_WR     0x40
#define FLAG_1STBIT_TRN 0x40
#define FLAG_SWD_RD_NWR 0x08
#define FLAG_SWD_EN_DISABLE     0x04
#define EXTRA_8BIT_FLAG 0x10

//#define SPI2JTAG 1

int spi2jtag_test();
void  spi_dp_line_reset();
uint32_t spi_firmware_dp_low_read(/*adiv5_debug_port_s *dp,*/ const uint16_t addr);
bool spi_firmware_dp_low_write(/*adiv5_debug_port_s *dp, */const uint16_t addr, const uint32_t data_in);
void spi_dp_wr_nbit(uint32_t data_in, uint8_t nbit);
void spi_dp_wr32bit(uint32_t data_in);
uint8_t spi_request_seq_in(uint8_t request, bool last_time_rd);

bool spi_dp_seq_in_parity_32bit(uint32_t *data);
void spi_dp_seq_out_parity_32bit(uint32_t data_in);
#endif//SPI_FPGA_H
