#include "ws169_init.h"

#define INIT_COMMAND_COUNT 14U

size_t WS169_InitCommandCount(void)
{
    return INIT_COMMAND_COUNT;
}

bool WS169_InitCommandAt(size_t index, WS169_InitCommand_t* command)
{
    static const uint8_t pixel_format[] = {0x55U};
    static const uint8_t porch[] = {0x0BU, 0x0BU, 0x00U, 0x33U, 0x35U};
    static const uint8_t gate_control[] = {0x11U};
    static const uint8_t vcom[] = {0x35U};
    static const uint8_t lcm_control[] = {0x2CU};
    static const uint8_t vdv_vrh_enable[] = {0x01U};
    static const uint8_t vrh_set[] = {0x0DU};
    static const uint8_t vdv_set[] = {0x20U};
    static const uint8_t frame_rate[] = {0x13U};
    static const uint8_t power[] = {0xA4U, 0xA1U};
    static const uint8_t power_control[] = {0xA1U};
    static const uint8_t gamma_positive[] = {0xF0U, 0x06U, 0x0BU, 0x0AU, 0x09U, 0x26U, 0x29U,
                                             0x33U, 0x41U, 0x18U, 0x16U, 0x15U, 0x29U, 0x2DU};
    static const uint8_t gamma_negative[] = {0xF0U, 0x04U, 0x08U, 0x08U, 0x07U, 0x03U, 0x28U,
                                             0x32U, 0x40U, 0x3BU, 0x19U, 0x18U, 0x2AU, 0x2EU};
    static const uint8_t vendor_e4[] = {0x25U, 0x00U, 0x00U};

    static const WS169_InitCommand_t sequence[INIT_COMMAND_COUNT] = {
        {0x3AU, pixel_format, (uint8_t)sizeof(pixel_format)},
        {0xB2U, porch, (uint8_t)sizeof(porch)},
        {0xB7U, gate_control, (uint8_t)sizeof(gate_control)},
        {0xBBU, vcom, (uint8_t)sizeof(vcom)},
        {0xC0U, lcm_control, (uint8_t)sizeof(lcm_control)},
        {0xC2U, vdv_vrh_enable, (uint8_t)sizeof(vdv_vrh_enable)},
        {0xC3U, vrh_set, (uint8_t)sizeof(vrh_set)},
        {0xC4U, vdv_set, (uint8_t)sizeof(vdv_set)},
        {0xC6U, frame_rate, (uint8_t)sizeof(frame_rate)},
        {0xD0U, power, (uint8_t)sizeof(power)},
        {0xD6U, power_control, (uint8_t)sizeof(power_control)},
        {0xE0U, gamma_positive, (uint8_t)sizeof(gamma_positive)},
        {0xE1U, gamma_negative, (uint8_t)sizeof(gamma_negative)},
        {0xE4U, vendor_e4, (uint8_t)sizeof(vendor_e4)}};

    bool found = false;

    if ((command != NULL) && (index < INIT_COMMAND_COUNT))
    {
        *command = sequence[index];
        found = true;
    }

    return found;
}
