#include "ble_codec.h"

#define LED_MASK_BITS 8U

uint8_t BleCodec_LedMask(const uint8_t* led, unsigned count)
{
    uint8_t mask = 0U;

    if (led != NULL)
    {
        for (unsigned i = 0U; (i < count) && (i < LED_MASK_BITS); i++)
        {
            if (led[i] != 0U)
            {
                mask = (uint8_t)(mask | (uint8_t)(1U << i));
            }
        }
    }

    return mask;
}

uint8_t BleCodec_LedBit(uint8_t mask, unsigned index)
{
    uint8_t bit = 0U;

    if (index < LED_MASK_BITS)
    {
        bit = (uint8_t)((mask >> index) & 1U);
    }

    return bit;
}

void BleCodec_StatusPacket(uint8_t* out, uint8_t ble_status, uint8_t led_mask, uint32_t heartbeat)
{
    if (out != NULL)
    {
        out[0] = ble_status;
        out[1] = led_mask;
        out[2] = (uint8_t)(heartbeat & 0xFFU);
        out[3] = (uint8_t)((heartbeat >> 8U) & 0xFFU);
        out[4] = (uint8_t)((heartbeat >> 16U) & 0xFFU);
        out[5] = (uint8_t)((heartbeat >> 24U) & 0xFFU);
    }
}

void BleCodec_Uuid128(uint8_t* out, uint8_t id)
{
    static const uint8_t base[BLE_CODEC_UUID_LENGTH] = {0xEEU, 0xFFU, 0xC0U, 0x00U, 0x5FU, 0x0BU,
                                                        0x4AU, 0x9DU, 0x2BU, 0x4EU, 0x3EU, 0x4CU,
                                                        0x00U, 0x00U, 0x7CU, 0x8AU};

    if (out != NULL)
    {
        for (size_t i = 0U; i < BLE_CODEC_UUID_LENGTH; i++)
        {
            out[i] = base[i];
        }
        out[12] = id;
    }
}

size_t BleCodec_CopyName(uint8_t* dst, size_t dst_capacity, const char* src)
{
    size_t length = 0U;

    if ((dst != NULL) && (dst_capacity != 0U))
    {
        if (src != NULL)
        {
            while ((length < (dst_capacity - 1U)) && (src[length] != '\0'))
            {
                dst[length] = (uint8_t)src[length];
                length++;
            }
        }
        dst[length] = 0U;
    }

    return length;
}

size_t BleCodec_LocalNameAd(uint8_t* out, size_t capacity, const uint8_t* name)
{
    size_t length = 0U;

    if ((out != NULL) && (capacity != 0U))
    {
        out[0] = (uint8_t)BLE_AD_TYPE_COMPLETE_LOCAL_NAME;
        length = 1U;
        if (name != NULL)
        {
            while ((length < capacity) && (name[length - 1U] != 0U))
            {
                out[length] = name[length - 1U];
                length++;
            }
        }
    }

    return length;
}
