#ifndef USB_CONF_H
#define USB_CONF_H

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>

extern usbd_device* hid_setup(void);

#endif
