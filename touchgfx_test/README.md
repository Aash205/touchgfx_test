# STM32L496 Nucleo + Waveshare 1.69" LCD + TouchGFX

Firmware for an STM32L496ZG (Nucleo-144) running ThreadX with a TouchGFX UI on a Waveshare
1.69" ST7789V2 SPI panel, a BlueNRG-2 BLE peripheral, and an LPUART1 command console.

> **Status:** builds with CMake. As documented by the original bring-up notes, it has not been
> verified on hardware.

## Features

| Area | Implementation |
|---|---|
| Display | ST7789V2 240x280 used in landscape 280x240, RGB565, SPI2 (`App/waveshare_driver/`) |
| GUI | TouchGFX 4.26, "Live Status" screen (`Screen1`, built in code), DMA2D (Chrom-ART) blits |
| RTOS | ThreadX: TouchGFX (prio 5), BLE (10), UART command console (11), Monitor heartbeat (12), USB CDC log drain (14); 20 ms VSYNC timer |
| BLE | BlueNRG-2 peripheral on SPI1 (prescaler 64), advertises `Nucleo-BLE-Demo`, re-advertises on disconnect |
| Console | LPUART1 115200 8N1: LED control, status, BLE status, HELP |
| Logging | `USB_Logging_*` mirrors to LPUART1 and a USBX CDC-ACM sink (a ring buffer drained by a thread; boot logs queue until a host opens the port) |
| Shared state | `AppState` (`Core/Inc/app_state.h`); all LED sources (UART, BLE, B1 button, GUI) go through `AppState_SetLed()` in `Core/Src/app_core.c` |

## Hardware and wiring

| Signal | Pin | Notes |
|---|---|---|
| LCD SCK / MOSI | PB10 / PB15 | SPI2, TX only |
| LCD CS / DC / RES | PG12 / PC2 / PA9 | GPIO out (`DISP_CS/DC/RES`); GPIOG needs VddIO2 (enabled in `MX_GPIO_Init`) |
| BlueNRG-2 | SPI1 SCK PA5 / MISO PA6 / MOSI PA7, CS PC0, RST PF13, IRQ PA3 (EXTI3) | SPI1 prescaler 64 (`custom_bus.c`) |
| Console | LPUART1 PG8 (RX) / PC1 (TX), 115200 8N1 | USB-UART adapter: adapter RX to PC1, adapter TX to PG8, GND |
| LEDs | LD1 = PC7, LD3 = PB14 | `LED0` = LD1, `LED1` = LD3 |
| User button | B1 = PC13 | toggles LD1 |

`I2C1` (PB7/PB8) is configured but unused. PA9 is USB VBUS-sense on some boards; check before
using it as LCD RES.

## Build, flash, test, lint

```
cmake --preset Debug && cmake --build --preset Debug     # output: build/Debug/touchgfx_test.elf
scripts/unit-test.sh                                      # unit tests on the PC, no board needed
scripts/format.sh check && scripts/lint.sh                # format and MISRA C:2012 lint
```

Toolchain: `cmake/gcc-arm-none-eabi.cmake` (needs `arm-none-eabi-gcc`, CMake >= 3.22, Ninja).
User sources are listed in the root `CMakeLists.txt` (`target_sources`). Flash the ELF with
STM32CubeProgrammer, or:

```
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program build/Debug/touchgfx_test.elf verify reset exit"
```

Open the console before flashing so the boot logs are not missed. Coding and lint rules are in
`CODING_CONVENTIONS.md` and `misra/README.md`.

## UART console

Type a command and press Enter:

```
LED0 ON | OFF | TOGGLE | BLINK | FAST      (same for LED1)
STATUS      LED states
BLE         BLE status code
HELP
```

Log lines look like `[INFO] [ssss.mmm] ...`. The Monitor thread logs a status line every 5 s:
`HEALTH ThreadX=OK USBX=<ACTIVE|WAIT> TouchGFX_FPS=<n> BLE=<n> Display=<n> DisplayFaults=<n> Heartbeat=<n>`.

## BLE GATT demo service

Advertised as `Nucleo-BLE-Demo` (use nRF Connect). Service `8a7c0001-4c3e-4e2b-9d4a-0b5f00c0ffee`:

