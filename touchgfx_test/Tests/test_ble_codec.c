#include "ble_codec.h"
#include "unity.h"

#include <stdint.h>
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- LED mask ------------------------------------------------------------------------------ */

void test_led_mask_sets_one_bit_per_lit_led(void)
{
    const uint8_t none[2] = {0U, 0U};
    const uint8_t first[2] = {1U, 0U};
    const uint8_t second[2] = {0U, 1U};
    const uint8_t both[2] = {1U, 1U};

    TEST_ASSERT_EQUAL_UINT8(0U, BleCodec_LedMask(none, 2U));
    TEST_ASSERT_EQUAL_UINT8(1U, BleCodec_LedMask(first, 2U));
    TEST_ASSERT_EQUAL_UINT8(2U, BleCodec_LedMask(second, 2U));
    TEST_ASSERT_EQUAL_UINT8(3U, BleCodec_LedMask(both, 2U));
}

void test_led_mask_treats_any_non_zero_value_as_on(void)
{
    const uint8_t odd[2] = {255U, 2U};

    TEST_ASSERT_EQUAL_UINT8(3U, BleCodec_LedMask(odd, 2U));
}

void test_led_mask_ignores_entries_beyond_the_count(void)
{
    const uint8_t leds[4] = {0U, 1U, 1U, 1U};

    TEST_ASSERT_EQUAL_UINT8(2U, BleCodec_LedMask(leds, 2U));
    TEST_ASSERT_EQUAL_UINT8(0U, BleCodec_LedMask(leds, 0U));
}

void test_led_mask_never_goes_beyond_eight_bits(void)
{
    const uint8_t leds[10] = {1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U};

    TEST_ASSERT_EQUAL_UINT8(0xFFU, BleCodec_LedMask(leds, 10U));
}

void test_led_mask_of_a_null_array_is_zero(void)
{
    TEST_ASSERT_EQUAL_UINT8(0U, BleCodec_LedMask(NULL, 2U));
}

/* ---- LED write unpack ---------------------------------------------------------------------- */

void test_led_bit_reads_each_bit(void)
{
    TEST_ASSERT_EQUAL_UINT8(1U, BleCodec_LedBit(0x01U, 0U));
    TEST_ASSERT_EQUAL_UINT8(0U, BleCodec_LedBit(0x01U, 1U));
    TEST_ASSERT_EQUAL_UINT8(1U, BleCodec_LedBit(0x02U, 1U));
    TEST_ASSERT_EQUAL_UINT8(1U, BleCodec_LedBit(0x80U, 7U));
}

void test_led_bit_beyond_the_mask_width_is_zero(void)
{
    TEST_ASSERT_EQUAL_UINT8(0U, BleCodec_LedBit(0xFFU, 8U));
    TEST_ASSERT_EQUAL_UINT8(0U, BleCodec_LedBit(0xFFU, 100U));
}

void test_a_mask_survives_the_round_trip(void)
{
    for (unsigned int mask = 0U; mask < 4U; mask++)
    {
        uint8_t leds[2];

        leds[0] = BleCodec_LedBit((uint8_t)mask, 0U);
        leds[1] = BleCodec_LedBit((uint8_t)mask, 1U);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)mask, BleCodec_LedMask(leds, 2U));
    }
}

/* ---- status packet ------------------------------------------------------------------------- */

void test_status_packet_layout(void)
{
    uint8_t packet[BLE_CODEC_STATUS_LENGTH];
    const uint8_t expected[BLE_CODEC_STATUS_LENGTH] = {3U, 2U, 0x78U, 0x56U, 0x34U, 0x12U};

    BleCodec_StatusPacket(packet, 3U, 2U, 0x12345678UL);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, packet, BLE_CODEC_STATUS_LENGTH);
}

