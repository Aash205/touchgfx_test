cmake_minimum_required(VERSION 3.22)
project(firmware_host_tests C)

enable_testing()

add_executable(firmware_host_tests
    test_main.c
    ../../App/waveshare_driver/Src/ws169_geometry.c
    ../../Core/Src/uart_line.c
)
target_include_directories(firmware_host_tests PRIVATE
    ../../App/waveshare_driver/Inc
    ../../Core/Inc
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(firmware_host_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME firmware_host_tests COMMAND firmware_host_tests)
