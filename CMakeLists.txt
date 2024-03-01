cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/../pico-extras/external/pico_extras_import.cmake)

project(pifi-firmware VERSION 0.0.0
    LANGUAGES C CXX ASM
)

pico_sdk_init()
add_executable(pifi-firmware-release pifi-firmware.c userport.c rxtx.c)
target_compile_definitions(pifi-firmware-release PRIVATE
    DEBUG=0
)
target_include_directories(pifi-firmware-release PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(pifi-firmware-release
    pico_stdlib
    pico_cyw43_arch_none
)
pico_enable_stdio_usb(pifi-firmware-release 1)
pico_enable_stdio_uart(pifi-firmware-release 0)
pico_add_extra_outputs(pifi-firmware-release)

add_executable(pifi-firmware-log pifi-firmware.c userport.c rxtx.c)
target_compile_definitions(pifi-firmware-log PRIVATE
    DEBUG=2
)
target_include_directories(pifi-firmware-log PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(pifi-firmware-log
    pico_stdlib
    pico_cyw43_arch_none
)
pico_enable_stdio_usb(pifi-firmware-log 1)
pico_enable_stdio_uart(pifi-firmware-log 0)
pico_add_extra_outputs(pifi-firmware-log)

add_executable(pifi-firmware-debug pifi-firmware.c userport.c rxtx.c)
target_compile_definitions(pifi-firmware-debug PRIVATE
    DEBUG=3
)
target_include_directories(pifi-firmware-debug PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(pifi-firmware-debug
    pico_stdlib
    pico_cyw43_arch_none
)
pico_enable_stdio_usb(pifi-firmware-debug 1)
pico_enable_stdio_uart(pifi-firmware-debug 0)
pico_add_extra_outputs(pifi-firmware-debug)