void test_status_packet_heartbeat_extremes(void)
{
    uint8_t packet[BLE_CODEC_STATUS_LENGTH];
    const uint8_t zero[BLE_CODEC_STATUS_LENGTH] = {0U, 0U, 0U, 0U, 0U, 0U};
    const uint8_t full[BLE_CODEC_STATUS_LENGTH] = {6U, 3U, 0xFFU, 0xFFU, 0xFFU, 0xFFU};

    BleCodec_StatusPacket(packet, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(zero, packet, BLE_CODEC_STATUS_LENGTH);
    BleCodec_StatusPacket(packet, 6U, 3U, UINT32_MAX);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(full, packet, BLE_CODEC_STATUS_LENGTH);
}

void test_status_packet_writes_exactly_six_bytes(void)
{
    uint8_t buffer[8] = {0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU};

    BleCodec_StatusPacket(buffer, 1U, 1U, 1U);
    TEST_ASSERT_EQUAL_UINT8(0xAAU, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0xAAU, buffer[7]);
}

void test_status_packet_to_null_does_nothing(void)
{
    BleCodec_StatusPacket(NULL, 1U, 1U, 1U);
}

/* ---- UUID ---------------------------------------------------------------------------------- */

void test_uuid_bytes_for_the_three_demo_ids(void)
{
    uint8_t uuid[BLE_CODEC_UUID_LENGTH];
    const uint8_t service[BLE_CODEC_UUID_LENGTH] = {0xEEU, 0xFFU, 0xC0U, 0x00U, 0x5FU, 0x0BU,
                                                    0x4AU, 0x9DU, 0x2BU, 0x4EU, 0x3EU, 0x4CU,
                                                    0x01U, 0x00U, 0x7CU, 0x8AU};

    BleCodec_Uuid128(uuid, 0x01U);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(service, uuid, BLE_CODEC_UUID_LENGTH);
    BleCodec_Uuid128(uuid, 0x02U);
    TEST_ASSERT_EQUAL_UINT8(0x02U, uuid[12]);
    BleCodec_Uuid128(uuid, 0x03U);
    TEST_ASSERT_EQUAL_UINT8(0x03U, uuid[12]);
}

void test_uuid_only_the_id_byte_differs(void)
{
    uint8_t first[BLE_CODEC_UUID_LENGTH];
    uint8_t second[BLE_CODEC_UUID_LENGTH];

    BleCodec_Uuid128(first, 0x10U);
    BleCodec_Uuid128(second, 0xF0U);
    for (unsigned int i = 0U; i < BLE_CODEC_UUID_LENGTH; i++)
    {
        if (i == 12U)
        {
            TEST_ASSERT_NOT_EQUAL(first[i], second[i]);
        }
        else
        {
            TEST_ASSERT_EQUAL_UINT8(first[i], second[i]);
        }
    }
}

void test_uuid_to_null_does_nothing(void)
{
    BleCodec_Uuid128(NULL, 1U);
}

/* ---- name copy ----------------------------------------------------------------------------- */

void test_a_short_name_is_copied_whole(void)
{
    uint8_t name[32];

    TEST_ASSERT_EQUAL_UINT(4U, BleCodec_CopyName(name, sizeof(name), "Demo"));
    TEST_ASSERT_EQUAL_STRING("Demo", (const char*)name);
}

void test_a_name_that_just_fits_is_kept(void)
{
    uint8_t name[8];

    TEST_ASSERT_EQUAL_UINT(7U, BleCodec_CopyName(name, sizeof(name), "1234567"));
    TEST_ASSERT_EQUAL_STRING("1234567", (const char*)name);
}

void test_a_long_name_is_truncated_and_terminated(void)
{
    uint8_t name[8];

    TEST_ASSERT_EQUAL_UINT(7U, BleCodec_CopyName(name, sizeof(name), "12345678"));
    TEST_ASSERT_EQUAL_STRING("1234567", (const char*)name);
    TEST_ASSERT_EQUAL_UINT(7U, BleCodec_CopyName(name, sizeof(name), "a much longer name"));
    TEST_ASSERT_EQUAL_STRING("a much ", (const char*)name);
}

void test_an_empty_or_null_name_gives_an_empty_string(void)
{
    uint8_t name[8] = {'x', 'x', 'x', 'x', 'x', 'x', 'x', 'x'};

    TEST_ASSERT_EQUAL_UINT(0U, BleCodec_CopyName(name, sizeof(name), ""));
    TEST_ASSERT_EQUAL_UINT8(0U, name[0]);
    name[0] = 'x';
    TEST_ASSERT_EQUAL_UINT(0U, BleCodec_CopyName(name, sizeof(name), NULL));
    TEST_ASSERT_EQUAL_UINT8(0U, name[0]);
}

void test_a_capacity_of_one_stores_only_the_terminator(void)
{
    uint8_t name[2] = {'x', 'x'};

    TEST_ASSERT_EQUAL_UINT(0U, BleCodec_CopyName(name, 1U, "abc"));
    TEST_ASSERT_EQUAL_UINT8(0U, name[0]);
    TEST_ASSERT_EQUAL_UINT8('x', name[1]);
}

void test_a_null_destination_or_zero_capacity_writes_nothing(void)
{
    uint8_t name[2] = {'x', 'x'};

    TEST_ASSERT_EQUAL_UINT(0U, BleCodec_CopyName(NULL, 8U, "abc"));
    TEST_ASSERT_EQUAL_UINT(0U, BleCodec_CopyName(name, 0U, "abc"));
    TEST_ASSERT_EQUAL_UINT8('x', name[0]);
}

void test_the_name_copy_never_writes_past_the_capacity(void)
{
    uint8_t buffer[12];

    memset(buffer, 0xAA, sizeof(buffer));
    (void)BleCodec_CopyName(buffer, 8U, "a name that is far too long");
    for (unsigned int i = 8U; i < sizeof(buffer); i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0xAAU, buffer[i]);
    }
}

