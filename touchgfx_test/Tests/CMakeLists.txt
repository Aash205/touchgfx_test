cmake_minimum_required(VERSION 3.22)
project(unit_tests C)

# Same language standard as the firmware (root CMakeLists.txt: C11 with GNU extensions).
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

enable_testing()

# Same source list as the firmware build, compiled with the native compiler.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../App/logic ${CMAKE_BINARY_DIR}/app_logic)

# The product logic is held to the same warning bar as the tests.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(app_logic PRIVATE -Wall -Wextra -Werror)
endif()

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

add_unit_test(test_ble_codec)
add_unit_test(test_cmd_parse)
add_unit_test(test_counters)
add_unit_test(test_debounce)
add_unit_test(test_led_fsm)
add_unit_test(test_log_format)
add_unit_test(test_ring)
add_unit_test(test_text_writer)
add_unit_test(test_timeouts)
add_unit_test(test_uart_line)
add_unit_test(test_ws169_geometry)
