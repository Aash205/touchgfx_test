/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ble_app.c
  * @brief   BLE application on the BlueNRG-2 (SPI1) network processor.
  *          Peripheral role: advertises a name and tracks connect / disconnect.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ble_app.h"
#include "usb_logging.h"
#include "app_state.h"
#include "hci.h"
#include "bluenrg1_hci_le.h"
#include "bluenrg1_hal_aci.h"
#include "bluenrg1_gap_aci.h"
#include "bluenrg1_gatt_aci.h"
#include "bluenrg1_gap.h"
#include "bluenrg1_hal.h"
#include "bluenrg1_events.h"
#include "bluenrg1_types.h"
#include "hci_const.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static BLE_AppHandleTypeDef ble_app_handle = {
  .status = BLE_APP_STATUS_IDLE,
  .device_name = "Nucleo-BLE-Demo",
  .connection_handle = 0xFFFF,
  .is_paired = 0,
  .last_update_time = 0
};

/* Demo GATT service (128-bit UUIDs, little-endian byte order as the BlueNRG expects):
 *   service 8a7c0001-4c3e-4e2b-9d4a-0b5f00c0ffee
 *   0002  LED control  : read/write, 1 byte bitmask (bit0 = LD1, bit1 = LD3)
 *   0003  Status       : read/notify, 6 bytes {ble_status, led_mask, heartbeat u32 LE}     */
#define UUID128(x) {0xee,0xff,0xc0,0x00,0x5f,0x0b,0x4a,0x9d,0x2b,0x4e,0x3e,0x4c,(x),0x00,0x7c,0x8a}
static const uint8_t uuid_service[16] = UUID128(0x01);
static const uint8_t uuid_led[16]     = UUID128(0x02);
static const uint8_t uuid_status[16]  = UUID128(0x03);

static uint16_t demo_service_handle;
static uint16_t led_char_handle;
static uint16_t status_char_handle;
static uint8_t  last_led_mask = 0xFF;      /* force first sync */
static uint32_t last_status_tick;

static uint16_t gap_service_handle;
static uint16_t gap_name_char_handle;
static uint16_t gap_appearance_char_handle;

/* Advertising interval in 0.625 ms units: 100 .. 200 ms */
#define BLE_ADV_INTERVAL_MIN  0x00A0U
#define BLE_ADV_INTERVAL_MAX  0x0140U

/* Private function prototypes -----------------------------------------------*/
static void ble_user_notify(void *pData);
static BLE_StatusTypeDef ble_set_discoverable(void);
static tBleStatus ble_add_demo_service(void);
static uint8_t led_mask_get(void);
static void ble_sync(void);

/**
 * @brief Initialize the BlueNRG-2: reset, GATT/GAP init, device name.
 */
BLE_StatusTypeDef BLE_App_Init(void)
{
  tBleStatus ret;
  const char *stage = "HCI reset";
  /* Static random-looking public address; change per board if several are used. */
  uint8_t bdaddr[6] = {0x01, 0x00, 0x00, 0xE1, 0x80, 0x02};

  ble_app_handle.status = BLE_APP_STATUS_INITIALIZING;

  hci_init(ble_user_notify, NULL);
  /* Allow the BlueNRG-M2SP DTM firmware to finish booting after hardware reset. */
  HAL_Delay(100);
  ret = hci_reset();
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  /* hci_reset() reboots the BlueNRG-2 controller. ST's X-CUBE-BLE2
   * reference applications require at least 2000 ms before the next ACI
   * command so both BlueNRG-2 and BlueNRG-2N firmware are ready. */
  HAL_Delay(2000);

  stage = "public address";
  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, sizeof(bdaddr), bdaddr);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  stage = "GATT init";
  ret = aci_gatt_init();
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  stage = "GAP init";
  ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0,
                     (uint8_t)strlen((char *)ble_app_handle.device_name),
                     &gap_service_handle, &gap_name_char_handle, &gap_appearance_char_handle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  stage = "device name";
  ret = aci_gatt_update_char_value(gap_service_handle, gap_name_char_handle, 0,
                                   (uint8_t)strlen((char *)ble_app_handle.device_name),
                                   ble_app_handle.device_name);
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  stage = "demo service";
  ret = ble_add_demo_service();
  if (ret != BLE_STATUS_SUCCESS) goto fail;

  ble_app_handle.status = BLE_APP_STATUS_INITIALIZED;
  return BLE_APP_STATUS_INITIALIZED;

fail:
  USB_Logging_Printf(LOG_LEVEL_ERROR, "BLE init failed at %s: 0x%02X", stage, ret);
  ble_app_handle.status = BLE_APP_STATUS_ERROR;
  return BLE_APP_STATUS_ERROR;
}

/**
 * @brief Start BLE advertising
 */