/* ---- advertising name element -------------------------------------------------------------- */

void test_the_local_name_element_starts_with_the_type_byte(void)
{
    uint8_t element[33];
    const uint8_t name[] = {'B', 'L', 'E', 0U};
    const uint8_t expected[4] = {0x09U, 'B', 'L', 'E'};

    TEST_ASSERT_EQUAL_UINT(4U, BleCodec_LocalNameAd(element, sizeof(element), name));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, element, 4U);
}

void test_the_local_name_element_is_cut_to_fit(void)
{
    uint8_t element[4] = {0U, 0U, 0U, 0U};
    const uint8_t name[] = {'A', 'B', 'C', 'D', 'E', 0U};
    const uint8_t expected[4] = {0x09U, 'A', 'B', 'C'};

    TEST_ASSERT_EQUAL_UINT(4U, BleCodec_LocalNameAd(element, sizeof(element), name));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, element, 4U);
}

void test_the_local_name_element_of_an_empty_or_null_name_is_just_the_type(void)
{
    uint8_t element[4];
    const uint8_t empty[] = {0U};

    TEST_ASSERT_EQUAL_UINT(1U, BleCodec_LocalNameAd(element, sizeof(element), empty));
    TEST_ASSERT_EQUAL_UINT8(0x09U, element[0]);
    TEST_ASSERT_EQUAL_UINT(1U, BleCodec_LocalNameAd(element, sizeof(element), NULL));
}

void test_the_local_name_element_with_no_room_writes_nothing(void)
{
    uint8_t element[2] = {'x', 'x'};
    const uint8_t name[] = {'A', 0U};

    TEST_ASSERT_EQUAL_UINT(0U, BleCodec_LocalNameAd(NULL, 8U, name));
    TEST_ASSERT_EQUAL_UINT(0U, BleCodec_LocalNameAd(element, 0U, name));
    TEST_ASSERT_EQUAL_UINT8('x', element[0]);
    TEST_ASSERT_EQUAL_UINT(1U, BleCodec_LocalNameAd(element, 1U, name));
    TEST_ASSERT_EQUAL_UINT8(0x09U, element[0]);
    TEST_ASSERT_EQUAL_UINT8('x', element[1]);
}

/* ---- differential tests against the original ble_app.c code -------------------------------- */

static unsigned next_random(unsigned* state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8;
}

void test_led_mask_matches_the_original_for_every_pair_of_bytes(void)
{
    for (unsigned int a = 0U; a < 256U; a++)
    {
        for (unsigned int b = 0U; b < 256U; b++)
        {
            const uint8_t leds[2] = {(uint8_t)a, (uint8_t)b};
            const uint8_t original = (uint8_t)((leds[0] ? 1U : 0U) | (leds[1] ? 2U : 0U));

            TEST_ASSERT_EQUAL_UINT8(original, BleCodec_LedMask(leds, 2U));
        }
    }
}

void test_led_write_unpack_matches_the_original_expression(void)
{
    for (unsigned int mask = 0U; mask < 256U; mask++)
    {
        for (unsigned int i = 0U; i < 2U; i++)
        {
            const uint8_t original = (uint8_t)((mask >> i) & 1U);

            TEST_ASSERT_EQUAL_UINT8(original, BleCodec_LedBit((uint8_t)mask, i));
        }
    }
}

void test_status_packet_matches_the_original_byte_assembly(void)
{
    unsigned state = 11U;

    for (unsigned int step = 0U; step < 20000U; step++)
    {
        const uint8_t ble_status = (uint8_t)next_random(&state);
        const uint8_t mask = (uint8_t)next_random(&state);
        const uint32_t heartbeat = (next_random(&state) << 8U) ^ next_random(&state);
        uint8_t original[BLE_CODEC_STATUS_LENGTH];
        uint8_t actual[BLE_CODEC_STATUS_LENGTH];

        original[0] = ble_status;
        original[1] = mask;
        original[2] = (uint8_t)(heartbeat & 0xFFU);
        original[3] = (uint8_t)((heartbeat >> 8U) & 0xFFU);
        original[4] = (uint8_t)((heartbeat >> 16U) & 0xFFU);
        original[5] = (uint8_t)((heartbeat >> 24U) & 0xFFU);
        BleCodec_StatusPacket(actual, ble_status, mask, heartbeat);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(original, actual, BLE_CODEC_STATUS_LENGTH);
    }
}

/* The original UUID128(x) macro. */
#define ORIGINAL_UUID128(x)                                                                        \
    {0xee, 0xff, 0xc0, 0x00, 0x5f, 0x0b, 0x4a, 0x9d, 0x2b, 0x4e, 0x3e, 0x4c, (x), 0x00, 0x7c, 0x8a}

