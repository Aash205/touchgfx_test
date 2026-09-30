# Demo overview

Firmware for STM32L496ZG (Nucleo-144) running ThreadX with a TouchGFX UI on a Waveshare 1.69" ST7789V2 panel.
**Builds; not yet run on hardware.**

## Features

| Area | Implementation |
|---|---|
| Display | ST7789V2 240x280 in landscape 280x240, RGB565, SPI2 (`App/waveshare_driver/`) |
| GUI | TouchGFX 4.26, 3 screens, 280x240 backgrounds, DMA2D (Chrom-ART) accelerated blits |
| RTOS | ThreadX: TouchGFX (5), BLE (10), UART cmd (11), Monitor (12); 20 ms VSYNC timer |
| BLE | BlueNRG-2 peripheral, SPI1 5 MHz, advertises `Nucleo-BLE-Demo`, re-advertises on disconnect |
| Console | LPUART1 115200: LED0/LED1 ON/OFF/TOGGLE/BLINK/FAST, STATUS, BLE, TEST, HELP |
| Logging | `USB_Logging_*` API (name is historical) writes to LPUART1 under a mutex |
| Tests | `TEST` command runs `Test_RunAll()` (display geometry, UART parsing, LEDs, BLE status, log sink) |

## Not included

- **USB CDC logging**: USBX is linked but the STM32 device controller driver and HAL PCD are not in the project, so USB cannot enumerate.
- **SSD1306 / I2C OLED**: removed (`oled_driver.*` deleted). I2C1 is left configured but unused.
- **Touch input**: no touch controller wired.

## Build

`cmake --preset Debug && cmake --build --preset Debug` (see `QUICK_START.md`).

## Known risks

40 MHz SPI may need `/4`; DMA2D port mixes 4.22 and 4.26 TouchGFX generators; BLE untested on a real module; UI asset names still contain `240x240`; PA9 is VBUS-sense on some boards.
