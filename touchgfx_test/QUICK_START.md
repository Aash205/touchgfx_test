# Quick Start Guide: Multi-Feature Demo on STM32L496 Nucleo

## What You're Getting

This demo implements a complete embedded system demonstrating:
- **Real-time multithreading** (ThreadX RTOS)
- **BLE wireless connectivity** (BlueNRG-2)
- **I2C OLED display** (Waveshare SSD1306)
- **Serial commands** (UART LED control)
- **Diagnostic logging** (USB CDC ACM)
- **GUI framework** (TouchGFX integration)

All components work together in a real-time system with 4 concurrent threads.

## Prerequisites

### Hardware
- STM32L496 Nucleo-L496ZQ board
- Waveshare OLED 128x64 I2C display (or compatible SSD1306)
- BlueNRG-2 BLE module (optional, for full BLE)
- USB cable for debugging/logging
- Serial terminal software (PuTTY, Tera Term, CoolTerm, etc.)

### Software
- STM32CubeIDE or STM32CubeMX
- ARM GCC compiler (arm-none-eabi)
- CMake 3.15+
- Git (for version control)

## File Structure

```
touchgfx_test/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── app_threadx.h
│   │   ├── oled_driver.h          ← NEW: OLED display driver
│   │   ├── ble_app.h              ← NEW: BLE application
│   │   ├── uart_commands.h        ← NEW: UART command handler
│   │   ├── usb_logging.h          ← NEW: USB diagnostic logging
│   │   └── app_tests.h            ← NEW: Test suite
│   └── Src/
│       ├── main.c
│       ├── app_threadx.c          ← MODIFIED: Added 4 ThreadX tasks
│       ├── oled_driver.c          ← NEW: OLED driver implementation
│       ├── ble_app.c              ← NEW: BLE app implementation
│       ├── uart_commands.c        ← NEW: UART handler implementation
│       ├── usb_logging.c          ← NEW: USB logging implementation
│       └── app_tests.c            ← NEW: Test implementations
├── Drivers/
├── Middlewares/
├── TouchGFX/
├── AZURE_RTOS/
├── USBX/
├── BlueNRG-2/
├── CMakeLists.txt
├── DEMO_README.md                 ← Complete demo documentation
├── INTEGRATION_GUIDE.md           ← Detailed integration guide
└── QUICK_START.md                 ← This file
```

## Configuration Checklist

### Step 1: Hardware Setup

1. **Connect OLED Display** (I2C1)
   - VCC → 3.3V
   - GND → GND
   - SCL → PB6 (I2C1_SCL)
   - SDA → PB7 (I2C1_SDA)
   - Connect 100nF capacitor between VCC and GND

2. **Connect USB Serial** (for UART commands)
   - TX → PA3 (LPUART1_TX) via USB-UART module
   - RX → PA2 (LPUART1_RX) via USB-UART module
   - GND → GND

3. **Optional: BLE Module** (BlueNRG-2)
   - Follow BlueNRG-2 expansion board documentation
   - CS → PC2
   - RST → PF13
   - SPI/I2C interface

### Step 2: STM32CubeMX Configuration

**Important**: If using STM32CubeIDE with code generation, only regenerate at specific checkpoints. Many custom files should NOT be regenerated.

1. **Open touchgfx_test.ioc** in STM32CubeMX

2. **Verify Peripherals**:
   - ✓ I2C1: Enabled (100 kHz, no interrupts)
   - ✓ LPUART1: Enabled (115200 baud, RX interrupt)
   - ✓ GPIO: PA5 configured as output (LED)
   - ✓ SPI (if using BLE): Configured per BlueNRG-2 requirements

3. **Verify Middleware**:
   - ✓ ThreadX: Enabled with memory allocation
   - ✓ TouchGFX: Configured (simulator or display)
   - ✓ USBX: Device mode with CDC ACM (optional)

4. **Clock Configuration**:
   - MSI: 4 MHz (default, can be increased to 80 MHz if needed)
   - PLL: Optional (for better performance)

5. **Generate Code** (Code Generator → Generate)
   - **Important**: Only regenerate Core/Src/main.c if absolutely necessary
   - **Never overwrite**: app_threadx.c, and all the new component files
   - If regenerated, manually reapply changes

### Step 3: Build & Flash

```bash
# Navigate to project
cd touchgfx_test

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build project
cmake --build . -j4

# Flash to board (via STM32CubeIDE or command line)
cmake --build . --target flash
```

### Step 4: Connect & Test

1. **Open Serial Terminal**
   - Port: COM3 (or your USB serial port)
   - Baud: 115200
   - Data: 8 bits
   - Stop: 1
   - Parity: None
   - Flow: None

2. **Send First Command**
   ```
   LED0 ON
   ```
   Expected response:
   ```
   LED0: ON
   ```