BLE_StatusTypeDef BLE_App_StartAdvertising(const char *device_name)
{
  if (!device_name || ble_app_handle.status == BLE_APP_STATUS_ERROR) return BLE_APP_STATUS_ERROR;

  strncpy((char *)ble_app_handle.device_name, device_name, sizeof(ble_app_handle.device_name) - 1);
  return ble_set_discoverable();
}

/**
 * @brief Stop BLE advertising
 */
BLE_StatusTypeDef BLE_App_StopAdvertising(void)
{
  if (aci_gap_set_non_discoverable() != BLE_STATUS_SUCCESS) return BLE_APP_STATUS_ERROR;

  ble_app_handle.status = BLE_APP_STATUS_INITIALIZED;
  return BLE_APP_STATUS_INITIALIZED;
}

/**
 * @brief Get BLE connection status
 */
BLE_StatusTypeDef BLE_App_GetStatus(void)
{
  return ble_app_handle.status;
}

/**
 * @brief Get BLE application handle
 */
BLE_AppHandleTypeDef *BLE_App_GetHandle(void)
{
  return &ble_app_handle;
}

/**
 * @brief Process pending HCI events (call periodically from the BLE thread).
 */
void BLE_App_Process(void)
{
  hci_user_evt_proc();
  ble_app_handle.last_update_time = HAL_GetTick();
  ble_sync();
}

/**
 * @brief Send data over BLE (no application GATT service yet: not connected -> -1)
 */
int BLE_App_SendData(uint8_t *data, uint16_t size)
{
  (void)data;
  (void)size;
  if (ble_app_handle.status != BLE_APP_STATUS_CONNECTED) {
    return -1;
  }
  return -1; /* TODO: add a GATT service/characteristic and aci_gatt_update_char_value() */
}

/* Private functions ---------------------------------------------------------*/
static uint8_t led_mask_get(void)
{
  AppState st;
  AppState_Get(&st);
  return (uint8_t)((st.led[0] ? 1U : 0U) | (st.led[1] ? 2U : 0U));
}

static tBleStatus ble_add_demo_service(void)
{
  Service_UUID_t svc;
  Char_UUID_t chr;
  tBleStatus ret;
  uint8_t mask = led_mask_get();
  uint8_t status[6] = {0};

  memcpy(svc.Service_UUID_128, uuid_service, 16);
  ret = aci_gatt_add_service(UUID_TYPE_128, &svc, PRIMARY_SERVICE, 8, &demo_service_handle);
  if (ret != BLE_STATUS_SUCCESS) return ret;

  memcpy(chr.Char_UUID_128, uuid_led, 16);
  ret = aci_gatt_add_char(demo_service_handle, UUID_TYPE_128, &chr, 1,
                          CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP,
                          ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE, 16, 0, &led_char_handle);
  if (ret != BLE_STATUS_SUCCESS) return ret;
  aci_gatt_update_char_value(demo_service_handle, led_char_handle, 0, 1, &mask);

  memcpy(chr.Char_UUID_128, uuid_status, 16);
  ret = aci_gatt_add_char(demo_service_handle, UUID_TYPE_128, &chr, sizeof(status),
                          CHAR_PROP_READ | CHAR_PROP_NOTIFY,
                          ATTR_PERMISSION_NONE, GATT_DONT_NOTIFY_EVENTS, 16, 0, &status_char_handle);
  return ret;
}

/* Push LED changes to the LED characteristic immediately, status (notify) once a second. */
static void ble_sync(void)
{
  if (demo_service_handle == 0 || ble_app_handle.status != BLE_APP_STATUS_CONNECTED) return;

  uint8_t mask = led_mask_get();
  if (mask != last_led_mask) {
    if (aci_gatt_update_char_value(demo_service_handle, led_char_handle, 0, 1, &mask) == BLE_STATUS_SUCCESS) {
      last_led_mask = mask;
    }
  }

  uint32_t now = HAL_GetTick();
  if ((now - last_status_tick) >= 1000U) {
    AppState st;
    uint8_t status[6];
    AppState_Get(&st);
    status[0] = st.ble_status;
    status[1] = mask;
    memcpy(&status[2], &st.heartbeat, 4);
    aci_gatt_update_char_value(demo_service_handle, status_char_handle, 0, sizeof(status), status);
    last_status_tick = now;
  }
}

