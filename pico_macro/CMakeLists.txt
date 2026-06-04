cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pico_macro C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_macro
    src/main.c
)

# Include tusb_config.h directory
target_include_directories(pico_macro PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(pico_macro 
    pico_stdlib 
    tinyusb_device 
    tinyusb_board
)

# Disable USB CDC, enable USB HID only
pico_enable_stdio_usb(pico_macro 0)
pico_enable_stdio_uart(pico_macro 0)

pico_add_extra_outputs(pico_macro)
