# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
set(PICO_EXTRAS_FETCH_FROM_GIT "ON")
include(pico_extras_import.cmake)

project(platform16 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(platform16
        platform16.cpp
        )

target_link_libraries(platform16 pico_stdlib hardware_adc pico_audio_i2s)


target_compile_definitions(platform16 PRIVATE
# compile time configuration of I2S
#PICO_AUDIO_I2S_MONO_INPUT=1
#define for our example code
USE_AUDIO_I2S=1
PICO_AUDIO_I2S_DATA_PIN=9
PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
#            PICO_DEFAULT_UART=0
#            PICO_DEFAULT_UART_TX_PIN=28
#            PICO_DEFAULT_UART_RX_PIN=29
)

# enable usb output, disable uart output
pico_enable_stdio_usb(platform16 1)
pico_enable_stdio_uart(platform16 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(platform16)

# add url via pico_set_program_url

