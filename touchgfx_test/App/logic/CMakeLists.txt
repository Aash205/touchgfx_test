# Pure, hardware-independent application logic. Standard library only: this target deliberately
# gets no HAL, ThreadX, USBX, BlueNRG or TouchGFX include path, so a file in here that includes
# one fails to compile in both the firmware build and the unit-test build.
add_library(app_logic STATIC
    ble_codec.c
    ble_fsm.c
    ble_sync.c
    change_detect.c
    cmd_parse.c
    counters.c
    debounce.c
    led_fsm.c
    log_format.c
    ring.c
    text_writer.c
    timeouts.c
    uart_line.c
    ui_format.c
    ws169_geometry.c
)
target_include_directories(app_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
