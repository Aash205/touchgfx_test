```
╔═══════════════════════════════════════════════════════════════════════════════╗
║                   STM32L496 NUCLEO MULTI-FEATURE DEMO                        ║
║              ThreadX + TouchGFX + BLE + USBX + OLED Display                   ║
╚═══════════════════════════════════════════════════════════════════════════════╝

┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃                        STM32L496 MCU BOARD                                ┃
┃  ┌──────────────────────────────────────────────────────────────────┐   ┃
┃  │  ThreadX RTOS Kernel                                             │   ┃
┃  ├──────────────────────────────────────────────────────────────────┤   ┃
┃  │                                                                  │   ┃
┃  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │   ┃
┃  │  │   BLE THREAD    │  │  UART THREAD    │  │ MONITOR THREAD  │ │   ┃
┃  │  │   (10 Hz)       │  │   (20 Hz)       │  │   (1 Hz)        │ │   ┃
┃  │  │  Priority: +2   │  │  Priority: +1   │  │  Priority: 0    │ │   ┃
┃  │  │                 │  │                 │  │                 │ │   ┃
┃  │  │ • Init BLE      │  │ • Parse UART    │  │ • Track uptime  │ │   ┃
┃  │  │ • Advertise     │  │ • LED control   │  │ • Log status    │ │   ┃
┃  │  │ • Connection    │  │ • Cmd feedback  │  │ • Monitor tasks │ │   ┃
┃  │  │ • Status track  │  │ • Update OLED   │  │ • System info   │ │   ┃
┃  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘ │   ┃
┃  │           │                    │                    │           │   ┃
┃  │  ┌────────┴────────────────────┴────────────────────┴────────┐  │   ┃
┃  │  │  All threads update OLED display with real-time status    │  │   ┃
┃  │  └────────┬────────────────────────────────────────────────┬─┘  │   ┃
┃  │           │                                                │    │   ┃
┃  │  ┌────────▼──────────────────────────────────────────────▼───┐ │   ┃
┃  │  │              TouchGFX THREAD (100 Hz)                     │ │   ┃
┃  │  │              Priority: -1 (Lowest)                        │ │   ┃
┃  │  │  • GUI rendering                                          │ │   ┃
┃  │  │  • Screen updates                                         │ │   ┃
┃  │  │  • Touch input handling                                   │ │   ┃
┃  │  └────────┬──────────────────────────────────────────────────┘ │   ┃
┃  │           │                                                    │   ┃
┃  └───────────┼────────────────────────────────────────────────────┘   ┃
┃              │                                                         ┃
┃  ┌───────────┴────────────────────────────────────────────────────┐   ┃
┃  │          Shared Resources & Communication                     │   ┃
┃  │  ┌──────────────────────────────────────────────────────────┐ │   ┃
┃  │  │ • TX_SEMAPHORE: semaphore_ble, semaphore_uart,          │ │   ┃
┃  │  │                 semaphore_monitor                         │ │   ┃
┃  │  │ • OLED_HandleTypeDef: oled_handle (concurrent access)   │ │   ┃
┃  │  │ • UART_CommandTypeDef: uart_cmd_handler                 │ │   ┃
┃  │  │ • BLE_AppHandleTypeDef: ble_handle                       │ │   ┃
┃  │  │ • USB_LoggingTypeDef: usb_logging                        │ │   ┃
┃  │  └──────────────────────────────────────────────────────────┘ │   ┃
┃  └──────────────────────────────────────────────────────────────┘   ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
                           ↓     ↓     ↓     ↓
        ┌──────────────┬──────┴──┬──────┴──┬──────┴──┬──────────┐
        │              │         │         │         │          │
        ↓              ↓         ↓         ↓         ↓          ↓
    ┌────────┐     ┌────────┐ ┌───────┐ ┌───────┐ ┌────────┐ ┌───────┐
    │  BLE   │     │ OLED   │ │ UART  │ │ GPIO  │ │  USB   │ │ USB   │
    │ Stack  │     │ Driver │ │ Cmds  │ │ LEDs  │ │ Logs   │ │ Diag  │
    │        │     │ (I2C)  │ │       │ │       │ │        │ │       │
    │        │     │        │ │       │ │       │ │        │ │       │
    └────┬───┘     └───┬────┘ └───┬───┘ └───┬───┘ └───┬────┘ └───┬───┘
         │             │          │         │         │          │
         ▼             ▼          ▼         ▼         ▼          ▼
    ┌──────────┐  ┌──────────┐ ┌───────┐ ┌────────┐ ┌────────┐ ┌──────┐
    │BlueNRG-2 │  │SSD1306   │ │LPUART1│ │GPIO PA5│ │USB Dev │ │Logs  │
    │ Module   │  │Display   │ │115200 │ │(LED0)  │ │CDC ACM │ │Port  │
    │(SPI/I2C) │  │128x64px  │ │       │ │        │ │        │ │      │
    │          │  │          │ │       │ │        │ │        │ │      │
    └──────────┘  └──────────┘ └───────┘ └────────┘ └────────┘ └──────┘
         │             │          │         │         │          │
         └─────────────┴──────────┴─────────┴─────────┴──────────┘
                              │
                              ▼
                    EXTERNAL CONNECTIONS
                    ├─ BLE Device (phone, tablet)
                    ├─ OLED Display (Waveshare)
                    ├─ Serial Terminal (UART)
                    ├─ USB Serial Port (Logs)
                    └─ Power Supply (USB or external)


╔═══════════════════════════════════════════════════════════════════════════════╗
║                        DATA FLOW EXAMPLES                                     ║
╚═══════════════════════════════════════════════════════════════════════════════╝

SCENARIO 1: User sends UART command "LED0 ON"
────────────────────────────────────────────
User (Terminal) 
    │
    └──► "LED0 ON\r\n" ──► LPUART1 RX ──► UART CMD Thread
                                              │
                                              ├──► Parse command
                                              ├──► GPIO PA5 = SET
                                              ├──► OLED update: "LED0: ON"
                                              └──► USB_Log: "[INFO] LED0 ON"
                                                      │
                                                      ├──► LED turns ON
                                                      ├──► Display shows status
                                                      └──► User sees log

SCENARIO 2: BLE device connects
─────────────────────────────────
Remote BLE Device
    │
    └──► Connect Request ──► BlueNRG-2 ──► HCI Event ──► BLE Thread
                                               │
                                               ├──► Update status
                                               ├──► OLED: "Connected"
                                               └──► USB_Log: "[INFO] Connected"
                                                       │
                                                       ├──► BLE shows connection
                                                       ├──► Display indicates link
                                                       └──► Diagnostics available

SCENARIO 3: System Status Update
─────────────────────────────────
Monitor Thread (1Hz)
    │
    └──► Check System Status ──► Collect metrics
                                    │
                                    ├──► Uptime: 125 seconds
                                    ├──► BLE Status: Connected
                                    ├──► LED Count: 1
                                    │
                                    └──► USB_Log: "[INFO] System Status"
                                            │
                                            ├──► Timestamped
                                            ├──► Formatted output
                                            └──► User diagnostics


╔═══════════════════════════════════════════════════════════════════════════════╗
║                    MEMORY & RESOURCE ALLOCATION                              ║
╚═══════════════════════════════════════════════════════════════════════════════╝

STM32L496 Resources:
├─ Flash Memory: 1 MB (plenty for this application)
├─ RAM: 128 KB
│   ├─ ThreadX Kernel: ~2 KB
│   ├─ Thread Stacks:
│   │   ├─ BLE Thread: 1 KB
│   │   ├─ UART Thread: 1 KB
│   │   ├─ Monitor Thread: 1 KB
│   │   └─ TouchGFX Thread: 4 KB
│   ├─ OLED Buffer: 1 KB
│   ├─ USB Logging Buffer: 512 bytes
│   └─ Remaining for heap: ~110 KB
│
├─ I2C1: OLED Display (100 kHz)
├─ LPUART1: UART Commands (115200 baud)
├─ GPIO PA5: LED output
├─ SPI/I2C: BLE communication
└─ USB: Diagnostics logging


╔═══════════════════════════════════════════════════════════════════════════════╗
║                        PIN CONFIGURATION                                     ║
╚═══════════════════════════════════════════════════════════════════════════════╝

STM32L496 Nucleo Board Pins
┌──────────────────────────────────────┐
│ Component      │ Pin(s)   │ Port     │
├──────────────────────────────────────┤
│ OLED SCL       │ PB6      │ I2C1_SCL │
│ OLED SDA       │ PB7      │ I2C1_SDA │
├──────────────────────────────────────┤
│ UART TX        │ PA3      │ LPUART_TX│
│ UART RX        │ PA2      │ LPUART_RX│
├──────────────────────────────────────┤
│ User LED       │ PA5      │ GPIO     │
├──────────────────────────────────────┤
│ BLE Chip Select│ PC2      │ GPIO     │
│ BLE Reset      │ PF13     │ GPIO     │
├──────────────────────────────────────┤
│ USB D+         │ PA12     │ USB      │
│ USB D-         │ PA11     │ USB      │
└──────────────────────────────────────┘


╔═══════════════════════════════════════════════════════════════════════════════╗
║                        TIMING DIAGRAM                                        ║
╚═══════════════════════════════════════════════════════════════════════════════╝

Time (ms)  │ BLE Thread     │ UART Thread    │ Monitor        │ TouchGFX
───────────┼────────────────┼────────────────┼────────────────┼─────────
0          │ START          │ START          │ START          │ START
           │                │                │                │
10         │                │                │                │ RUN (10ms)
20         │                │ RUN (50ms)     │                │ RUN (10ms)
30         │                │                │                │ RUN (10ms)
40         │                │                │                │ RUN (10ms)
50         │                │ DONE           │                │ RUN (10ms)
           │                │                │                │
100        │ RUN (100ms)    │                │                │ RUN (10ms)
           │ OLED UPDATE    │                │                │
           │ BLE CHECK      │                │                │
150        │                │ RUN (50ms)     │                │
           │                │ CMD PROCESS    │                │
200        │                │                │                │ RUN (10ms)
           │                │                │                │
300        │ RUN (100ms)    │                │                │
           │                │                │                │
500        │                │                │ RUN (1000ms)   │
           │                │                │ SYSTEM STATUS  │
           │                │                │ USB LOG        │
...        │ ... (10Hz)     │ ... (20Hz)     │ ... (1Hz)      │ ... (100Hz)
───────────┴────────────────┴────────────────┴────────────────┴─────────


╔═══════════════════════════════════════════════════════════════════════════════╗
║                    SYSTEM STATE MACHINE                                      ║
╚═══════════════════════════════════════════════════════════════════════════════╝

BLE State Machine:
┌─────────┐     Init      ┌────────────┐    Start    ┌──────────────┐
│  IDLE   │─────────────→ │INITIALIZED │─────────────→ │ ADVERTISING  │
└─────────┘               └────────────┘               └────┬─────────┘
                                ▲                          │
                                │                     Connect
                                │                          │
                              Stop                         ▼
                            Advertise             ┌──────────────┐
                                │                 │  CONNECTED   │
                                └─────────────────┤              │
                                                  │   (Optional) │
                                                  │    Pairing   │
                                                  │      │       │
                                                  │      ▼       │
                                                  │   PAIRED     │
                                                  └──────────────┘

UART Command State Machine:
RX_IDLE ──[Char Received]──→ RX_BUFFER ──[Enter Pressed]──→ PROCESS
  ▲                            │                              │
  │                            │                              ▼
  │                            │                         EXECUTE
  └────────────────────────────┴──────────────────────────────┘
                        [Ready for next command]


╔═══════════════════════════════════════════════════════════════════════════════╗
║                    FILE DEPENDENCY GRAPH                                     ║
╚═══════════════════════════════════════════════════════════════════════════════╝

main.c
  ├─ app_threadx.h
  │   └─ tx_api.h (ThreadX)
  │
  └─ main.h
      ├─ stm32l4xx_hal.h
      ├─ hci_tl_interface.h
      └─ [HAL drivers]

app_threadx.c
  ├─ app_threadx.h
  ├─ oled_driver.h
  │   └─ stm32l4xx_hal.h
  ├─ ble_app.h
  │   └─ stm32l4xx_hal.h
  ├─ uart_commands.h
  │   └─ stm32l4xx_hal.h
  └─ usb_logging.h
      └─ stm32l4xx_hal.h

oled_driver.c ──→ oled_driver.h
ble_app.c ──→ ble_app.h
uart_commands.c ──→ uart_commands.h
usb_logging.c ──→ usb_logging.h
app_tests.c ──→ app_tests.h + all component headers


╔═══════════════════════════════════════════════════════════════════════════════╗
║                    QUICK REFERENCE: THREAD PRIORITIES                        ║
╚═══════════════════════════════════════════════════════════════════════════════╝

Higher Priority = Runs more frequently when other threads are ready

Priority Level │ Thread          │ Hz   │ Purpose
────────────────┼─────────────────┼──────┼──────────────────────────
PRIORITY + 2   │ BLE Thread      │ 10   │ Wireless event processing
PRIORITY + 1   │ UART CMD Thread │ 20   │ User command handling
PRIORITY (0)   │ Monitor Thread  │ 1    │ System monitoring
PRIORITY - 1   │ TouchGFX Thread │ 100  │ GUI rendering

Lower Priority = Can be preempted by higher priority threads
```

## Legend

```
┌─────────┐  ┬─────────┬  ╔═════════╗  ┏━━━━━━━━━┓
│ Element │  │ Heading │  ║ Title ║  ┗━━━━━━━━━┛
└─────────┘  └─────────┘  ╚═════════╝
   Box      Separator      Header      Section

  ──→  Flow/Connection
  ──┤  Junction
  ──┬  Split
  ▼ │  Down
  └─┘  Corner
```
