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
#    ./src/platforms/include 
#    ../common
#    ./src/platforms/target
#    .
#    ./src/libopencm3/include)
#
#set(elf_file ${CMAKE_PROJECT_NAME}.elf)

set(SOURCES 
./src/gdb_main.c
./src/exception.c  
./src/hex_utils.c  
./src/crc32.c  
./src/command.c  
#./src/main.c  
./src/timing.c  
./src/morse.c  
./src/gdb_main.c  
./src/gdb_hostio.c  
./src/gdb_packet.c
./src/remote.c
./src/platforms/esp32/main/platform.c
./src/platforms/esp32/main/spi2jtag.c
./src/platforms/esp32/main/gdb_if.c
#./src/platforms/esp32/main/main.c 
./src/platforms/common/swdptap.c
./src/platforms/common/jtagtap.c
#./src/platforms/native/platform.c
#./src/platforms/stm32/traceswodecode.c
#./src/platforms/stm32/traceswo.c
#./src/platforms/stm32/serialno.c
#./src/platforms/stm32/gdb_if.c
)

set(TARGETS 
./src/target/adiv5.c          
./src/target/adiv5_jtagdp.c   
./src/target/adiv5_swdp.c     
./src/target/cortexa.c   
./src/target/cortexm.c   
./src/target/efm32.c     
./src/target/gdb_reg.c
./src/target/msp432.c       
./src/target/nrf51.c         
./src/target/nxpke04.c   
./src/target/jtag_devs.c   
./src/target/jtag_scan.c   
./src/target/lmi.c               
./src/target/lpc_common.c   
./src/target/lpc11xx.c   
./src/target/lpc17xx.c   
./src/target/lpc15xx.c   
./src/target/lpc43xx.c   
./src/target/lpc546xx.c   
./src/target/kinetis.c           
./src/target/samd.c      
./src/target/sam3x.c     
./src/target/sam4l.c     
./src/target/samx5x.c    
./src/target/sfdp.c
./src/target/stm32f1.c   
./src/target/ch32f1.c
./src/target/stm32f4.c   
./src/target/stm32h7.c           
./src/target/stm32l0.c           
./src/target/stm32l4.c
./src/target/stm32g0.c   
./src/target/renesas.c
./src/target/target.c
./src/target/target_flash.c
./src/target/target_probe.c
./src/target/rp.c
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
#            ./libopencm3/include
        REQUIRES driver esp_event esp_wifi nvs_flash esp_timer
)

