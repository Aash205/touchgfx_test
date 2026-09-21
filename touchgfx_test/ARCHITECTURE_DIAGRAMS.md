# Architecture

## Threads (ThreadX, 100 Hz tick, lower number = higher priority)

```
 tx_timer "TouchGFX VSync" (20 ms) ──► touchgfxSignalVSync()
                                             │
 TouchGFX thread (5) ◄── vsync queue ────────┘
   render → TouchGFXHAL::flushFrameBuffer(rect)
        → Display_FlushRectRGB565()  (full-width dirty rows)
        → SPI2 16-bit DMA, thread sleeps on semaphore until DMA done

 BLE thread (10)      BLE_App_Init / advertise, then hci_user_evt_proc() every 10 ms
 UART cmd thread (11) UART_CMD_Process + LED blink service every 20 ms
 Monitor thread (12)  heartbeat log every 5 s
```

## Data path to the panel

```
TouchGFX (LCD16bpp, framebuffer 280x240x2 B in RAM, section TouchGFX_Framebuffer)
   │  blits/fills via DMA2D (STM32DMA.cpp)
   ▼
Display_FlushRectRGB565 ── window set (8-bit SPI frames, CS/DC on PG12/PA8)
   ▼  switch SPI2 to 16-bit frames (MSB first = ST7789 byte order, no swap)
DMA1 Ch5 (SPI2_TX, halfword) ── 40 MHz ──► ST7789V2 (PB10 SCK, PB15 MOSI)
```

## Clocks

MSI 4 MHz → PLL (M=1, N=40, R=2) = 80 MHz SYSCLK/HCLK/PCLK1/PCLK2, flash latency 4.
ThreadX `SYSTEM_CLOCK` = 80 MHz. I2C1 timing 0x10909CEC. SPI2 = 40 MHz, SPI1 (BLE) = 5 MHz.

## Interrupts

| IRQ | Priority | Purpose |
|---|---|---|
| EXTI9_5 (PE8) | 0 | BlueNRG-2 data ready |
| DMA1_Channel5 | 5 | display pixel DMA complete |
| LPUART1 | 6 | console RX (byte at a time) |
| DMA2D | 9 | Chrom-ART complete |
| TIM1 update | 15 | HAL tick |

## Peripherals

SPI2 (LCD) · SPI1 (BlueNRG-2) · LPUART1 (console) · DMA1 · DMA2D · CRC · I2C1 (unused).
