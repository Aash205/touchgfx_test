# Demo: STM32L496 Nucleo + Waveshare 1.69" LCD + TouchGFX + BLE + console

Build status: everything compiles with CMake. **Nothing has been run on hardware yet.**
Work through the stages below in order and stop at the first failure.

## What the demo shows

| Feature | Where |
|---|---|
| TouchGFX "Live Status" screen (280x240): BLE state, uptime, heartbeat, FPS, LD1/LD3 buttons | `TouchGFX/gui/**` |
| ST7789V2 panel over SPI2, 40 MHz, DMA pixel path, DMA2D (Chrom-ART) | `App/waveshare_driver/**`, `TouchGFX/target/**` |
| BlueNRG-2 BLE peripheral + custom GATT service (LED control, status notify) | `Core/Src/ble_app.c` |
| LPUART1 command console + self tests | `Core/Src/uart_commands.c`, `app_tests.c` |
| Shared state, LED ownership, Nucleo B1 button | `Core/Src/app_core.c`, `Core/Inc/app_state.h` |
| USB CDC log sink (inert until USB is generated, see "Regeneration") | `Core/Src/usb_cdc_log.c` |

## 0. Build and flash

```
cmake --preset Debug && cmake --build --preset Debug
```

Flash `build/Debug/touchgfx_test.elf` via the Nucleo ST-LINK (STM32CubeProgrammer, or
`openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program build/Debug/touchgfx_test.elf verify reset exit"`).

Wiring:

| Function | Pins |
|---|---|
| LCD | SCK PB10, MOSI PB15, CS PG12, DC PA8, RES PA9, VCC 3.3 V, GND |
| Console | USB-UART adapter: adapter RX to PC1, adapter TX to PC0, GND. 115200 8N1 |
| BlueNRG-2 shield | SPI1 (PA1/PA6/PA7), CS PC2, RST PF13, IRQ PE8 |

Open the console before flashing so the boot logs are not missed.

## 1. Boot and display
Expect: panel shows "Live Status", four rows and two buttons; console prints `Console ready. Type HELP.`

| Symptom | Try |
|---|---|
| Blank/white screen | CS/DC/RES wiring, 3.3 V supply |
| Garbled/shifted image | Lower SPI2 speed by changing the prescaler in `MX_SPI2_Init()` in `Core/Src/main.c` |
| Colors swapped | Check the ST7789 MADCTL and pixel-format initialization in `App/waveshare_driver/Src/waveshare_driver.c` |
| No console output at all | Debugger: look for a HardFault (80 MHz clock setup, ThreadX start) |

## 2. Screen values
Uptime ticks every second, FPS > 0 while the UI redraws, heartbeat +1 every 5 s
(console: `Heartbeat N, BLE 3, FPS x`).

## 3. Console and LEDs

| Command | Expect |
|---|---|
| `HELP` | command list |
| `LED0 ON` | LD1 (green) on, screen button green within ~200 ms |
| `LED1 TOGGLE` | LD3 (red) toggles |
| `LED0 BLINK` / `LED0 FAST` | slow / fast blink |
| `STATUS` | LED states |
| `BLE` | `BLE status: 3` (advertising) |
| `TEST` | 5 checks, `Done: 0/5 failed` |

The blue Nucleo button B1 toggles LD1 and the on-screen button.

## 4. BLE (nRF Connect on a phone)
1. Connect to `Nucleo-BLE-Demo`; screen shows Connected, console logs `BLE connected`.
2. Service `8a7c0001-4c3e-4e2b-9d4a-0b5f00c0ffee`:
   - write `03` to `...0002`: both LEDs and both screen buttons turn on; `00` turns them off
   - `LED0 ON` on the console, then read `...0002`: value follows
   - subscribe to `...0003`: 6-byte notification each second (byte0 = 04, byte1 = LED mask, bytes 2-5 = heartbeat u32 LE)
3. Disconnect: console logs `re-advertising`, screen returns to Advertising.

`BLE bring-up failed` in the log: check shield seating and CS/RST/IRQ wiring.

## 5. Stress (5 min)
`LED0 FAST` + BLE connected, run `TEST` repeatedly; the screen must not freeze and FPS must not drop to 0.

## Not testable yet
- On-screen buttons: no touch controller (`STM32TouchController.cpp` is a stub); buttons only show state.
- USB CDC logging: needs regeneration with USB enabled.

---

# Regeneration (CubeMX + TouchGFX Designer)

The code is arranged so regeneration is safe, but do it in this order and review the diff.

