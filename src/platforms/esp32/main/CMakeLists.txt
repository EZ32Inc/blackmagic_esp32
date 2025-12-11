cmake_minimum_required(VERSION 3.5)

#idf_build_set_property(COMPILE_OPTIONS "-DPC_HOSTED=0" APPEND)
#add_definitions(-DPC_HOSTED=0)

#set(SUPPORTED_TARGETS esp32s3)

#    # Include for ESP-IDF build system functions
#    include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
#    # Create idf::esp32 and idf::freertos static libraries
#    idf_build_process(esp32s3
#                    # try and trim the build; additional components
#                    # will be included as needed based on dependency tree
#                    #
#                    # although esptool_py does not generate static library,
#                    # processing the component is needed for flashing related
#                    # targets and file generation
#                    COMPONENTS esp32 freertos esptool_py nvs_flash
#                    SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig
#                    BUILD_DIR ${CMAKE_BINARY_DIR})
#
#include_directories($ENV{IDF_PATH}/components/nvs_flash/include
#    ../../include 
#    ../common
#    ../../target
#    .
#    ../../../libopencm3/include)
#
#set(elf_file ${CMAKE_PROJECT_NAME}.elf)

set(SOURCES 
platform.c
gdb_if.c
rtt_if.c
swo_if.c
swo_uart.c
spi_fpga.c
spi2swd.c
spi2jtag.c
esp32_appmain.c
main.c
../../../command.c
../../../crc32.c
../../../exception.c
../../../gdb_main.c
../../../gdb_packet.c
../../../hex_utils.c
../../../maths_utils.c
../../../morse.c
../../../remote.c
../../../timing.c
../../../rtt.c
../../../target/adi.c
../../../target/adiv5.c
../../../target/adiv5_jtag.c
../../../target/adiv5_swd.c
../../../target/adiv6.c
../../../target/gdb_reg.c
../../../target/jtag_devs.c
../../../target/jtag_scan.c
../../../target/semihosting.c
../../../target/sfdp.c
../../../target/spi.c
../../../target/target.c
../../../target/target_flash.c
../../../target/target_probe.c
../../../target/cortexar.c
../../../target/cortex.c
../../../target/cortexm.c
../../../target/riscv32.c
../../../target/riscv_debug.c
../../../target/riscv_adi_dtm.c
../../../target/riscv_jtag_dtm.c
../../../target/riscv64.c
../../../target/apollo3.c
../../../target/at32f43x.c
../../../target/stm32_common.c
../../../target/ch32f1.c
../../../target/stm32f1.c
../../../target/ch579.c
../../../target/efm32.c
../../../target/stm32f4.c
../../../target/hc32l110.c
../../../target/lpc11xx.c
../../../target/lpc15xx.c
../../../target/lpc17xx.c
../../../target/lpc40xx.c
../../../target/lpc43xx.c
../../../target/lpc546xx.c
../../../target/lpc55xx.c
../../../target/lpc_common.c
../../../target/nrf51.c
../../../target/nrf54l.c
../../../target/nrf91.c
../../../target/imxrt.c
../../../target/kinetis.c
../../../target/nxpke04.c
../../../target/s32k3xx.c
../../../target/puya.c
../../../target/renesas_ra.c
../../../target/renesas_rz.c
../../../target/rp2040.c
../../../target/rp2350.c
../../../target/sam3x.c
../../../target/sam4l.c
../../../target/samd.c
../../../target/samx5x.c
../../../target/stm32g0.c
../../../target/stm32h5.c
../../../target/stm32h7.c
../../../target/stm32l0.c
../../../target/stm32l4.c
../../../target/stm32mp15.c
../../../target/stm32wb0.c
../../../target/lmi.c
../../../target/msp432e4.c
../../../target/msp432p4.c
../../../target/mspm0.c
../../../target/zynq7000.c
#../../../platforms/common/blackpill-f4/blackpill-f4.c
#../../../platforms/common/aux_serial.c
../../../platforms/common/jtagtap.c
../../../platforms/common/swdptap.c
#../../../platforms/common/usb.c
#../../../platforms/common/usb_dfu_stub.c
#../../../platforms/common/usb_serial.c
#../../../platforms/common/syscalls.c
#../../../platforms/common/stm32/serialno.c
#../../../platforms/common/stm32/timing_stm32.c
#../../../platforms/common/stm32/gdb_if.c
#../../../platforms/common/stm32/rtt_if.c
#../../../platforms/common/stm32/swo.c
#../../../platforms/common/stm32/swo_itm_decode.c
#../../../platforms/common/stm32/swo_manchester.c
#../../../platforms/common/stm32/swo_uart.c
)

set(TARGETS 
)

# add_executable(${elf_file} ${SOURCES} ${TARGETS} main.c)
# 
# # Link the static libraries to the executable
# target_link_libraries(${elf_file} idf::esp32 idf::freertos idf::spi_flash)
# # Attach additional targets to the executable file for flashing,
# # linker script generation, partition_table generation, etc.
# idf_build_executable(${elf_file})
# 
# set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
#target_compile_options(${SOURCES} ${TARGETS} PRIVATE -DPC_HOSTED=0)
#set_source_files_properties(${TARGETS} PROPERTIES COMPILE_FLAGS "-DPC_HOSTED=0")
#set_source_files_properties(${TARGETS} PROPERTIES COMPILE_FLAGS "-Wall")
idf_component_register(
        SRCS ${SOURCES} ${TARGETS}
        INCLUDE_DIRS 
            ../../../include 
            ../../common
            ../../../target
            .
#            ../../hosted/
#            ../../../../libopencm3/include
        REQUIRES driver esp_event esp_wifi nvs_flash esp_timer
)

target_compile_definitions(${COMPONENT_TARGET}
    PRIVATE
        NO_LIBOPENCM3=1
        CONFIG_BMDA=0
        HOSTED_BMP_ONLY=1
        ENABLE_RTT=1        
        PLATFORM_HAS_TRACESWO=1
        SWO_ENCODING=2
)
