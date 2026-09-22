# Implementation summary

**Builds with CMake; not yet run on hardware.**

## Done

1. **Display**: Waveshare 1.69" ST7789V2 driver (from the known-good `waveshare_OLED_driver` project) as `unified.c/h`. Pins SCK PB10, MOSI PB15, CS PG12, DC PA8, RES PA9. Landscape 280x240, RGB565.
2. **Speed**: 80 MHz PLL from MSI; SPI2 at 40 MHz; pixels sent as 16-bit SPI frames by DMA1 Ch5 straight from the framebuffer (no byte swap), only the dirty rows (full width) per flush; the TouchGFX thread waits on a semaphore so other threads run. DMA2D (Chrom-ART) accelerates TouchGFX blits/fills.
3. **TouchGFX**: 280x240 resolution, framebuffer in a NOLOAD RAM section (not stored in flash), backgrounds and layouts adapted to 280x240, ThreadX timer (20 ms) as VSYNC.
4. **Threads**: TouchGFX (5), BLE (10), UART cmd (11), Monitor (12); the old stub TouchGFX thread was removed.
5. **BLE**: BlueNRG-2 via SPI1 (5 MHz): reset, GATT/GAP init, advertising `Nucleo-BLE-Demo`, connect/disconnect events, re-advertise.
6. **Console**: LPUART1 command parser with IRQ RX (LED0/LED1 ON/OFF/TOGGLE/BLINK/FAST, STATUS, BLE, TEST, HELP); time-based LED blinking.
7. **Logging**: `USB_Logging_*` now writes to LPUART1 under a mutex. USB CDC is not possible: no USBX STM32 DCD / HAL PCD in the project.
8. **Tests**: `TEST` runs display geometry, UART parsing, LED, BLE status and log-sink checks.
9. **Removed**: SSD1306/I2C `oled_driver.*` and the old demo thread code. I2C1 stays configured (timing updated for 80 MHz).
10. **Config**: `.ioc` updated (pins, DMA, DMA2D, PLL, NVIC, width 280). Root `CMakeLists.txt` lists the user sources.

## Known risks

- 40 MHz SPI may need `DISPLAY_SPI_PRESCALER = SPI_BAUDRATEPRESCALER_4` (`unified.h`).
- DMA2D port mixes the 4.22 `STM32DMA` with the 4.26 framework.
- BLE not tested against a real BlueNRG-2; EXTI runs at priority 0.
- UI asset/file names still say `240x240`; background art is stretched.
- PA9 is VBUS-sense on some boards.
- Regenerating from CubeMX/TouchGFX can overwrite hand edits (list in `INTEGRATION_GUIDE.md`).
