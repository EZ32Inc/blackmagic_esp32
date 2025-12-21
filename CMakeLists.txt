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
src/platforms/esp32/main/main.c
src/platforms/esp32/main/platform.c
src/platforms/esp32/main/gdb_if.c
src/platforms/esp32/main/rtt_if.c
src/platforms/esp32/main/swo_if.c
src/platforms/esp32/main/swo_uart.c
src/platforms/esp32/main/spi_fpga.c
src/platforms/esp32/main/spi2swd.c
src/platforms/esp32/main/spi2jtag.c
src/command.c
src/crc32.c
src/exception.c
src/gdb_main.c
src/gdb_packet.c
src/hex_utils.c
src/main.c
src/maths_utils.c
src/morse.c
src/remote.c
src/timing.c
src/rtt.c
src/target/adi.c
src/target/adiv5.c
src/target/adiv5_jtag.c
src/target/adiv5_swd.c
src/target/adiv6.c
src/target/gdb_reg.c
src/target/jtag_devs.c
src/target/jtag_scan.c
src/target/semihosting.c
src/target/sfdp.c
src/target/spi.c
src/target/target.c
src/target/target_flash.c
src/target/target_probe.c
src/target/cortexar.c
src/target/cortex.c
src/target/cortexm.c
src/target/riscv32.c
src/target/riscv_debug.c
src/target/riscv_adi_dtm.c
src/target/riscv_jtag_dtm.c
src/target/riscv64.c
src/target/apollo3.c
src/target/at32f43x.c
src/target/stm32_common.c
src/target/ch32f1.c
src/target/stm32f1.c
src/target/ch579.c
src/target/efm32.c
src/target/stm32f4.c
src/target/hc32l110.c
src/target/lpc11xx.c
src/target/lpc15xx.c
src/target/lpc17xx.c
src/target/lpc40xx.c
src/target/lpc43xx.c
src/target/lpc546xx.c
src/target/lpc55xx.c
src/target/lpc_common.c
src/target/nrf51.c
src/target/nrf54l.c
src/target/nrf91.c
src/target/imxrt.c
src/target/kinetis.c
src/target/nxpke04.c
src/target/s32k3xx.c
src/target/puya.c
src/target/renesas_ra.c
src/target/renesas_rz.c
src/target/rp2040.c
src/target/rp2350.c
src/target/sam3x.c
src/target/sam4l.c
src/target/samd.c
src/target/samx5x.c
src/target/stm32g0.c
src/target/stm32h5.c
src/target/stm32h7.c
src/target/stm32l0.c
src/target/stm32l4.c
src/target/stm32mp15.c
src/target/stm32wb0.c
src/target/lmi.c
src/target/msp432e4.c
src/target/msp432p4.c
src/target/mspm0.c
src/target/zynq7000.c
#src/platforms/common/jtagtap.c
#src/platforms/common/swdptap.c
#src/platforms/common/blackpill-f4/blackpill-f4.c
#src/platforms/common/aux_serial.c
#src/platforms/common/usb.c
#src/platforms/common/usb_dfu_stub.c
#src/platforms/common/usb_serial.c
#src/platforms/common/syscalls.c
#src/platforms/common/stm32/serialno.c
#src/platforms/common/stm32/timing_stm32.c
#src/platforms/common/stm32/gdb_if.c
#src/platforms/common/stm32/rtt_if.c
#src/platforms/common/stm32/swo.c
#src/platforms/common/stm32/swo_itm_decode.c
#src/platforms/common/stm32/swo_manchester.c
#src/platforms/common/stm32/swo_uart.c
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
            ./src/include
            ./src/platforms/common
            ./src/target
            ./src/platforms/esp32/main
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