| Characteristic | Access | Content |
|---|---|---|
| `...0002` LED control | read / write | 1 byte bitmask (bit0 = LD1, bit1 = LD3) |
| `...0003` status | read / notify | 6 bytes: `ble_status`, `led_mask`, heartbeat (u32 little-endian); notified every second |

## Bring-up checklist

Expected behaviour from the original bring-up notes; none of it has been verified on hardware.
Work through the stages in order and stop at the first failure.

1. **Boot and display.** The panel shows "Live Status" with four rows and two LED buttons, and
   the console prints `Console ready. Type HELP.`

   | Symptom | Try |
   |---|---|
   | Blank or white screen | CS/DC/RES wiring, 3.3 V supply |
   | Garbled or shifted image | Lower the SPI2 speed via the prescaler in `MX_SPI2_Init()` (`Core/Src/main.c`) |
   | Colours swapped | Check the ST7789 MADCTL and pixel-format initialisation in `App/waveshare_driver/Src/waveshare_driver.c` |
   | No console output at all | Attach a debugger and look for a HardFault (80 MHz clock setup, ThreadX start) |

2. **Screen values.** Uptime ticks every second, FPS is above 0 while the UI redraws, and the
   heartbeat rises by 1 every 5 s (visible in the `HEALTH` line).
3. **Console and LEDs.** `HELP` lists the commands; `LED0 ON` lights LD1 and the on-screen
   button follows within about 200 ms; `LED1 TOGGLE` toggles LD3; `LED0 BLINK` / `LED0 FAST`
   blink slowly / quickly; `STATUS` prints the LED states; `BLE` prints `BLE status: 3`
   (advertising). The blue user button B1 toggles LD1 and the on-screen button.
4. **BLE (nRF Connect on a phone).**
   - Connect to `Nucleo-BLE-Demo`; the screen shows Connected and the console logs `BLE connected`.
   - Write `03` to `...0002`: both LEDs and both on-screen buttons turn on; `00` turns them off.
     After `LED0 ON` on the console, reading `...0002` follows the state.
   - Subscribe to `...0003`: one 6-byte notification per second (byte 0 = `04` connected,
     byte 1 = LED mask, bytes 2-5 = heartbeat, little-endian).
   - Disconnect: the console logs `re-advertising` and the screen returns to Advertising.
   - `BLE bring-up failed (is the BlueNRG-2 shield fitted?)` in the log means: check shield
     seating and the CS / RST / IRQ wiring.
5. **USB CDC log (optional).** Connect the Nucleo USB user port to the PC and open the new
   serial port (`/dev/ttyACM*`). The same log lines as on LPUART1 appear once the port is open
   (the sink waits for DTR); the `USBX=` field in the `HEALTH` line shows `ACTIVE`.
6. **Stress (5 minutes).** `LED0 FAST` with BLE connected; the screen must not freeze and FPS
   must not drop to 0.

Not testable yet: the on-screen buttons need a touch controller (`STM32TouchController.cpp` is
a stub), so they only show state.

## Architecture

Threads (ThreadX, 100 Hz tick, lower number = higher priority):

```
 tx_timer "TouchGFX VSync" (20 ms) --> touchgfxSignalVSync()
                                             |
 TouchGFX thread (5) <-- vsync queue --------+
   render -> TouchGFXHAL::flushFrameBuffer(rect)
        -> WS169_FlushRectRGB565()  (full-width dirty rows)
        -> SPI2 16-bit DMA, thread sleeps on a semaphore until DMA is done

 BLE thread (10)      BLE_App_Init / advertise, then hci_user_evt_proc() every 10 ms
 UART cmd thread (11) UART_CMD_Process + LED blink service every 20 ms
 Monitor thread (12)  heartbeat log every 5 s
 USB CDC log drain (14) UsbCdcLog: drains the log ring buffer to the host every 50 ms
```

Data path to the panel:

```
TouchGFX (LCD16bpp, framebuffer 280x240x2 B in RAM, section TouchGFX_Framebuffer)
   |  blits/fills via DMA2D (STM32DMA.cpp)
   v
WS169_FlushRectRGB565 -- window set (8-bit SPI frames, CS/DC on PG12/PC2)
   v  switch SPI2 to 16-bit frames (MSB first = ST7789 byte order, no swap)
DMA1 Ch5 (SPI2_TX, halfword) -- 40 MHz --> ST7789V2 (PB10 SCK, PB15 MOSI)
```

