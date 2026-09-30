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
| RTOS | ThreadX: TouchGFX (prio 5), BLE (10), UART command console (11), Monitor heartbeat (12); 20 ms VSYNC timer |
| BLE | BlueNRG-2 peripheral on SPI1 (5 MHz), advertises `Nucleo-BLE-Demo`, re-advertises on disconnect |
| Console | LPUART1 115200 8N1: LED control, status, BLE status, HELP |
| Logging | `USB_Logging_*` mirrors to LPUART1 and a USB CDC sink (inert until the USB device controller is generated) |
| Shared state | `AppState` (`Core/Inc/app_state.h`); all LED sources (UART, BLE, B1 button, GUI) go through `AppState_SetLed()` in `Core/Src/app_core.c` |

## Hardware and wiring

| Signal | Pin | Notes |
|---|---|---|
| LCD SCK / MOSI | PB10 / PB15 | SPI2, TX only |
| LCD CS / DC / RES | PG12 / PA8 / PA9 | GPIO out (`DISP_CS/DC/RES`); GPIOG needs VddIO2 (enabled in `MX_GPIO_Init`) |
| BlueNRG-2 | SPI1 PA1/PA6/PA7, CS PC2, RST PF13, IRQ PE8 | SPI1 at 5 MHz |
| Console | LPUART1 PC0 (RX) / PC1 (TX), 115200 8N1 | USB-UART adapter: adapter RX to PC1, adapter TX to PC0, GND |
| LEDs | LD1 = PC7, LD3 = PB14 | `LED0` = LD1, `LED1` = LD3 |
| User button | B1 = PC13 | toggles LD1 |

`I2C1` (PB7/PB8) is configured but unused. PA9 is USB VBUS-sense on some boards; check before
using it as LCD RES.

## Build, flash, test, lint

