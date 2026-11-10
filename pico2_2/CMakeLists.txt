# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico2_2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_compile_definitions(PICO_CLOCK_ADJUST_PERI_CLOCK_WITH_SYS_CLOCK=1)

# Add executable. Default name is the project name, version 0.1

add_executable(pico2_2 pico2_2.cpp )

pico_set_program_name(pico2_2 "pico2_2")
pico_set_program_version(pico2_2 "0.1")
pico_set_binary_type(pico2_2 copy_to_ram)
pico_override_flash_size(pico2_2 "2 * 1024 * 1024")

# Generate PIO header
pico_generate_pio_header(pico2_2 ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico2_2 1)
pico_enable_stdio_usb(pico2_2 1)

# Add the standard library to the build
target_link_libraries(pico2_2
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(pico2_2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        C:/Drivers/Display/uTFT2
        C:/Drivers/Display/uTFT2/Driver
        C:/Drivers/libs/logUART
)

# Add driver and application sources
target_sources(pico2_2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/scope_input.cpp
        ${CMAKE_CURRENT_LIST_DIR}/scope_adc.cpp
        ${CMAKE_CURRENT_LIST_DIR}/scope_dsp.cpp
        ${CMAKE_CURRENT_LIST_DIR}/scope_render.cpp
        C:/Drivers/Display/uTFT2/TFT.cpp
        C:/Drivers/Display/uTFT2/TFT_Primitive.cpp
        C:/Drivers/Display/uTFT2/Driver/ST7789.cpp
        C:/Drivers/Display/uTFT2/FontClassic/Resource_Classic_Fonts.cpp
        C:/Drivers/libs/logUART/timber.cpp
)

# Add any user requested libraries
target_link_libraries(pico2_2 
        hardware_spi
        hardware_dma
        hardware_pio
        hardware_pwm
        hardware_gpio
        hardware_vreg
        hardware_adc
        pico_multicore
        )

pico_add_extra_outputs(pico2_2)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_custom_command(TARGET pico2_2 POST_BUILD
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/tools/pico_size_report.py
            --elf $<TARGET_FILE:pico2_2>
            --map ${CMAKE_CURRENT_BINARY_DIR}/pico2_2.elf.map
            --html ${CMAKE_CURRENT_BINARY_DIR}/firmware_size.html
        COMMENT "Detailed Memory Footprint:"
    )
else()
    add_custom_command(TARGET pico2_2 POST_BUILD
        COMMAND arm-none-eabi-size $<TARGET_FILE:pico2_2>
        COMMENT "Memory footprint:"
    )
endif()
