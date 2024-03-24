cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/../pico-extras/external/pico_extras_import.cmake)

project(wipi-firmware VERSION 0.0.0
    LANGUAGES C CXX ASM
)

pico_sdk_init()
add_executable(wipi-firmware-release wipi-firmware.c status.c userport.c rxtx.c)
target_compile_definitions(wipi-firmware-release PRIVATE
    DEBUG=0
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)
target_include_directories(wipi-firmware-release PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(wipi-firmware-release
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
)

add_executable(wipi-firmware-log wipi-firmware.c status.c userport.c rxtx.c)
target_compile_definitions(wipi-firmware-log PRIVATE
    DEBUG=2
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)
target_include_directories(wipi-firmware-log PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(wipi-firmware-log
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
)

add_executable(wipi-firmware-debug wipi-firmware.c status.c userport.c rxtx.c)
target_compile_definitions(wipi-firmware-debug PRIVATE
    DEBUG=3
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)
target_include_directories(wipi-firmware-debug PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(wipi-firmware-debug
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_enable_stdio_usb(wipi-firmware-release 0)
pico_enable_stdio_uart(wipi-firmware-release 0)
pico_add_extra_outputs(wipi-firmware-release)

pico_enable_stdio_usb(wipi-firmware-log 1)
pico_enable_stdio_uart(wipi-firmware-log 0)
pico_add_extra_outputs(wipi-firmware-log)

pico_enable_stdio_usb(wipi-firmware-debug 1)
pico_enable_stdio_uart(wipi-firmware-debug 0)
pico_add_extra_outputs(wipi-firmware-debug)
