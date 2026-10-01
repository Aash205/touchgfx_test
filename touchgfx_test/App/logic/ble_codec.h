#ifndef BLE_CODEC_H
#define BLE_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define BLE_CODEC_STATUS_LENGTH 6U
#define BLE_CODEC_UUID_LENGTH 16U
#define BLE_AD_TYPE_COMPLETE_LOCAL_NAME 0x09U

/* clang-format off */
/*
 * Byte-level encoding for the BLE demo GATT service. Pure functions: the BlueNRG calls that send
 * these bytes stay in the firmware. Every function accepts a NULL output (or a zero capacity)
 * and then writes nothing.
 */

/*
 * Bitmask of the LEDs that are on: bit i is set when led[i] is non-zero, for i below count
 * and below 8. A NULL array gives 0.
 */
uint8_t BleCodec_LedMask(const uint8_t* led, unsigned count);

/* Bit `index` of a received LED mask as 0 or 1; 0 when index is 8 or more. */
uint8_t BleCodec_LedBit(uint8_t mask, unsigned index);

/*
 * The status characteristic value: {ble_status, led_mask, heartbeat as u32 little-endian}.
 * out must hold BLE_CODEC_STATUS_LENGTH bytes.
 */
void BleCodec_StatusPacket(uint8_t* out, uint8_t ble_status, uint8_t led_mask, uint32_t heartbeat);

/*
 * The 128-bit UUID 8a7c00<id>-4c3e-4e2b-9d4a-0b5f00c0ffee in the little-endian byte order the
 * BlueNRG expects. out must hold BLE_CODEC_UUID_LENGTH bytes.
 */
void BleCodec_Uuid128(uint8_t* out, uint8_t id);

/*
 * Copy a NUL-terminated name into dst, truncated to dst_capacity - 1 characters, and always
 * NUL-terminate. Returns the number of characters copied. A NULL src gives an empty name.
 */
size_t BleCodec_CopyName(uint8_t* dst, size_t dst_capacity, const char* src);

/*
 * The advertising data element for the complete local name: {0x09, name bytes}, without a NUL,
 * truncated so the whole element fits in capacity. Returns the element length (1 plus the
 * name characters used), or 0 when out is NULL or capacity is 0. A NULL name gives just the
 * type byte.
 */
size_t BleCodec_LocalNameAd(uint8_t* out, size_t capacity, const uint8_t* name);
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* BLE_CODEC_H */