static BLE_StatusTypeDef ble_set_discoverable(void)
{
  uint8_t name_len = (uint8_t)strlen((char *)ble_app_handle.device_name);
  uint8_t local_name[33];

  local_name[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
  memcpy(&local_name[1], ble_app_handle.device_name, name_len);

  aci_gap_set_non_discoverable();
  tBleStatus ret = aci_gap_set_discoverable(ADV_IND, BLE_ADV_INTERVAL_MIN, BLE_ADV_INTERVAL_MAX,
                                            PUBLIC_ADDR, NO_WHITE_LIST_USE,
                                            (uint8_t)(name_len + 1U), local_name,
                                            0, NULL, 0, 0);
  if (ret != BLE_STATUS_SUCCESS) {
    USB_Logging_Printf(LOG_LEVEL_ERROR, "BLE advertise failed: 0x%02X", ret);
    ble_app_handle.status = BLE_APP_STATUS_ERROR;
    return BLE_APP_STATUS_ERROR;
  }

  ble_app_handle.status = BLE_APP_STATUS_ADVERTISING;
  return BLE_APP_STATUS_ADVERTISING;
}

/* Called for each queued HCI packet: route events to the middleware's handler tables
 * (which invoke the hci_*_event() callbacks below). */
static void ble_user_notify(void *pData)
{
  hci_spi_pckt *hci_pckt = (hci_spi_pckt *)pData;
  hci_event_pckt *event_pckt;
  const hci_events_table_type *table = NULL;
  uint32_t count = 0;
  uint8_t code;

  if (hci_pckt->type != HCI_EVENT_PKT) return;

  event_pckt = (hci_event_pckt *)hci_pckt->data;

  if (event_pckt->evt == EVT_LE_META_EVENT) {
    evt_le_meta_event *meta = (evt_le_meta_event *)event_pckt->data;
    table = hci_le_meta_events_table;
    count = sizeof(hci_le_meta_events_table) / sizeof(hci_le_meta_events_table[0]);
    code = meta->subevent;
    for (uint32_t i = 0; i < count; i++) {
      if (table[i].evt_code == code) { table[i].process(meta->data); return; }
    }
  } else if (event_pckt->evt == EVT_VENDOR) {
    evt_blue_aci *blue = (evt_blue_aci *)event_pckt->data;
    for (uint32_t i = 0; i < sizeof(hci_vendor_specific_events_table) / sizeof(hci_vendor_specific_events_table[0]); i++) {
      if (hci_vendor_specific_events_table[i].evt_code == blue->ecode) {
        hci_vendor_specific_events_table[i].process(blue->data);
        return;
      }
    }
  } else {
    for (uint32_t i = 0; i < sizeof(hci_events_table) / sizeof(hci_events_table[0]); i++) {
      if (hci_events_table[i].evt_code == event_pckt->evt) {
        hci_events_table[i].process(event_pckt->data);
        return;
      }
    }
  }
}

/* HCI event callbacks (weak in the middleware) ------------------------------*/
void hci_le_connection_complete_event(uint8_t Status, uint16_t Connection_Handle, uint8_t Role,
                                      uint8_t Peer_Address_Type, uint8_t Peer_Address[6],
                                      uint16_t Conn_Interval, uint16_t Conn_Latency,
                                      uint16_t Supervision_Timeout, uint8_t Master_Clock_Accuracy)
{
  (void)Role; (void)Peer_Address_Type; (void)Peer_Address; (void)Conn_Interval;
  (void)Conn_Latency; (void)Supervision_Timeout; (void)Master_Clock_Accuracy;

  if (Status == BLE_STATUS_SUCCESS) {
    ble_app_handle.connection_handle = Connection_Handle;
    ble_app_handle.status = BLE_APP_STATUS_CONNECTED;
    last_led_mask = 0xFF;
    USB_Logging_Printf(LOG_LEVEL_INFO, "BLE connected (handle 0x%04X)", Connection_Handle);
  }
}

void hci_disconnection_complete_event(uint8_t Status, uint16_t Connection_Handle, uint8_t Reason)
{
  (void)Connection_Handle;
  if (Status == BLE_STATUS_SUCCESS) {
    ble_app_handle.connection_handle = 0xFFFF;
    ble_app_handle.is_paired = 0;
    USB_Logging_Printf(LOG_LEVEL_INFO, "BLE disconnected (reason 0x%02X), re-advertising", Reason);
    ble_set_discoverable();
  }
}

/* A client wrote a characteristic. Attr_Handle is the value handle (= char handle + 1). */
void aci_gatt_attribute_modified_event(uint16_t Connection_Handle, uint16_t Attr_Handle,
                                       uint16_t Offset, uint16_t Attr_Data_Length, uint8_t Attr_Data[])
{
  (void)Connection_Handle; (void)Offset;

  if (Attr_Handle == (uint16_t)(led_char_handle + 1U) && Attr_Data_Length >= 1U) {
    for (uint8_t i = 0; i < APP_LED_COUNT; i++) {
      AppState_SetLed(i, (Attr_Data[0] >> i) & 1U);
    }
    USB_Logging_Printf(LOG_LEVEL_INFO, "BLE LED write: 0x%02X", Attr_Data[0]);
    last_led_mask = 0xFF;   /* re-read state on next sync */
  }
}
