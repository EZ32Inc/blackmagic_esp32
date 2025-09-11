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
../../../gdb_main.c
../../../exception.c  
../../../hex_utils.c  
../../../crc32.c  
../../../command.c  
../../../main.c  
../../../timing.c  
../../../morse.c  
../../../gdb_main.c  
../../../gdb_hostio.c  
../../../gdb_packet.c
../../../remote.c
platform.c
gdb_if.c
spi2jtag.c
../../common/swdptap.c
../../common/jtagtap.c
#../../../platforms/native/platform.c
#../../../platforms/stm32/traceswodecode.c
#../../../platforms/stm32/traceswo.c
#../../../platforms/stm32/serialno.c
#../../../platforms/stm32/gdb_if.c
)

set(TARGETS 
../../../target/adiv5.c          
../../../target/adiv5_jtagdp.c   
../../../target/adiv5_swdp.c     
../../../target/cortexa.c   
../../../target/cortexm.c   
../../../target/efm32.c     
../../../target/gdb_reg.c
../../../target/msp432.c       
../../../target/nrf51.c         
../../../target/nxpke04.c   
../../../target/jtag_devs.c   
../../../target/jtag_scan.c   
../../../target/lmi.c               
../../../target/lpc_common.c   
../../../target/lpc11xx.c   
../../../target/lpc17xx.c   
../../../target/lpc15xx.c   
../../../target/lpc43xx.c   
../../../target/lpc546xx.c   
../../../target/kinetis.c           
../../../target/samd.c      
../../../target/sam3x.c     
../../../target/sam4l.c     
../../../target/samx5x.c    
../../../target/sfdp.c
../../../target/stm32f1.c   
../../../target/ch32f1.c
../../../target/stm32f4.c   
../../../target/stm32h7.c           
../../../target/stm32l0.c           
../../../target/stm32l4.c
../../../target/stm32g0.c   
../../../target/renesas.c
../../../target/target.c
../../../target/target_flash.c
../../../target/target_probe.c
../../../target/rp.c
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
        SRCS ${SOURCES} ${TARGETS} main.c 
        INCLUDE_DIRS 
            ../../../include 
            ../../common
            ../../../target
            .
#            ../../../../libopencm3/include
        REQUIRES driver esp_event esp_wifi nvs_flash esp_timer
)

