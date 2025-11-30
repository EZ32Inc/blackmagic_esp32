# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32jtag)
#idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DPC_HOSTED=0" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DENABLE_DEBUG=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format" APPEND)
