#include "ble_fsm.h"

#include "timeouts.h"

BleFsm_State_t BleFsm_Next(BleFsm_State_t state, BleFsm_Event_t event)
{
    BleFsm_State_t next = state;

    switch (event)
    {
    case BLE_FSM_EVENT_INIT_STARTED:
        next = BLE_FSM_INITIALIZING;
        break;
    case BLE_FSM_EVENT_INIT_SUCCEEDED:
    case BLE_FSM_EVENT_STOP_SUCCEEDED:
        next = BLE_FSM_INITIALIZED;
        break;
    case BLE_FSM_EVENT_INIT_FAILED:
    case BLE_FSM_EVENT_ADVERTISE_FAILED:
    case BLE_FSM_EVENT_STOP_FAILED:
        next = BLE_FSM_ERROR;
        break;
    case BLE_FSM_EVENT_ADVERTISE_SUCCEEDED:
        next = BLE_FSM_ADVERTISING;
        break;
    case BLE_FSM_EVENT_CONNECT_SUCCEEDED:
        next = BLE_FSM_CONNECTED;
        break;
    case BLE_FSM_EVENT_CONNECT_FAILED:
    default:
        break;
    }

    return next;
}

bool BleFsm_MayAdvertise(BleFsm_State_t state)
{
    return state != BLE_FSM_ERROR;
}

bool BleFsm_AdvertisingExpired(BleFsm_State_t state, uint32_t started, uint32_t now)
{
    return (state == BLE_FSM_ADVERTISING) &&
           Timeout_Elapsed(started, now, BLE_FSM_ADVERTISING_LIMIT_MS);
}
