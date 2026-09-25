cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME touchgfx_test)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM, C and C++ languages
enable_language(C CXX ASM)

# Use the application-owned linker script (adds the TouchGFX_Framebuffer NOLOAD section).
# The CubeMX one is regenerated and does not have it.
string(REPLACE "STM32L496XX_FLASH.ld" "linker/STM32L496XX_FLASH_app.ld"
       CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add touchgfx generated sources and libraries
add_subdirectory(cmake/touchgfx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/unified.c          # ST7789V2 (Waveshare 1.69") display driver, SPI2 + DMA
    Core/Src/ble_app.c          # BlueNRG-2 peripheral
    Core/Src/uart_commands.c    # LPUART1 command console
    Core/Src/usb_logging.c      # log / console sink (LPUART1)
    Core/Src/app_tests.c        # on-target smoke tests ("TEST" command)
    Core/Src/app_core.c         # shared AppState, LED/console owner, user button
    Core/Src/app_bsp.c          # BlueNRG-2 SPI1 override (regeneration-safe)
    Core/Src/usb_cdc_log.c      # USB CDC log sink (inert until the USB device is enabled)
)

# STM32 USBX device-controller bridge. CubeMX enabled USBX CDC-ACM but did not
# add these port sources, so the USB peripheral was never attached to USBX.
file(GLOB USBX_DCD_STM32_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/USBX/Target/ux_dcd_stm32*.c"
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${USBX_DCD_STM32_SOURCES})

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
