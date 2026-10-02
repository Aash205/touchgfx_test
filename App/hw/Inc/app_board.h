/**
 ******************************************************************************
 * @file    app_board.h
 * @brief   Peripheral handles that CubeMX defines in Core/Src/main.c and the
 *          application glue uses.
 ******************************************************************************
 */

#ifndef APP_BOARD_H
#define APP_BOARD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32l4xx_hal.h"

/** LPUART1: the command console and the UART log sink. */
/* cppcheck-suppress misra-c2012-8.5 -- CubeMX's generated stm32l4xx_it.c declares it as well */
extern UART_HandleTypeDef hlpuart1;

/** USB OTG full-speed device controller used by USBX. */
/* cppcheck-suppress misra-c2012-8.5 -- CubeMX's generated stm32l4xx_it.c declares it as well */
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

#ifdef __cplusplus
}
#endif

#endif /* APP_BOARD_H */
