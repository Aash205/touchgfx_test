#ifndef BLE_SYNC_H
#define BLE_SYNC_H

#include "ble_fsm.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* The status characteristic is notified at most this often (milliseconds). */
#define BLE_SYNC_STATUS_PERIOD_MS 1000U

/* clang-format off */
/*
 * When the firmware pushes its state to a connected BLE client. Pure predicates: the GATT calls
 * and the "last sent" bookkeeping stay in the firmware.
 *
 * BleSync_Active: syncing happens only while the state is CONNECTED and the GATT service exists
 * (have_service).
 * BleSync_LedDue: the LED characteristic is written when the LED mask differs from the one
 * written last.
 * BleSync_StatusDue: the status characteristic is notified when at least
 * BLE_SYNC_STATUS_PERIOD_MS have passed since the last one; wrap-safe for free-running
 * millisecond counters.
 */
bool BleSync_Active(BleFsm_State_t state, bool have_service);
bool BleSync_LedDue(uint8_t led_mask, uint8_t last_led_mask);
bool BleSync_StatusDue(uint32_t now, uint32_t last_status_tick);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* BLE_SYNC_H */
