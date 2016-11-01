#ifndef USB_H
#define USB_H
#include <stdlib.h>

#include <libopencm3/stm32/st_usbfs.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#define USB_USE_INT 1

#define USB_PORT GPIOA
#define USB_DM_PIN GPIO11
#define USB_DP_PIN GPIO12

void usb_setup(void);
#endif