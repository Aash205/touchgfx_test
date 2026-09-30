cmake_minimum_required(VERSION 3.22)
project(unit_tests C)

enable_testing()

# Same source list as the firmware build, compiled with the native compiler.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../App/logic ${CMAKE_BINARY_DIR}/app_logic)

add_library(unity STATIC vendor/unity/unity.c)
target_include_directories(unity PUBLIC vendor/unity)

function(add_unit_test name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} PRIVATE app_logic unity)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(test_uart_line)
add_unit_test(test_ws169_geometry)
