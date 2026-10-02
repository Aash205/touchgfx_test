#include "ble_sync.h"

#include "timeouts.h"

bool BleSync_Active(BleFsm_State_t state, bool have_service)
{
    return have_service && (state == BLE_FSM_CONNECTED);
}

bool BleSync_LedDue(uint8_t led_mask, uint8_t last_led_mask)
{
    return led_mask != last_led_mask;
}

bool BleSync_StatusDue(uint32_t now, uint32_t last_status_tick)
{
    return Timeout_Elapsed(last_status_tick, now, BLE_SYNC_STATUS_PERIOD_MS);
}
