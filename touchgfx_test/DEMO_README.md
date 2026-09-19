# Nucleo-L496 Multi-Feature Demo: TouchGFX + ThreadX + BLE + USBX

## Overview
This demo integrates multiple advanced features on the STM32L496 Nucleo board:
- **TouchGFX**: GUI framework for display rendering
- **ThreadX**: Real-time operating system (RTOS)
- **BLE (BlueNRG-2)**: Bluetooth Low Energy connectivity
- **USBX**: USB device communication and logging
- **Waveshare OLED Display**: I2C-based SSD1306 display
- **UART**: Command interface for LED control

## Architecture

```
┌─────────────────────────────────────────────────┐
│          ThreadX Kernel (RTOS)                  │
├─────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │ BLE Thread  │  │UART/LED Cmd │  │ Monitor │ │
│  │  (Prio+2)   │  │   (Prio+1)   │  │ (Prio)  │ │
│  └─────────────┘  └─────────────┘  └─────────┘ │
│  ┌──────────────────────────────────────────────┤
│  │    TouchGFX Thread (Prio-1)                  │
│  └──────────────────────────────────────────────┤
│                                                  │
│  ┌──────────────────────────────────────────────┤
│  │        Shared Resources (Semaphores)         │
│  └──────────────────────────────────────────────┤
└─────────────────────────────────────────────────┘
         │            │            │
         ▼            ▼            ▼
      BLE Stack    I2C/OLED    UART/LEDs
      USB Logging   Display
```

## Components

### 1. OLED Driver (`oled_driver.h/c`)
- **Type**: SSD1306 I2C Display Driver (Waveshare compatible)
- **Resolution**: 128x64 pixels
- **Interface**: I2C1
- **Features**:
  - Display initialization
  - Pixel drawing
  - Text printing
  - Line and rectangle drawing
  - Display buffer management

### 2. BLE Application (`ble_app.h/c`)
- **Stack**: BlueNRG-2
- **Features**:
  - Device initialization
  - Advertising control
  - Connection status tracking
  - Pairing status
  - Extensible for GATT services

### 3. UART Command Handler (`uart_commands.h/c`)
- **Interface**: LPUART1
- **Features**:
  - LED control commands (ON/OFF/TOGGLE)
  - LED blinking patterns
  - Status reporting
  - Interactive command line

### 4. USB Logging (`usb_logging.h/c`)
- **Interface**: USB Device (CDC ACM via USBX)
- **Features**:
  - Log level filtering (DEBUG, INFO, WARNING, ERROR, CRITICAL)
  - Timestamped logging
  - System diagnostics
  - Real-time log streaming

## ThreadX Threads

### BLE Thread (Priority: PRIORITY + 2)
- Monitors BLE connection status
- Processes BLE events
- Updates OLED with connection state
- Runs at 10Hz update rate

### UART Command Thread (Priority: PRIORITY + 1)
- Listens for UART commands
- Parses and executes LED control
- Updates LED states
- Displays command feedback on OLED
- Logs commands via USB

### Monitor Thread (Priority: PRIORITY)
- Tracks system uptime
- Logs periodic system status
- Manages diagnostic information
- Runs at 1Hz update rate

### TouchGFX Thread (Priority: PRIORITY - 1)
- Main GUI rendering loop
- Display updates
- Touch input handling
- Runs at ~100Hz

## Usage

### UART Commands
Connect to LPUART1 (USB serial if available) and send commands:

```
LED0 ON        - Turn on LED 0
LED0 OFF       - Turn off LED 0
LED0 TOGGLE    - Toggle LED 0
LED0 BLINK     - Blink LED 0
STATUS         - Display current status
HELP           - Show available commands
```

### BLE Connection
1. Power on the device
2. BLE automatically starts advertising as "Nucleo-BLE-Demo"
3. Scan from a BLE-enabled device
4. Connect to the device
5. Device will show connection status on OLED
6. USB logs will show connection events

### OLED Display
The display shows real-time status updates:
- **Line 1**: Initialization message or current mode (BLE, UART, Monitor)
- **Line 2**: Status information
  - BLE: "Advertising" / "Connected" / "Paired"
  - UART: Last command executed
  - System: Running status

## Pin Configuration

### STM32L496 Nucleo Board
| Component | Pin | Port | Interface |
|-----------|-----|------|-----------|
| OLED SDA  | PB7 | B    | I2C1      |
| OLED SCL  | PB6 | B    | I2C1      |
| UART TX   | PA3 | A    | LPUART1   |
| UART RX   | PA2 | A    | LPUART1   |
| User LED  | PA5 | A    | GPIO      |
| BLE CS    | PC2 | C    | GPIO      |
| BLE RST   | PF13| F    | GPIO      |

## System Clock
- **Oscillator**: MSI (Multi-Speed Internal)
- **Clock Range**: 4MHz (default)
- **PLL**: Disabled for this demo

## Memory Configuration
- **ThreadX Stack**: 1KB per thread
- **OLED Buffer**: 1KB (128x64 / 8)
- **Total Dynamic Memory**: ~8KB

## Logging Output

### USB Log Format
```
[LEVEL] [HH:MM:SS.mmm] Message\r\n
```

### Example Logs
```
[INFO] [00:00:00.000] System Startup - ThreadX Init
[INFO] [00:00:00.050] BLE Advertising: Nucleo-BLE-Demo
[INFO] [00:00:00.100] UART Command Handler Started
[DEBUG] [00:00:01.234] UART Command processed: LED0 ON
[INFO] [00:00:05.000] System Uptime: 5 seconds
```

## Extension Points

### Adding BLE GATT Services
1. Configure BlueNRG-2 HCI functions in `ble_app.c`
2. Add GATT service discovery
3. Implement characteristic callbacks
4. Add service data to advertising payload

### Adding More LEDs
1. Register additional LEDs in `app_threadx.c`:
   ```c
   UART_CMD_RegisterLED(&uart_cmd_handler, GPIOB, GPIO_PIN_3);
   ```
2. Update UART command parser for LED numbering

### Custom Display Updates
1. Modify thread callbacks in `app_threadx.c`
2. Use OLED functions for text/graphics
3. Call `OLED_UpdateDisplay()` after changes

### Sensor Integration
1. Create new thread for sensor reading
2. Add I2C/SPI initialization
3. Log sensor data via USB
4. Display on OLED

## Compilation & Build

```bash
# Navigate to project directory
cd touchgfx_test

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Flash
cmake --build . --target flash
```

## Debugging
- Set breakpoints in thread functions
- Monitor via USB logging output
- Check OLED display for real-time status
- Use UART commands to test LED functionality

## Notes
- BLE Stack integration requires HCI command implementation
- USBX CDC ACM transport layer needs configuration
- TouchGFX rendering should be synchronized with main GUI thread
- I2C frequency should be set appropriately (default 100kHz for OLED)

## Future Enhancements
- [ ] Full BLE GATT implementation
- [ ] USBX CDC ACM full integration
- [ ] TouchGFX screen animations
- [ ] Sensor data integration (accelerometer, temperature)
- [ ] Over-the-air firmware updates via BLE
- [ ] EEPROM configuration storage
- [ ] Power management modes
