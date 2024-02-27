cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(blink C CXX ASM)

pico_sdk_init()
add_executable(blink blink.c)
target_link_libraries(blink pico_stdlib pico_cyw43_arch_none)
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)
pico_add_extra_outputs(blink)