## 1. Before you start
```
git add -A && git commit -m "Demo: before regeneration"
```
Then any problem is one `git diff` away. Keep `ProjectManager.KeepUserCode=true` (already set).

## 2. STM32CubeMX
1. Open `touchgfx_test.ioc`. Check it loads without warnings. If it complains about the USB entries, toggle `USB_OTG_FS` once in the GUI (Connectivity, Device Only).
2. Verify in the GUI:
   - Clock: MSI 4 MHz, PLL to 80 MHz SYSCLK; USB clock source HSI48.
   - Pins: PB10/PB15 SPI2 TX-only, PG12/PA8/PA9 outputs (`DISP_CS/DC/RES`), PC7/PB14 LD1/LD3, PA11/PA12 USB.
   - DMA: SPI2_TX on DMA1 Ch5, halfword. DMA2D enabled. NVIC priorities: DMA1 Ch5 = 5, LPUART1 = 6, DMA2D = 9, OTG_FS = 7.
   - TouchGFX pack: display 280 x 240. USBX pack: Device, CDC ACM.
3. Project Manager: toolchain **CMake**. Click **Generate Code**.

## 3. TouchGFX Designer
Open `TouchGFX/touchgfx_test.touchgfx`, confirm resolution 280x240, then **Generate Code**.
Recommended: replace the stretched background PNGs with real 280x240 art and re-layout the other two screens (Screen1 is built in code and is not touched by Designer).

## 4. Review the diff and fix leftovers
`git diff --stat`, then check:

| Item | Expected / action |
|---|---|
| `cmake/stm32cubemx/CMakeLists.txt` | Now lists `dma2d.c`, `stm32l4xx_hal_dma2d.c`, and the USB/PCD sources. The root `CMakeLists.txt` only adds dma2d files if missing, so no duplicates. If you see "multiple definition", remove the root entry. |
| `stm32l4xx_it.c` | CubeMX adds strong DMA1_Ch5 / DMA2D / LPUART1 / OTG_FS handlers. Mine are `__weak` inside the `USER CODE 1` block, so they lose. `EXTI9_5_IRQHandler` must remain (BlueNRG). |
| `Core/Src/tx_initialize_low_level.S` | `SYSTEM_CLOCK` must be `80000000`. If it reverted to 4000000, fix it. |
| `stm32l4xx_hal_conf.h` | `HAL_PCD_MODULE_ENABLED` and `HAL_DMA2D_MODULE_ENABLED` must be on. |
| USBX sources | `ux_dcd_stm32_*` and HAL PCD must be in the build. If the pack did not add them, add the USBX STM32 device-controller sources from ST's USBX package. |
| `TouchGFX/target/generated/*` | Regenerated by the generator: DMA2D `STM32DMA`, 280x240 framebuffer. My hand-edited copies are simply replaced. |
| USBX memory | `app_azure_rtos_config.h` and `app_usbx_device.h` keep my overrides (18 KB stack, 22 KB pool) because they are in `USER CODE` blocks. Verify. |
| `main.c` | `MX_DMA_Init` / `MX_DMA2D_Init` regenerated; nothing of mine is lost (all app code is in app-owned files). |
| Linker | The app uses `linker/STM32L496XX_FLASH_app.ld` (has the `TouchGFX_Framebuffer` section). The root CMake swaps it in. If CubeMX changes the `.ld` name, update the `string(REPLACE ...)` line. |

## 5. Build and test
```
cmake --build --preset Debug
```
Run the stages above. For USB CDC: after flashing, connect the Nucleo USB user port to the PC, open the new serial port (`/dev/ttyACM*`), and you should see the same log lines as on LPUART1 (only while the port is open).

## What is permanent vs generated

- **App-owned (never regenerated):** `App/waveshare_driver/**`, `Core/Src/{ble_app,uart_commands,usb_logging,usb_cdc_log,app_tests,app_bsp,app_core}.c`, `Core/Inc/{app_state,app_core,usb_cdc_log,...}.h`, `linker/*.ld`, `TouchGFX/target/TouchGFXHAL.cpp`, `TouchGFX/gui/**`, root `CMakeLists.txt`.
- **Hooks in generated files (inside USER CODE blocks):** `app_threadx.c`, `stm32l4xx_it.c`, `ux_device_cdc_acm.c`, `app_azure_rtos_config.h`, `app_usbx_device.h`.
- **Derived from the `.ioc`:** pins, clocks, DMA/DMA2D/USB setup, NVIC, framebuffer size.
