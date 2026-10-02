#include "unity.h"
#include "ws169_status.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- HAL code to driver status ------------------------------------------------------------- */

void test_the_hal_codes_have_the_values_of_the_stm32_hal(void)
{
    TEST_ASSERT_EQUAL_UINT(0U, WS169_HAL_OK);
    TEST_ASSERT_EQUAL_UINT(1U, WS169_HAL_ERROR);
    TEST_ASSERT_EQUAL_UINT(2U, WS169_HAL_BUSY);
    TEST_ASSERT_EQUAL_UINT(3U, WS169_HAL_TIMEOUT);
}

void test_ok_busy_and_timeout_do_not_depend_on_the_transfer_kind(void)
{
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_OK, WS169_StatusFromHal(WS169_HAL_OK, false));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_OK, WS169_StatusFromHal(WS169_HAL_OK, true));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_SPI_BUSY, WS169_StatusFromHal(WS169_HAL_BUSY, false));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_SPI_BUSY, WS169_StatusFromHal(WS169_HAL_BUSY, true));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_TIMEOUT, WS169_StatusFromHal(WS169_HAL_TIMEOUT, false));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_TIMEOUT, WS169_StatusFromHal(WS169_HAL_TIMEOUT, true));
}

void test_a_hal_error_is_a_dma_error_only_for_a_dma_transfer(void)
{
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_SPI_ERROR, WS169_StatusFromHal(WS169_HAL_ERROR, false));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_DMA_ERROR, WS169_StatusFromHal(WS169_HAL_ERROR, true));
}

void test_an_unknown_hal_code_is_treated_as_an_error(void)
{
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_SPI_ERROR, WS169_StatusFromHal(4U, false));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_DMA_ERROR, WS169_StatusFromHal(255U, true));
    TEST_ASSERT_EQUAL_INT(WS169_STATUS_SPI_ERROR, WS169_StatusFromHal(UINT32_MAX, false));
}

/* ---- which counter a status is counted in -------------------------------------------------- */

void test_each_status_is_counted_in_the_expected_counter(void)
{
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_NONE, WS169_CounterOf(WS169_STATUS_OK));
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_NONE, WS169_CounterOf(WS169_STATUS_INVALID_ARGUMENT));
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_NONE, WS169_CounterOf(WS169_STATUS_NOT_INITIALIZED));
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_SPI, WS169_CounterOf(WS169_STATUS_SPI_ERROR));
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_SPI, WS169_CounterOf(WS169_STATUS_SPI_BUSY));
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_TIMEOUT, WS169_CounterOf(WS169_STATUS_TIMEOUT));
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_NONE, WS169_CounterOf(WS169_STATUS_RTOS_ERROR));
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_DMA, WS169_CounterOf(WS169_STATUS_DMA_ERROR));
}

void test_a_status_outside_the_enum_is_not_counted(void)
{
    TEST_ASSERT_EQUAL_INT(WS169_COUNTER_NONE, WS169_CounterOf((WS169_Status_t)99));
}

/* ---- differential test against the original driver code ------------------------------------ */

/* The original ws169_status_from_hal switch and ws169_record_status chain, with the HAL enum
 * replaced by its integer values. */
static WS169_Status_t original_status(uint32_t hal_status, bool dma_transfer)
{
    WS169_Status_t status;

    switch (hal_status)
    {
    case 0U:
        status = WS169_STATUS_OK;
        break;
    case 2U:
        status = WS169_STATUS_SPI_BUSY;
        break;
    case 3U:
        status = WS169_STATUS_TIMEOUT;
        break;
    case 1U:
    default:
        status = dma_transfer ? WS169_STATUS_DMA_ERROR : WS169_STATUS_SPI_ERROR;
        break;
    }
    return status;
}

static int original_counter(WS169_Status_t status)
{
    int counter = 0;

    if ((status == WS169_STATUS_SPI_ERROR) || (status == WS169_STATUS_SPI_BUSY))
    {
        counter = 1;
    }
    else if (status == WS169_STATUS_DMA_ERROR)
    {
        counter = 2;
    }
    else if (status == WS169_STATUS_TIMEOUT)
    {
        counter = 3;
    }
    return counter;
}

void test_status_and_counter_match_the_original_code_for_all_inputs(void)
{
    for (uint32_t hal = 0U; hal < 300U; hal++)
    {
        for (int dma = 0; dma < 2; dma++)
        {
            const WS169_Status_t status = WS169_StatusFromHal(hal, dma != 0);

            TEST_ASSERT_EQUAL_INT(original_status(hal, dma != 0), status);
            TEST_ASSERT_EQUAL_INT(original_counter(status), WS169_CounterOf(status));
        }
    }
    for (int status = 0; status < 12; status++)
    {
        TEST_ASSERT_EQUAL_INT(original_counter((WS169_Status_t)status),
                              WS169_CounterOf((WS169_Status_t)status));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_the_hal_codes_have_the_values_of_the_stm32_hal);
    RUN_TEST(test_ok_busy_and_timeout_do_not_depend_on_the_transfer_kind);
    RUN_TEST(test_a_hal_error_is_a_dma_error_only_for_a_dma_transfer);
    RUN_TEST(test_an_unknown_hal_code_is_treated_as_an_error);
    RUN_TEST(test_each_status_is_counted_in_the_expected_counter);
    RUN_TEST(test_a_status_outside_the_enum_is_not_counted);
    RUN_TEST(test_status_and_counter_match_the_original_code_for_all_inputs);
    return UNITY_END();
}
