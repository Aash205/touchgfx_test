# Integration Guide: TouchGFX + ThreadX + BLE + USBX Demo

## System Architecture

This demo implements a multi-threaded embedded system with four key components working together:

```
Hardware (STM32L496)
├── I2C1 → OLED Display (SSD1306 128x64)
├── LPUART1 → USB Serial (UART Commands)
├── SPI → BlueNRG-2 BLE Module
├── USB → USBX CDC Device (Logging)
└── GPIO → LEDs (PA5 - User LED)

Software (Middleware & Apps)
├── ThreadX RTOS (4 Tasks)
│   ├── BLE Task (10Hz) - BLE connectivity
│   ├── UART CMD Task (20Hz) - Command processing
│   ├── Monitor Task (1Hz) - System monitoring
│   └── TouchGFX Task (100Hz) - GUI rendering
├── OLED Driver (I2C) - Display output
├── USB Logging - Diagnostics
└── BLE Stack (BlueNRG-2) - Wireless comms
```

## Component Integration Map

### 1. OLED Display ↔ ThreadX Threads
**File**: `Core/Src/oled_driver.c`, `Core/Src/app_threadx.c`

Each thread updates the OLED with its status:
- BLE Thread: Shows connection status ("Advertising", "Connected", "Paired")
- UART Thread: Shows last command executed ("LED0 ON", etc.)
- Monitor Thread: Shows system status ("Running...")

**Integration Points**:
```c
// In each thread:
OLED_Clear(&oled_handle);
OLED_PrintStr(&oled_handle, 0, 0, "BLE Status");
OLED_PrintStr(&oled_handle, 0, 1, status_str);
OLED_UpdateDisplay(&oled_handle);
```

### 2. UART Commands ↔ LED Control ↔ OLED Feedback
**File**: `Core/Src/uart_commands.c`, `Core/Src/app_threadx.c`

Command flow:
```
User sends UART command
    ↓
UART_CMD_Process() parses command
    ↓
UART_CMD_SetLEDState() toggles GPIO
    ↓
OLED displays command feedback
    ↓
USB logs the action
```

**Example - LED0 ON command**:
1. Receives: "LED0 ON\r\n"
2. Parses: LED index = 0, Action = ON
3. Executes: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET)
4. Displays: "LED0: ON" on OLED line 2
5. Logs: "[INFO] LED0 turned ON"

### 3. BLE Status ↔ OLED Display ↔ USB Logging
**File**: `Core/Src/ble_app.c`, `Core/Src/app_threadx.c`, `Core/Src/usb_logging.c`

BLE state transitions:
```
IDLE → INITIALIZING → INITIALIZED → ADVERTISING → CONNECTED → PAIRED
```

Each state change:
- Updates OLED display in real-time
- Logs event with timestamp via USB
- May trigger UART notifications

**Example - BLE Connection**:
```
1. Device starts advertising (OLED: "Advertising")
2. Remote device connects (OLED: "Connected", USB: "[INFO] BLE Connected")
3. Pairing request (OLED: "Pairing...", USB: "[INFO] Pairing requested")
4. Pairing complete (OLED: "Paired", USB: "[INFO] Pairing successful")
```

### 4. System Monitor ↔ USB Logging
**File**: `Core/Src/usb_logging.c`, `Core/Src/app_threadx.c`

Monitor thread periodically logs:
- System uptime
- Thread states
- BLE connection status
- LED states
- Memory usage

**Log Example**:
```
[INFO] [00:00:05.000] System Uptime: 5 seconds
[INFO] [00:00:05.100] BLE Status: 1, LED Count: 1
[DEBUG] [00:00:10.000] Heap Free: 8192 bytes
```

## Data Flow Diagrams