Clocks: MSI 4 MHz -> PLL (M=1, N=40, R=2) = 80 MHz SYSCLK/HCLK/PCLK1/PCLK2, flash latency 4.
ThreadX `SYSTEM_CLOCK` = 80 MHz. SPI2 = 40 MHz (prescaler 2), SPI1 (BLE) prescaler 64.

Interrupts:

| IRQ | Priority | Purpose |
|---|---|---|
| EXTI3 (PA3) | 0 | BlueNRG-2 data ready |
| DMA1_Channel5 | 5 | display pixel DMA complete |
| LPUART1 | 0 | console RX (byte at a time) |
| DMA2D | 9 | Chrom-ART complete |
| OTG_FS | 7 | USB device |
| TIM1 update | 15 | HAL tick |

Peripherals: SPI2 (LCD), SPI1 (BlueNRG-2), LPUART1 (console), DMA1, DMA2D, CRC, I2C1 (unused).

## Source map

| File | Role |
|---|---|
| `App/waveshare_driver/**` | ST7789V2 driver and board SPI2/pin setup; pin macros `DISP_*` in `Core/Inc/main.h` |
| `TouchGFX/target/TouchGFXHAL.cpp` | inits the panel, flushes dirty rows, `touchgfxSignalVSync()` |
| `TouchGFX/gui/**` | screens, presenters, `Model`, custom widgets (user-owned); status texts from `App/logic/ui_format`, model polling from `App/logic/change_detect` |
| `Core/Src/app_threadx.c` | VSYNC timer, threads, UART RX callbacks |
| `Core/Src/dma2d.c` | DMA2D init for TouchGFX |
| `Core/Src/app_core.c` | shared `AppState`, LED/console ownership, user button (`App/logic/debounce`, `App/logic/counters`) |
| `Core/Src/ble_app.c` | BlueNRG-2 init, advertising, event dispatch, GATT service (byte layouts in `App/logic/ble_codec`) |
| `Core/Src/uart_commands.c` | console line handling and LED pins (parsing and replies in `App/logic/cmd_parse`, LED states in `App/logic/led_fsm`) |
| `App/logic/**` | pure, unit-tested logic (`ble_codec`, `change_detect`, `cmd_parse`, `counters`, `debounce`, `led_fsm`, `log_format`, `ring`, `text_writer`, `timeouts`, `uart_line`, `ui_format`, `ws169_geometry`); standard library only |
| `Tests/**` | unit tests (Unity) for `App/logic`; run `scripts/unit-test.sh` |
| `Core/Src/usb_logging.c`, `usb_cdc_log.c` | log sink (LPUART1 + USB CDC, buffered by `App/logic/ring`, formatted by `App/logic/log_format`) |

## Changing the display pins or speed

Edit `DISP_CS/DC/RES_*` in `Core/Inc/main.h` and the GPIO init in `MX_GPIO_Init`; SPI2 pins are
in `stm32l4xx_hal_msp.c`. To lower the SPI speed, change the prescaler in `MX_SPI2_Init()` in
`Core/Src/main.c`.

## Regeneration (CubeMX, then TouchGFX Designer)

The code is arranged so regeneration is safe, but follow this order and review the diff.

**Hand edits that regeneration can overwrite** (outside USER CODE blocks; re-check after any
regeneration):

- `TouchGFX/target/generated/STM32DMA.cpp/.hpp`: DMA2D version from TouchGFX 4.22 with its `paint`
  namespace removed (4.26 provides it).
- `TouchGFXGeneratedHAL.cpp`: DMA2D IRQ enable/disable/priority, framebuffer 280x240, Paint includes kept.
- `TouchGFXConfiguration.cpp`: HAL size 280x240; `touchgfx_test.touchgfx` resolution 280x240;
  generated GUI backgrounds were resized by hand.
- `STM32L496XX_FLASH.ld`: `TouchGFX_Framebuffer` NOLOAD section (the build uses
  `linker/STM32L496XX_FLASH_app.ld`).
- `Core/Src/custom_bus.c`: SPI1 8-bit, prescaler 64.
- `Core/Src/tx_initialize_low_level.S`: `SYSTEM_CLOCK` 80 MHz.
- `main.c`, `stm32l4xx_hal_msp.c`, `stm32l4xx_it.c`: clock, DMA init, GPIOG VddIO2, DMA/LPUART/EXTI/DMA2D handlers.

