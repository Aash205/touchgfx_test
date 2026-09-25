/**
  ******************************************************************************
  * @file    usb_cdc_log.c
  * @brief   USB CDC-ACM log sink (see usb_cdc_log.h).
  ******************************************************************************
  */

#include "usb_cdc_log.h"
#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"

#define RING_SIZE          2048U     /* power of two */
#define DRAIN_STACK_BYTES  2048U
#define DRAIN_PRIORITY     14U       /* below every application thread */
#define DRAIN_PERIOD_TICKS 5U        /* 50 ms @ 100 Hz */
#define WRITE_TIMEOUT_TICKS 20U      /* give up on a stalled host after 200 ms */

static UX_SLAVE_CLASS_CDC_ACM *volatile s_cdc;
static volatile UINT s_configured;
static volatile UINT s_port_open;

static unsigned char s_ring[RING_SIZE];
static volatile unsigned s_head;   /* written by producers */
static volatile unsigned s_tail;   /* written by the drain thread */

static TX_THREAD s_thread;
static ULONG s_stack[DRAIN_STACK_BYTES / sizeof(ULONG)];

static VOID drain_entry(ULONG input);

UINT UsbCdcLog_Init(void)
{
  return tx_thread_create(&s_thread, "USB CDC log", drain_entry, 0,
                          s_stack, sizeof(s_stack),
                          DRAIN_PRIORITY, DRAIN_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
}

void UsbCdcLog_OnActivate(VOID *cdc_acm_instance)
{
  ULONG timeout = WRITE_TIMEOUT_TICKS;

  s_cdc = (UX_SLAVE_CLASS_CDC_ACM *)cdc_acm_instance;
  ux_device_class_cdc_acm_ioctl(s_cdc, UX_SLAVE_CLASS_CDC_ACM_IOCTL_SET_WRITE_TIMEOUT, (VOID *)timeout);

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

  TX_DISABLE
  while (n < size && (s_head - s_tail) < RING_SIZE) {
    s_ring[s_head & (RING_SIZE - 1U)] = data[n++];
    s_head++;
  }
  TX_RESTORE

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
    if (s_configured && s_cdc != UX_NULL) {
      UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER line_state = {0};
      if (ux_device_class_cdc_acm_ioctl(s_cdc,
                                        UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE,
                                        &line_state) == UX_SUCCESS) {
        s_port_open = line_state.ux_slave_class_cdc_acm_parameter_dtr ? 1U : 0U;
      }
    } else {
      s_port_open = 0;
    }

    if (!s_port_open) {
      tx_thread_sleep(DRAIN_PERIOD_TICKS);
      continue;
    }

    while (s_tail != s_head && n < sizeof(chunk)) {
      chunk[n++] = s_ring[s_tail & (RING_SIZE - 1U)];
      s_tail++;
    }

    if (n > 0 && s_port_open && s_cdc != UX_NULL) {
      ULONG actual = 0;
      ux_device_class_cdc_acm_write(s_cdc, chunk, n, &actual);   /* result ignored: best effort */
    }

    if (s_tail == s_head) {
      tx_thread_sleep(DRAIN_PERIOD_TICKS);
    }
  }
}
