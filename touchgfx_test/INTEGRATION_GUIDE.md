# Integration guide

## Source map

| File | Role |
|---|---|
| `App/waveshare_driver/Src/waveshare_driver.c`, `App/waveshare_driver/Inc/waveshare_driver.h` | ST7789V2 driver and board SPI2/pin setup; pin macros `DISP_*` in `Core/Inc/main.h` |
| `TouchGFX/target/TouchGFXHAL.cpp` | inits panel, flushes dirty rows, `touchgfxSignalVSync()` |
| `Core/Src/app_threadx.c` | VSYNC timer, threads, UART RX callbacks |
| `Core/Src/ble_app.c` | BlueNRG-2 init/advertising/event dispatch |
| `Core/Src/uart_commands.c` | console command parser and LED control |
| `Core/Src/usb_logging.c` | log/console sink on LPUART1 (mutex-protected) |
| `Core/Src/app_tests.c` | smoke tests, run with `TEST` |
| `Core/Src/dma2d.c` | DMA2D init for TouchGFX |

## Build

Root `CMakeLists.txt` `target_sources` lists the application sources. Build: `cmake --preset Debug && cmake --build --preset Debug`.

## Changing the display pins/speed

Edit `DISP_CS/DC/RES_*` in `Core/Inc/main.h` and the GPIO init in `MX_GPIO_Init`; SPI2 pins are in `stm32l4xx_hal_msp.c`. To lower SPI speed, adjust the prescaler in `MX_SPI2_Init()` in `Core/Src/main.c`.

## Logging / console

`USB_Logging_Printf(level, fmt, ...)` and `USB_Logging_SendRaw()` write to LPUART1. USB CDC is not possible until the USBX STM32 device controller driver and HAL PCD are added; only `USB_Logging_SendRaw()` would need to change.

## Hand edits that CubeMX / TouchGFX regeneration can overwrite

`touchgfx_test.ioc` has been updated (pins, DMA, DMA2D, PLL, NVIC, 280 width), but these edits sit outside USER CODE blocks and must be re-checked after any regeneration:

- `TouchGFX/target/generated/STM32DMA.cpp/.hpp`: DMA2D version from TouchGFX 4.22, with its `paint` namespace removed (4.26 provides it).
- `TouchGFXGeneratedHAL.cpp`: DMA2D IRQ enable/disable/priority, framebuffer 280x240, Paint includes kept.
- `TouchGFXConfiguration.cpp`: HAL size 280x240; `touchgfx_test.touchgfx` resolution 280x240; generated GUI/backgrounds resized by hand.
- `STM32L496XX_FLASH.ld`: `TouchGFX_Framebuffer` NOLOAD section.
- `Core/Src/custom_bus.c`: SPI1 8-bit, prescaler 16.
- `Core/Src/tx_initialize_low_level.S`: `SYSTEM_CLOCK` 80 MHz.
- `main.c` / `stm32l4xx_hal_msp.c` / `stm32l4xx_it.c`: clock, DMA init, GPIOG VddIO2, DMA/LPUART/EXTI/DMA2D handlers.

## Untested / risks

Not run on hardware. 40 MHz SPI, the 4.22 DMA2D code on the 4.26 framework, and BLE against a real BlueNRG-2 are all unverified. Asset names still say `240x240`. PA9 is VBUS-sense on some boards.

## Regeneration checklist (CubeMX, then TouchGFX Designer)

Hand edits are arranged so regeneration is safe:

- App-owned files (never regenerated): `App/waveshare_driver/**`, `Core/Src/{ble_app,uart_commands,usb_logging,usb_cdc_log,app_tests,app_bsp}.c`, `linker/STM32L496XX_FLASH_app.ld` (used via root `CMakeLists.txt`), `TouchGFX/target/TouchGFXHAL.cpp`.
- Interrupt handlers for DMA1_Ch5 / DMA2D / LPUART1 in `stm32l4xx_it.c` are `__weak`; CubeMX generates the strong ones. `EXTI9_5_IRQHandler` stays (not in the .ioc: BlueNRG pack owns it).
- `dma2d.c` / `stm32l4xx_hal_dma2d.c` are added by the root CMake only if CubeMX's list lacks them.
- USB: the `.ioc` has USB_OTG_FS (device only, PA11/PA12, HSI48, no VBUS sensing so PA9 stays LCD RES). Pool sizes are overridden in USER CODE blocks (`app_azure_rtos_config.h`, `app_usbx_device.h`). Log sink hooks sit in the USER CODE blocks of `ux_device_cdc_acm.c`.

After generating: build; if the USB pack did not add `ux_dcd_stm32_*` / HAL PCD sources, enable `HAL_PCD_MODULE_ENABLED` and add the USBX STM32 device-controller sources. Check `cmake/stm32cubemx/CMakeLists.txt` for duplicates, and confirm `SYSTEM_CLOCK` in `tx_initialize_low_level.S` is 80000000. In Designer, re-layout the screens at 280x240 with real 280x240 background PNGs (the current ones are stretched 240x240 assets).
