# Unit tests

Unit tests for the hardware-independent code in `App/logic/`. They run on the development
computer with the native compiler: no board, no HAL, no mocks. Code that cannot be separated from
hardware is not tested (there are no on-device tests).

```sh
scripts/unit-test.sh
```

The script configures `Tests/` with CMake + Ninja, builds one executable per module, and runs them
with `ctest`. It exits non-zero if any test fails. Framework: Unity v2.7.0 (MIT), vendored in
`Tests/vendor/unity/` (see `VENDORED.md` there; do not edit those files).

## Adding a test for a module

1. Put the module in `App/logic/` (a `.c` and a `.h`, standard library includes only) and add the
   `.c` to the source list in `App/logic/CMakeLists.txt`.
2. Write `Tests/test_<module>.c` with `setUp` / `tearDown`, one `void test_<behaviour>(void)` per
   behaviour, and a `main` that calls `UNITY_BEGIN()`, `RUN_TEST(...)` per test, `UNITY_END()`.
3. Add `add_unit_test(test_<module>)` to `Tests/CMakeLists.txt`.
4. Format the file once with `clang-format -i` (`Tests/` is format-checked but not MISRA-linted).

## Current modules

| Module | Test file | Cases |
|---|---|---|
| `ble_codec` | `test_ble_codec.c` | 31 |
| `change_detect` | `test_change_detect.c` | 12 |
| `cmd_parse` | `test_cmd_parse.c` | 21 |
| `counters` | `test_counters.c` | 9 |
| `debounce` | `test_debounce.c` | 8 |
| `led_fsm` | `test_led_fsm.c` | 15 |
| `log_format` | `test_log_format.c` | 16 |
| `ring` | `test_ring.c` | 14 |
| `text_writer` | `test_text_writer.c` | 11 |
| `timeouts` | `test_timeouts.c` | 12 |
| `uart_line` | `test_uart_line.c` | 7 |
| `ui_format` | `test_ui_format.c` | 11 |
| `ws169_geometry` | `test_ws169_geometry.c` | 23 |