### User Interaction Flow
```
┌──────────────┐
│ User connects│
│ to UART port │
└──────┬───────┘
       │
       ▼
┌──────────────────────────────┐
│ Send: "LED0 ON\r\n"          │
└──────┬───────────────────────┘
       │
       ▼
┌──────────────────────────────┐    ┌──────────────────┐
│ UART RX Interrupt            │───▶│ uart_cmd_handler │
└──────┬───────────────────────┘    └──────────────────┘
       │
       ▼
┌──────────────────────────────┐    ┌──────────────────┐
│ Command Ready (rx_buffer)    │───▶│ UART CMD Thread  │
└──────┬───────────────────────┘    └────────┬─────────┘
       │                                     │
       ├─────────────────────────────────────┤
       │                                     ▼
       │                        ┌──────────────────┐
       │                        │ Parse command    │
       │                        │ LED0 ON          │
       │                        └────────┬─────────┘
       │                                 │
       │                    ┌────────────┴────────────┐
       │                    │                         ▼
       │                    ▼                ┌──────────────────┐
       │         ┌─────────────────┐        │ Set LED GPIO     │
       │         │ Update OLED     │        │ PA5 = SET        │
       │         │ "LED0: ON"      │        └────────┬─────────┘
       │         └────────┬────────┘                 │
       │                  │                         │
       │                  ▼                         ▼
       │         ┌─────────────────────────────────────┐
       │         │ USB_Logging_Printf                  │
       │         │ "LED0 turned ON"                    │
       │         └────────┬────────────────────────────┘
       │                  │
       ▼                  ▼
    ┌────────────────────────────────┐
    │ User sees feedback on all       │
    │ - OLED: LED0 ON                │
    │ - USB Log: [INFO] LED0 ON      │
    └────────────────────────────────┘
```

## Configuration Requirements

### 1. HAL Initialization (main.c)
```c
// Must be called BEFORE MX_ThreadX_Init():
MX_GPIO_Init();          // GPIO for LEDs
MX_LPUART1_UART_Init();  // UART for commands
// I2C1 must be initialized for OLED
// SPI for BLE (if using SPI interface)
```

### 2. Interrupt Handlers
```c
// UART RX Callback (in stm32l4xx_it.c)
void LPUART1_IRQHandler(void) {
  HAL_UART_IRQHandler(&hlpuart1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == LPUART1) {
    uint8_t data;
    HAL_UART_Receive(&hlpuart1, &data, 1, 0);
    UART_CMD_ReceiveCallback(&uart_cmd_handler, data);
  }
}
```

### 3. I2C Configuration
```c
// I2C1 for OLED (100 kHz standard mode)
// Typical STM32CubeMX settings:
// - Fast mode: Disabled
// - Filtering: Enabled
// - Timing: STM32L496 at 4MHz MSI
```

### 4. USART/LPUART
```c
// LPUART1 for UART commands
// Typical settings:
// - Baud rate: 115200
// - Data bits: 8
// - Stop bits: 1
// - Parity: None
// - Flow control: Disabled
```

## Thread Priority Design

```
Priority    Thread              Usage
───────────────────────────────────────
+2          BLE Thread          High-priority event handling
+1          UART CMD Thread     User command processing  
 0          Monitor Thread      System monitoring (base priority)
-1          TouchGFX Thread     GUI rendering (lowest priority)
```

**Rationale**:
- BLE events are time-critical (wireless communication)
- User commands need responsive feedback
- Monitoring is periodic, less urgent
- GUI can run at lower priority but still responsive

## State Machine Flows

### BLE State Machine
```
┌─────────────────────────────────────────┐
│ IDLE                                    │
│ (Not initialized)                       │
└────────────┬────────────────────────────┘
             │ BLE_App_Init()
             ▼
┌─────────────────────────────────────────┐
│ INITIALIZED                             │
│ (Stack ready, not advertising)          │
└────────────┬────────────────────────────┘
             │ BLE_App_StartAdvertising()
             ▼
┌─────────────────────────────────────────┐
│ ADVERTISING                             │
│ (Scanning for connections)              │
└────┬──────────────────────────┬──────────┘
     │ Remote connects          │ BLE_App_StopAdvertising()
     ▼                          ▼
┌─────────────────────┐    ┌──────────────┐
│ CONNECTED           │    │ INITIALIZED  │
│ (Link established)  │    │ (Idle)       │
└────────┬────────────┘    └──────────────┘
         │ Pairing request
         ▼
┌─────────────────────┐
│ PAIRED              │
│ (Bonded link)       │
└─────────────────────┘
```