**1. Before you start:** `git add -A && git commit -m "before regeneration"` so any problem is one
`git diff` away. Keep `ProjectManager.KeepUserCode=true` (already set).

**2. STM32CubeMX:** open `touchgfx_test.ioc`. If it complains about the USB entries, toggle
`USB_OTG_FS` once (Connectivity, Device Only). Verify:

- Clock: MSI 4 MHz, PLL to 80 MHz SYSCLK; USB clock source HSI48.
- Pins: PB10/PB15 SPI2 TX-only, PG12/PC2/PA9 outputs (`DISP_CS/DC/RES`), PC7/PB14 LD1/LD3, PA11/PA12 USB.
- DMA: SPI2_TX on DMA1 Ch5, halfword. DMA2D enabled. NVIC priorities: DMA1 Ch5 = 5, LPUART1 = 0, EXTI3 = 0, DMA2D = 9, OTG_FS = 7.
- TouchGFX pack: display 280 x 240. USBX pack: Device, CDC ACM.
- Project Manager: toolchain **CMake**, then **Generate Code**.

**3. TouchGFX Designer:** open `TouchGFX/touchgfx_test.touchgfx`, confirm resolution 280x240,
then **Generate Code**. `Screen1` is built in code and is not touched by Designer.

**4. Review the diff and fix leftovers:**

| Item | Expected / action |
|---|---|
| `cmake/stm32cubemx/CMakeLists.txt` | Lists `dma2d.c`, `stm32l4xx_hal_dma2d.c`, and the USB/PCD sources. The root `CMakeLists.txt` adds the dma2d files only if missing; on "multiple definition", remove the root entry. |
| `stm32l4xx_it.c` | CubeMX adds strong DMA1_Ch5 / DMA2D / LPUART1 / OTG_FS handlers; the hand-written ones are `__weak` inside `USER CODE 1`, so they lose. `EXTI3_IRQHandler` must remain (BlueNRG). |
| `Core/Src/tx_initialize_low_level.S` | `SYSTEM_CLOCK` must be `80000000`; if it reverted to 4000000, fix it. |
| `stm32l4xx_hal_conf.h` | `HAL_PCD_MODULE_ENABLED` and `HAL_DMA2D_MODULE_ENABLED` must be on. |
| USBX sources | `ux_dcd_stm32_*` and HAL PCD must be in the build; if the pack did not add them, add the USBX STM32 device-controller sources. |
| `TouchGFX/target/generated/*` | Regenerated by the generator (DMA2D `STM32DMA`, 280x240 framebuffer). The hand-edited copies are simply replaced. |
| USBX memory | `app_azure_rtos_config.h` and `app_usbx_device.h` keep their overrides (18 KB stack, 22 KB pool) because they sit in USER CODE blocks; verify. |
| Linker | The app uses `linker/STM32L496XX_FLASH_app.ld`; if CubeMX changes the `.ld` name, update the `string(REPLACE ...)` line in the root CMake. |

**5. Build** with `cmake --build --preset Debug`.

**App-owned (never regenerated):** `App/waveshare_driver/**`, `App/logic/**`, `Core/Src/{ble_app,uart_commands,usb_logging,usb_cdc_log,app_core}.c`, `Core/Inc/{app_state,app_core,usb_cdc_log,...}.h`, `linker/*.ld`, `TouchGFX/target/TouchGFXHAL.cpp`, `TouchGFX/gui/**`, root `CMakeLists.txt`. **Hooks in generated files** sit in USER CODE blocks: `app_threadx.c`, `stm32l4xx_it.c`, `ux_device_cdc_acm.c`, `app_azure_rtos_config.h`, `app_usbx_device.h`.

## Known risks and gaps (as documented, not re-verified)

- SPI2 runs at 40 MHz (80 MHz / 2); if the image is garbled, lower the speed via the prescaler.
- DMA2D support uses the TouchGFX 4.22 `STM32DMA` with the 4.26 framework.
- BLE has not been tried against a real BlueNRG-2.
- UI asset and file names still say `240x240` (they are 280x240 now).
- No touch controller is wired (`STM32TouchController.cpp` is a stub), so the on-screen LED buttons only show state.
- USB CDC logging (USBX device, `usb_cdc_log.c`) has not been verified on hardware.