void test_uuid_matches_the_original_macro_for_every_id(void)
{
    for (unsigned int id = 0U; id < 256U; id++)
    {
        const uint8_t original[BLE_CODEC_UUID_LENGTH] = ORIGINAL_UUID128(id);
        uint8_t actual[BLE_CODEC_UUID_LENGTH];

        BleCodec_Uuid128(actual, (uint8_t)id);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(original, actual, BLE_CODEC_UUID_LENGTH);
    }
}

/* The original BLE_App_StartAdvertising copy into a 32-byte device_name. */
static void original_copy_name(uint8_t device_name[32], const char* name)
{
    size_t name_length = strlen(name);

    if (name_length >= 32U)
    {
        name_length = 32U - 1U;
    }
    (void)memcpy(device_name, name, name_length);
    device_name[name_length] = '\0';
}

void test_name_copy_and_advertising_element_match_the_original(void)
{
    unsigned state = 5U;

    for (unsigned int step = 0U; step < 20000U; step++)
    {
        char name[80];
        const unsigned length = next_random(&state) % 70U;
        uint8_t expected_name[32];
        uint8_t actual_name[32];
        uint8_t expected_element[33];
        uint8_t actual_element[33];
        size_t name_length;

        for (unsigned int i = 0U; i < length; i++)
        {
            name[i] = (char)(1U + (next_random(&state) % 255U));
        }
        name[length] = '\0';

        memset(expected_name, 0xAA, sizeof(expected_name));
        memset(actual_name, 0xAA, sizeof(actual_name));
        original_copy_name(expected_name, name);
        (void)BleCodec_CopyName(actual_name, sizeof(actual_name), name);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_name, actual_name, sizeof(expected_name));

        /* The original ble_set_discoverable builds the element from device_name. */
        name_length = strlen((const char*)expected_name);
        memset(expected_element, 0xBB, sizeof(expected_element));
        memset(actual_element, 0xBB, sizeof(actual_element));
        expected_element[0] = 0x09U;
        (void)memcpy(&expected_element[1], expected_name, name_length);
        TEST_ASSERT_EQUAL_UINT(
            name_length + 1U,
            BleCodec_LocalNameAd(actual_element, sizeof(actual_element), actual_name));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_element, actual_element, name_length + 1U);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_led_mask_sets_one_bit_per_lit_led);
    RUN_TEST(test_led_mask_treats_any_non_zero_value_as_on);
    RUN_TEST(test_led_mask_ignores_entries_beyond_the_count);
    RUN_TEST(test_led_mask_never_goes_beyond_eight_bits);
    RUN_TEST(test_led_mask_of_a_null_array_is_zero);
    RUN_TEST(test_led_bit_reads_each_bit);
    RUN_TEST(test_led_bit_beyond_the_mask_width_is_zero);
    RUN_TEST(test_a_mask_survives_the_round_trip);
    RUN_TEST(test_status_packet_layout);
    RUN_TEST(test_status_packet_heartbeat_extremes);
    RUN_TEST(test_status_packet_writes_exactly_six_bytes);
    RUN_TEST(test_status_packet_to_null_does_nothing);
    RUN_TEST(test_uuid_bytes_for_the_three_demo_ids);
    RUN_TEST(test_uuid_only_the_id_byte_differs);
    RUN_TEST(test_uuid_to_null_does_nothing);
    RUN_TEST(test_a_short_name_is_copied_whole);
    RUN_TEST(test_a_name_that_just_fits_is_kept);
    RUN_TEST(test_a_long_name_is_truncated_and_terminated);
    RUN_TEST(test_an_empty_or_null_name_gives_an_empty_string);
    RUN_TEST(test_a_capacity_of_one_stores_only_the_terminator);
    RUN_TEST(test_a_null_destination_or_zero_capacity_writes_nothing);
    RUN_TEST(test_the_name_copy_never_writes_past_the_capacity);
    RUN_TEST(test_the_local_name_element_starts_with_the_type_byte);
    RUN_TEST(test_the_local_name_element_is_cut_to_fit);
    RUN_TEST(test_the_local_name_element_of_an_empty_or_null_name_is_just_the_type);
    RUN_TEST(test_the_local_name_element_with_no_room_writes_nothing);
    RUN_TEST(test_led_mask_matches_the_original_for_every_pair_of_bytes);
    RUN_TEST(test_led_write_unpack_matches_the_original_expression);
    RUN_TEST(test_status_packet_matches_the_original_byte_assembly);
    RUN_TEST(test_uuid_matches_the_original_macro_for_every_id);
    RUN_TEST(test_name_copy_and_advertising_element_match_the_original);
    return UNITY_END();
}
