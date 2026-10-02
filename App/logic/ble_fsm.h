#ifndef BLE_FSM_H
#define BLE_FSM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Same values and order as BLE_StatusTypeDef in ble_app.h. */
typedef enum
{
    BLE_FSM_IDLE = 0,
    BLE_FSM_INITIALIZING = 1,
    BLE_FSM_INITIALIZED = 2,
    BLE_FSM_ADVERTISING = 3,
    BLE_FSM_CONNECTED = 4,
    BLE_FSM_PAIRED = 5,
    BLE_FSM_ERROR = 6
} BleFsm_State_t;

typedef enum
{
    BLE_FSM_EVENT_INIT_STARTED = 0,
    BLE_FSM_EVENT_INIT_SUCCEEDED = 1,
    BLE_FSM_EVENT_INIT_FAILED = 2,
    BLE_FSM_EVENT_ADVERTISE_SUCCEEDED = 3,
    BLE_FSM_EVENT_ADVERTISE_FAILED = 4,
    BLE_FSM_EVENT_STOP_SUCCEEDED = 5,
    BLE_FSM_EVENT_STOP_FAILED = 6,
    BLE_FSM_EVENT_CONNECT_SUCCEEDED = 7,
    BLE_FSM_EVENT_CONNECT_FAILED = 8
} BleFsm_Event_t;

/* Advertising stops by itself after this long (milliseconds). */
#define BLE_FSM_ADVERTISING_LIMIT_MS 120000U

/* clang-format off */
/*
 * The connection and advertising state of the BLE application.
 *
 * BleFsm_Next gives the state after an event. The result of the hardware call that was made is
 * the event, so the state always follows what the BlueNRG reported:
 *   INIT_STARTED -> INITIALIZING; INIT_SUCCEEDED -> INITIALIZED; INIT_FAILED -> ERROR;
 *   ADVERTISE_SUCCEEDED -> ADVERTISING; ADVERTISE_FAILED -> ERROR;
 *   STOP_SUCCEEDED -> INITIALIZED; STOP_FAILED -> ERROR;
 *   CONNECT_SUCCEEDED -> CONNECTED; CONNECT_FAILED -> the state does not change.
 * These do not depend on the current state (the firmware never guarded them), except that
 * CONNECT_FAILED keeps it. A state or event outside the enums leaves the state unchanged.
 * The PAIRED state is never reached: the firmware does not track pairing.
 *
 * BleFsm_MayAdvertise: advertising can be started from every state except ERROR.
 *
 * BleFsm_AdvertisingExpired: true when the state is ADVERTISING and at least
 * BLE_FSM_ADVERTISING_LIMIT_MS have passed since `started`; wrap-safe for free-running
 * millisecond counters.
 */
BleFsm_State_t BleFsm_Next(BleFsm_State_t state, BleFsm_Event_t event);
bool BleFsm_MayAdvertise(BleFsm_State_t state);
bool BleFsm_AdvertisingExpired(BleFsm_State_t state, uint32_t started, uint32_t now);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* BLE_FSM_H */
