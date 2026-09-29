# Board Bring-Up Test Cases

Connect the LPUART1 console at `115200 8N1`, reset the board, and wait for the
ThreadX/application startup messages. Tests are non-destructive unless stated
otherwise. A case passes only when the console reports `Test case N: PASS`.

| Case | Command | Objective | Expected result |
|---|---|---|---|
| 1 | `TEST 1` | Verify the logging sink used by the bring-up console. | The console sink accepts a log message. |
| 2 | `TEST 2` | Verify ST7789 panel constants, geometry, rotation/window translation, initialized state, and driver error counters. | 12 deterministic driver checks pass; display is `280x240`, ST7789, with zero SPI/DMA/timeout faults. |
| 3 | `TEST 3` | Verify UART line assembly and command parsing. | A simulated `LED0 ON` line is assembled exactly. |
| 4 | `TEST 4` | Verify LED state-machine timing configuration. | Fast blink is 100 ms and slow blink is 500 ms. |
| 5 | `TEST 5` | Verify the BlueNRG-2 application reaches an operational state. | BLE status is initialized, advertising, or connected. |

Use `TEST` or `TEST ALL` to execute all five cases. The TouchGFX live-status
screen shows the aggregate result and last case, for example `Tests 5/5 L5 PASS`.
Use `HELP` to display the command list. Record the board revision, firmware
build, console output, and any hardware setup or skipped checks in the bring-up
report.
