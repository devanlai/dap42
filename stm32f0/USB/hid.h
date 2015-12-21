#ifndef HID_H_INCLUDED
#define HID_H_INCLUDED

#include <libopencm3/usb/usbd.h>
#include "hid_defs.h"

extern const struct full_usb_hid_descriptor hid_function;

extern void hid_setup(usbd_device* usbd_dev,
                      HostInFunction report_send_cb,
                      HostOutFunction report_recv_cb);

extern bool hid_send_report(const uint8_t* report, size_t len);

#endif
