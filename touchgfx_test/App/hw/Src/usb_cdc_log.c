/**
  ******************************************************************************
  * @file    usb_cdc_log.c
  * @brief   USB CDC-ACM log sink (see usb_cdc_log.h).
  ******************************************************************************
  */

#include "usb_cdc_log.h"
#include "app_board.h"
#include "ring.h"
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"
#include "ux_dcd_stm32.h"

#define RING_SIZE          2048U     /* power of two */
#define DRAIN_STACK_BYTES  2048U
#define DRAIN_PRIORITY     14U       /* below every application thread */
#define DRAIN_PERIOD_TICKS 5U        /* 50 ms @ 100 Hz */
#define WRITE_TIMEOUT_TICKS 20U      /* give up on a stalled host after 200 ms */

static UX_SLAVE_CLASS_CDC_ACM *volatile s_cdc;
static volatile UINT s_configured;
static volatile UINT s_port_open;

_Static_assert((RING_SIZE != 0U) && ((RING_SIZE & (RING_SIZE - 1U)) == 0U),
               "RING_SIZE must be a non-zero power of two");
static unsigned char s_ring_storage[RING_SIZE];
/* Statically initialised: log writes can arrive before UsbCdcLog_Init() runs. */
static Ring_t s_ring = RING_INITIALIZER(s_ring_storage);   /* producers push, the drain thread pops */

static VOID drain_entry(ULONG input);

UINT UsbCdcLog_Init(void)
{
  /* ThreadX keeps pointers to these for the life of the program, so they are static. */
  static TX_THREAD s_thread;
  static ULONG s_stack[DRAIN_STACK_BYTES / sizeof(ULONG)];
  static CHAR s_thread_name[] = {'U', 'S', 'B', ' ', 'C', 'D', 'C', ' ', 'l', 'o', 'g', '\0'};

  return tx_thread_create(&s_thread, s_thread_name, drain_entry, 0,
                          s_stack, sizeof(s_stack),
                          DRAIN_PRIORITY, DRAIN_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
}

void UsbCdcLog_StartDevice(void)
{
  /* The PCD itself is initialized before ThreadX starts.  Complete the USBX
     device-controller binding here, once the USBX stack and CDC class exist. */
  (void)HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x100U);
  (void)HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0U, 0x10U); /* EP0 control */
  (void)HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1U, 0x10U); /* EP1 CDC bulk IN */
  (void)HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2U, 0x20U); /* EP2 CDC notify IN */

  /* cppcheck-suppress misra-c2012-11.4 -- USB_OTG_FS is the HAL's fixed peripheral address */
  if (ux_dcd_stm32_initialize((ULONG)USB_OTG_FS, (ULONG)&hpcd_USB_OTG_FS) == (UINT)UX_SUCCESS)
  {
    (void)HAL_PCD_Start(&hpcd_USB_OTG_FS);
  }
}

void UsbCdcLog_OnActivate(VOID *cdc_acm_instance)
{
  ULONG timeout = WRITE_TIMEOUT_TICKS;

  /* cppcheck-suppress misra-c2012-11.5 -- USBX hands the class instance over as VOID* */
  s_cdc = cdc_acm_instance;
  (void)ux_device_class_cdc_acm_ioctl(s_cdc, UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_WRITE_TIMEOUT,
                                      /* cppcheck-suppress misra-c2012-11.6 -- the USBX ioctl takes the timeout value in a VOID* argument */
                                      (VOID *)timeout);

  /* Keep messages produced during boot. They are drained as soon as the host
     finishes enumeration and opens the CDC port. */
  s_configured = 1;
  s_port_open = 0;
}

void UsbCdcLog_OnDeactivate(VOID *cdc_acm_instance)
{
  (void)cdc_acm_instance;
  s_configured = 0;
  s_port_open = 0;
  s_cdc = UX_NULL;
}

int UsbCdcLog_IsActive(void)
{
  return (int)s_port_open;
}

unsigned UsbCdcLog_Write(const unsigned char *data, unsigned size)
{
  unsigned n = 0;
  TX_INTERRUPT_SAVE_AREA

  if ((data != UX_NULL) && (size > 0U))
  {
    TX_DISABLE
    n = Ring_Push(&s_ring, data, size);
    TX_RESTORE
  }

  return n;
}

static VOID drain_entry(ULONG input)
{
  static UCHAR chunk[64];   /* one full-speed bulk packet */

  (void)input;

  for (;;) {
    unsigned n = 0;

    /* Enumeration alone does not mean a terminal is listening. Wait for the
       host to assert DTR so boot messages are not lost before Serial Monitor
       opens the COM port. */
    if ((s_configured != 0U) && (s_cdc != UX_NULL)) {
      UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER line_state = {0};
      if (ux_device_class_cdc_acm_ioctl(s_cdc,
                                        UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE,
                                        &line_state) == (UINT)UX_SUCCESS)
      {
        s_port_open = line_state.ux_slave_class_cdc_acm_parameter_dtr ? 1U : 0U;
      }
    } else {
      s_port_open = 0;
    }

    if (s_port_open == 0U) {
      (void)tx_thread_sleep(DRAIN_PERIOD_TICKS);
      continue;
    }

    n = Ring_Pop(&s_ring, chunk, (unsigned)sizeof(chunk));

    if ((n > 0U) && (s_port_open != 0U) && (s_cdc != UX_NULL))
    {
      ULONG actual = 0;
      UINT write_status = ux_device_class_cdc_acm_write(s_cdc, chunk, n, &actual);
      (void)write_status; /* A failed CDC write drops this best-effort log chunk. */
    }

    if (Ring_Count(&s_ring) == 0U)
    {
      (void)tx_thread_sleep(DRAIN_PERIOD_TICKS);
    }
  }
}