3. **Check OLED Display**
   - Should show "LED0: ON" or similar status

4. **Monitor USB Logs**
   - If USB logging is connected, should see system startup messages

## Basic Operation

### UART Command Examples

```
# LED Control
LED0 ON          → Turn on LED
LED0 OFF         → Turn off LED
LED0 TOGGLE      → Toggle LED state
LED0 BLINK       → Blink LED slowly

# System Status
STATUS           → Show current status
HELP             → Show available commands

# USB Logging (automatic)
[INFO] System startup
[DEBUG] UART Command processed: LED0 ON
[INFO] BLE Status: Advertising
```

### OLED Display Updates

The display shows different information as threads run:

```
┌─────────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│ Initialization      │ →  │ BLE Active       │ →  │ UART Ready      │
│                     │     │ Scanning...      │     │ Send commands   │
└─────────────────────┘     └──────────────────┘     └─────────────────┘
                                                              ↓
                                                     ┌─────────────────┐
                                                     │ UART Status     │
                                                     │ LED0: ON        │
                                                     └─────────────────┘
```

### BLE Discovery

1. Enable Bluetooth on your phone
2. Open BLE scanner app
3. Look for device named "Nucleo-BLE-Demo"
4. Tap to connect
5. Connection status updates on OLED and USB logs

## Running Tests

To verify all components work correctly:

```c
// In main.c, add before MX_ThreadX_Init():
Test_RunAll(&hi2c1, &hlpuart1);
```

Expected output:
```
[INFO] === Starting Test Suite ===
[INFO] Test PASSED: USB Logging
[INFO] Test PASSED: OLED Initialization
[INFO] Test PASSED: OLED Drawing Operations
[INFO] Test PASSED: UART Command Parsing
[INFO] Test PASSED: LED Control
[INFO] Test PASSED: BLE Initialization
[INFO] === Test Suite Complete ===
[INFO] Failed Tests: 0/6
```

## Troubleshooting

### OLED Not Displaying
- **Check I2C Connection**: Verify SCL (PB6) and SDA (PB7) are connected
- **Check Address**: OLED address should be 0x78 (or 0x3C << 1)
- **Check I2C Frequency**: Should be 100 kHz (standard mode)
- **Add Pull-ups**: 10kΩ pull-up resistors on SCL/SDA if not built-in

### UART Commands Not Working
- **Check Baud Rate**: Must be 115200
- **Check RX Interrupt**: LPUART1 interrupt must be enabled
- **Check Pin Connections**: PA2=RX, PA3=TX
- **Check Terminal**: Use proper line ending (CR or CRLF)

### LED Not Toggling
- **Check GPIO**: PA5 should be configured as output
- **Check LED Polarity**: Should light up when PA5 is HIGH
- **Check Power**: Ensure 3.3V is supplied to LED through resistor

### BLE Not Advertizing
- **Check BlueNRG-2**: Verify module is connected and powered
- **Check HCI**: May need to implement full HCI command sequence
- **Check SPI/I2C**: Verify communication interface is working

### Build Errors
- **CMake Error**: Run `cmake --version` (need 3.15+)
- **Compiler Error**: Run `arm-none-eabi-gcc --version`
- **Missing Files**: Verify all source files exist in Core/Src and Core/Inc

## Performance Expectations

- **Startup Time**: ~500ms until threads start running
- **OLED Update**: ~100ms per display update
- **UART Response**: <50ms to process commands
- **BLE Scanning**: Continuous (10Hz update rate)
- **System Uptime**: Should run indefinitely without issues

## Next Steps

1. **Understand the Demo**
   - Read [DEMO_README.md](DEMO_README.md) for detailed component info
   - Read [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) for architecture details

2. **Extend Functionality**
   - Add more BLE characteristics for data streaming
   - Implement USBX CDC ACM for USB logging
   - Add sensor interfaces (temperature, accelerometer)
   - Create custom GUI screens in TouchGFX

3. **Optimize Performance**
   - Tune thread priorities for your use case
   - Increase system clock if needed (up to 80 MHz)
   - Implement power management modes

4. **Deploy to Production**
   - Add secure BLE pairing
   - Implement firmware update mechanism
   - Add error handling and watchdog
   - Test extended runtime (24+ hours)

## Support & Documentation

- **Main README**: [DEMO_README.md](DEMO_README.md)
- **Integration Details**: [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)
- **This Guide**: [QUICK_START.md](QUICK_START.md)

Each file is self-contained and can be read independently for specific information.

## Safety & Best Practices

- Always use current-limiting resistors with LEDs
- Verify power supply polarity before connecting
- Use decoupling capacitors (100nF) on all VCC pins
- Avoid pressing reset while system is writing to flash
- Implement watchdog timer for production systems

---

**Happy Hacking!** 🚀

For questions or issues, check the integration guide or refer to component documentation.
