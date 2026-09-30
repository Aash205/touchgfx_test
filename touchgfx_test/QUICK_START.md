# Quick Start — STM32L496 Nucleo + Waveshare 1.69" LCD + TouchGFX

> **Status:** everything builds with CMake. **Nothing has been run on hardware yet.** See "Known risks" below.

## What this firmware does

- **TouchGFX 280x240 UI** on a Waveshare 1.69" ST7789V2 SPI panel (240x280 panel used in landscape).
- **ThreadX RTOS**: TouchGFX thread (prio 5) + BLE (10), UART command console (11), Monitor heartbeat (12).
- **BlueNRG-2 BLE** peripheral on SPI1 (advertises `Nucleo-BLE-Demo`).
- **UART console** on LPUART1 (LED control, status, self-tests) — also the log output.

## Hardware and wiring

| Signal | Pin | Notes |
|---|---|---|
| LCD SCK / MOSI | PB10 / PB15 | SPI2, TX only |
| LCD CS / DC / RES | PG12 / PA8 / PA9 | GPIO out (`DISP_CS/DC/RES`); GPIOG needs VddIO2 (enabled in `MX_GPIO_Init`) |
| BlueNRG-2 | SPI1 PA1/PA6/PA7, CS PC2, RST PF13, IRQ PE8 | SPI1 at 5 MHz |
| Console | LPUART1 PC0 (RX) / PC1 (TX), 115200 8N1 | needs a USB-UART adapter on these pins |
| LEDs | LD1 = PC7, LD3 = PB14 | `LED0` = LD1, `LED1` = LD3 |

`I2C1` (PB7/PB8) is still configured but unused (the old SSD1306 OLED is gone).

## Build (CMake only)

```
cmake --preset Debug
cmake --build --preset Debug        # or: cmake --build build/Debug
```

Toolchain: `cmake/gcc-arm-none-eabi.cmake` (needs `arm-none-eabi-gcc`, CMake ≥ 3.22, Ninja). Output: `build/Debug/touchgfx_test.elf`.
Flash with your usual tool (e.g. `openocd`/STM32CubeProgrammer) on the ELF.
User sources are listed in the root `CMakeLists.txt` (`target_sources`).

## UART console

Connect at 115200 8N1 and type (Enter terminates):

```
LED0 ON | OFF | TOGGLE | BLINK | FAST      (same for LED1)
STATUS      LED states
BLE         BLE status code
TEST        run on-target smoke tests
HELP
```

Logs (`[INFO] [ss.mmm] ...`) share the same port; the Monitor thread prints a heartbeat every 5 s.

## Known risks (untested)

- SPI2 runs at 40 MHz (80 MHz / 2). If the image is garbled, lower the SPI2 speed by changing the prescaler in `MX_SPI2_Init()` in `Core/Src/main.c`.
- DMA2D support uses the TouchGFX 4.22 `STM32DMA` with the 4.26 framework.
- BLE has not been tried against a real BlueNRG-2.
- UI asset/file names still say `240x240` (they are 280x240 now).
- PA9 is USB VBUS-sense on some boards; check before using it as LCD RES.

## Demo features

- **Live status screen (Screen1):** BLE state, uptime, heartbeat, FPS and two LED buttons (LD1/LD3). Buttons react to touch once a touch controller is added (`STM32TouchController.cpp` is still a stub); until then they show the LED state.
- **Nucleo user button B1 (PC13):** toggles LD1.
- **BLE GATT demo service** (advertised as `Nucleo-BLE-Demo`, use nRF Connect):
  - service `8a7c0001-4c3e-4e2b-9d4a-0b5f00c0ffee`
  - `...0002` LED control, read/write, 1 byte bitmask (bit0 = LD1, bit1 = LD3)
  - `...0003` status, read/notify, 6 bytes: `ble_status`, `led_mask`, heartbeat (u32 LE), notified every second
- All LED sources (UART, BLE, B1, GUI) go through `AppState_SetLed()` in `Core/Src/app_core.c`, so every view stays in sync.