## Critical Sections & Synchronization

### Shared Resources
1. **OLED Display** - Accessed by multiple threads
   - Protected by: Sequential access in app_threadx.c
   - Solution: Each thread gets its own time slice

2. **LED States** - Modified by UART CMD thread
   - Protected by: Direct GPIO write
   - No synchronization needed (atomic HAL calls)

3. **USB Logging Buffer** - Accessed by multiple threads
   - Protected by: usb_logging.c internal buffer
   - Solution: Atomic buffer swap on flush

## Timing Considerations

### Thread Execution Times
```
BLE Thread:        ~50-100ms per cycle (10Hz update)
UART Thread:       ~50ms per cycle (20Hz processing)
Monitor Thread:    ~1000ms per cycle (1Hz)
TouchGFX Thread:   ~10ms per cycle (100Hz)
```

### Critical Timings
- OLED Update: 50-100ms (I2C communication)
- LED Toggle: <1ms (GPIO write)
- USB Log: 5-10ms (USB transfer)
- BLE Event: 1-10ms (HCI command)

## Debugging & Monitoring

### Via USB Logging
```c
// Enable debug logging
usb_logging.log_level = LOG_LEVEL_DEBUG;

// Log custom messages
USB_Logging_Printf(LOG_LEVEL_INFO, "System status: %d", status);

// Get log count
printf("Total logs: %d\n", usb_logging.log_count);
```

### Via OLED
- Each thread updates OLED with current status
- Rotate display every 1-5 seconds
- Shows: Mode, Status, LED states, BLE connection

### Via UART
- Send STATUS command to get real-time state
- Send HELP to see available commands
- Real-time feedback for each command

## Known Limitations & TODOs

1. **BLE Stack Integration** (TODO in ble_app.c)
   - HCI commands not fully implemented
   - Need to call: hci_reset(), hci_le_set_advertising_parameters()
   - GATT services not configured

2. **USBX Integration** (TODO in usb_logging.c)
   - CDC ACM transport layer not connected
   - Need to implement: ux_device_class_cdc_acm_write()
   - Buffer not actually sent to USB

3. **TouchGFX Integration** (TODO in app_threadx.c)
   - GUI rendering not connected to main loop
   - Need to implement: MX_TouchGFX_Process() or equivalent
   - Screen animations not added

4. **Memory Management**
   - OLED print uses malloc() for temporary buffer (potential issue)
   - Should use stack-allocated buffer instead

## Next Steps for Full Implementation

1. **Complete BLE Integration**
   - Implement BlueNRG-2 HCI command sequences
   - Add GATT service discovery
   - Implement pairing/bonding

2. **Complete USB Integration**
   - Implement USBX CDC ACM write function
   - Set up USB device enumeration
   - Test USB logging output

3. **Optimize Performance**
   - Profile thread CPU usage
   - Adjust stack sizes if needed
   - Optimize I2C communication

4. **Add Features**
   - Sensor integration (accelerometer, temp)
   - Real-time data streaming via BLE
   - On-device configuration menu
   - Power management modes

## Testing Checklist

- [ ] OLED displays correctly initialized
- [ ] UART commands parse correctly
- [ ] LED0 toggles with commands
- [ ] BLE stack initializes without errors
- [ ] BLE device is discoverable
- [ ] USB logging shows system events
- [ ] Threads run without deadlock
- [ ] All four threads operational
- [ ] OLED updates in real-time
- [ ] System stable for >1 hour runtime