```
cmake --preset Debug && cmake --build --preset Debug     # output: build/Debug/touchgfx_test.elf
scripts/host-test.sh                                      # unit tests on the PC, no board needed
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

Log lines look like `[INFO] [ss.mmm] ...`; the Monitor thread logs a heartbeat every 5 s.

## BLE GATT demo service

Advertised as `Nucleo-BLE-Demo` (use nRF Connect). Service `8a7c0001-4c3e-4e2b-9d4a-0b5f00c0ffee`:

| Characteristic | Access | Content |
|---|---|---|
| `...0002` LED control | read / write | 1 byte bitmask (bit0 = LD1, bit1 = LD3) |
| `...0003` status | read / notify | 6 bytes: `ble_status`, `led_mask`, heartbeat (u32 little-endian); notified every second |

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
ThreadX `SYSTEM_CLOCK` = 80 MHz. SPI2 = 40 MHz, SPI1 (BLE) = 5 MHz.

Interrupts:

| IRQ | Priority | Purpose |
|---|---|---|
| EXTI9_5 (PE8) | 0 | BlueNRG-2 data ready |
| DMA1_Channel5 | 5 | display pixel DMA complete |
| LPUART1 | 6 | console RX (byte at a time) |
| DMA2D | 9 | Chrom-ART complete |
| TIM1 update | 15 | HAL tick |

Peripherals: SPI2 (LCD), SPI1 (BlueNRG-2), LPUART1 (console), DMA1, DMA2D, CRC, I2C1 (unused).

## Source map

| File | Role |
|---|---|
| `App/waveshare_driver/**` | ST7789V2 driver and board SPI2/pin setup; pin macros `DISP_*` in `Core/Inc/main.h` |
| `TouchGFX/target/TouchGFXHAL.cpp` | inits the panel, flushes dirty rows, `touchgfxSignalVSync()` |
| `TouchGFX/gui/**` | screens, presenters, `Model`, custom widgets (user-owned) |
| `Core/Src/app_threadx.c` | VSYNC timer, threads, UART RX callbacks |
| `Core/Src/app_core.c` | shared `AppState`, LED/console ownership, user button |
| `Core/Src/ble_app.c` | BlueNRG-2 init, advertising, event dispatch, GATT service |
| `Core/Src/uart_commands.c`, `uart_line.c` | console command parser, line assembly |
| `Core/Src/usb_logging.c`, `usb_cdc_log.c` | log sink (LPUART1 + USB CDC ring buffer) |

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
- `TouchGFXGeneratedHAL.cpp`: DMA2D IRQ enable/disable/priority, framebuffer 280x240.
- `TouchGFXConfiguration.cpp`: HAL size 280x240; `touchgfx_test.touchgfx` resolution 280x240;
  generated GUI backgrounds were resized by hand.
- `STM32L496XX_FLASH.ld`: `TouchGFX_Framebuffer` NOLOAD section (the build uses
  `linker/STM32L496XX_FLASH_app.ld`).
- `Core/Src/custom_bus.c`: SPI1 8-bit, prescaler 16.
- `Core/Src/tx_initialize_low_level.S`: `SYSTEM_CLOCK` 80 MHz.
- `main.c`, `stm32l4xx_hal_msp.c`, `stm32l4xx_it.c`: clock, DMA init, GPIOG VddIO2, DMA/LPUART/EXTI/DMA2D handlers.

**1. Before you start:** `git add -A && git commit -m "before regeneration"` so any problem is one
`git diff` away. Keep `ProjectManager.KeepUserCode=true` (already set).

**2. STM32CubeMX:** open `touchgfx_test.ioc`. If it complains about the USB entries, toggle
`USB_OTG_FS` once (Connectivity, Device Only). Verify:

- Clock: MSI 4 MHz, PLL to 80 MHz SYSCLK; USB clock source HSI48.
- Pins: PB10/PB15 SPI2 TX-only, PG12/PA8/PA9 outputs (`DISP_CS/DC/RES`), PC7/PB14 LD1/LD3, PA11/PA12 USB.
- DMA: SPI2_TX on DMA1 Ch5, halfword. DMA2D enabled. NVIC priorities: DMA1 Ch5 = 5, LPUART1 = 6, DMA2D = 9, OTG_FS = 7.
- TouchGFX pack: display 280 x 240. USBX pack: Device, CDC ACM.
- Project Manager: toolchain **CMake**, then **Generate Code**.

**3. TouchGFX Designer:** open `TouchGFX/touchgfx_test.touchgfx`, confirm resolution 280x240,
then **Generate Code**. `Screen1` is built in code and is not touched by Designer.

**4. Review the diff and fix leftovers:**

| Item | Expected / action |
|---|---|
| `cmake/stm32cubemx/CMakeLists.txt` | Lists `dma2d.c`, `stm32l4xx_hal_dma2d.c`, and the USB/PCD sources. The root `CMakeLists.txt` adds the dma2d files only if missing; on "multiple definition", remove the root entry. |
| `stm32l4xx_it.c` | CubeMX adds strong DMA1_Ch5 / DMA2D / LPUART1 / OTG_FS handlers; the hand-written ones are `__weak` inside `USER CODE 1`, so they lose. `EXTI9_5_IRQHandler` must remain (BlueNRG). |
| `Core/Src/tx_initialize_low_level.S` | `SYSTEM_CLOCK` must be `80000000`; if it reverted to 4000000, fix it. |
| `stm32l4xx_hal_conf.h` | `HAL_PCD_MODULE_ENABLED` and `HAL_DMA2D_MODULE_ENABLED` must be on. |
| USBX sources | `ux_dcd_stm32_*` and HAL PCD must be in the build; if the pack did not add them, add the USBX STM32 device-controller sources. |
| `TouchGFX/target/generated/*` | Regenerated by the generator (DMA2D `STM32DMA`, 280x240 framebuffer). The hand-edited copies are simply replaced. |
| USBX memory | `app_azure_rtos_config.h` and `app_usbx_device.h` keep their overrides (18 KB stack, 22 KB pool) because they sit in USER CODE blocks; verify. |
| Linker | The app uses `linker/STM32L496XX_FLASH_app.ld`; if CubeMX changes the `.ld` name, update the `string(REPLACE ...)` line in the root CMake. |

**5. Build** with `cmake --build --preset Debug`.

**App-owned (never regenerated):** `App/waveshare_driver/**`, `Core/Src/{ble_app,uart_commands,uart_line,usb_logging,usb_cdc_log,app_core}.c`, `Core/Inc/{app_state,app_core,usb_cdc_log,...}.h`, `linker/*.ld`, `TouchGFX/target/TouchGFXHAL.cpp`, `TouchGFX/gui/**`, root `CMakeLists.txt`. **Hooks in generated files** sit in USER CODE blocks: `app_threadx.c`, `stm32l4xx_it.c`, `ux_device_cdc_acm.c`, `app_azure_rtos_config.h`, `app_usbx_device.h`.

## Known risks and gaps (as documented, not re-verified)

- SPI2 runs at 40 MHz (80 MHz / 2); if the image is garbled, lower the speed via the prescaler.
- DMA2D support uses the TouchGFX 4.22 `STM32DMA` with the 4.26 framework.
- BLE has not been tried against a real BlueNRG-2.
- UI asset and file names still say `240x240` (they are 280x240 now).
- No touch controller is wired (`STM32TouchController.cpp` is a stub), so the on-screen LED buttons only show state.
- USB CDC logging needs the USB device controller generated; until then the sink is inert.
