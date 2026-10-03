/**
 ******************************************************************************
 * @file    usb_cdc_log.h
 * @brief   USB CDC-ACM log sink (USBX). Regeneration-safe: application-owned file.
 *
 *          Bytes queued with UsbCdcLog_Write() are drained by a low-priority thread
 *          into the CDC-ACM bulk-in endpoint while a host has the port open.
 *          With no host (or before the USB device is enabled) writes are dropped, so
 *          logging never blocks.
 ******************************************************************************
 */
#ifndef USB_CDC_LOG_H
#define USB_CDC_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tx_api.h"

/** Create the drain thread. Call from App_ThreadX_Init(). */
UINT UsbCdcLog_Init(void);

/**
 * Bind the USBX device controller to the USB peripheral and start it. Call once from the USBX
 * device thread, after the USBX stack and the CDC class exist. Returns without starting the
 * controller (and logs nothing) if the binding fails.
 */
void UsbCdcLog_StartDevice(void);

/** Hooks: call from USBD_CDC_ACM_Activate / _Deactivate in ux_device_cdc_acm.c. */
void UsbCdcLog_OnActivate(VOID* cdc_acm_instance);
void UsbCdcLog_OnDeactivate(VOID* cdc_acm_instance);

/** Non-zero while a host has the CDC-ACM interface active. */
int UsbCdcLog_IsActive(void);

/** Queue bytes for the host. Returns the number accepted (0 when inactive / full). */
unsigned UsbCdcLog_Write(const unsigned char* data, unsigned size);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_LOG_H */
