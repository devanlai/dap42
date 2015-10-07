#ifndef USB_CONF_H
#define USB_CONF_H

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>

#define USB_SERIAL_NUM_LENGTH 24

typedef void (*ReceiveReportFunction)(char* data, uint16_t len);
typedef void (*SendReportFunction)(char* data, uint16_t* len);

extern void set_usb_serial_number(const char* serial);
extern usbd_device* hid_setup(ReceiveReportFunction recv_cb, SendReportFunction send_cb);

#endif
