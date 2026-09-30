cmake_minimum_required(VERSION 3.22)
project(unit_tests C)

enable_testing()

add_executable(unit_tests
    test_main.c
    ../App/waveshare_driver/Src/ws169_geometry.c
    ../Core/Src/uart_line.c
)
target_include_directories(unit_tests PRIVATE
    ../App/waveshare_driver/Inc
    ../Core/Inc
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unit_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME unit_tests COMMAND unit_tests)
