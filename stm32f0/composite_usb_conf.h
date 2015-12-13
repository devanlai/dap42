#ifndef COMPOSITE_USB_CONF_H
#define COMPOSITE_USB_CONF_H

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/hid.h>

#define USB_CDC_MAX_PACKET_SIZE 64
#define USB_HID_MAX_PACKET_SIZE 64
#define USB_SERIAL_NUM_LENGTH   24

#define ENDP_CDC_DATA_OUT       0x01
#define ENDP_CDC_DATA_IN        0x82
#define ENDP_CDC_COMM_IN        0x83
#define ENDP_HID_REPORT_OUT     0x04
#define ENDP_HID_REPORT_IN      0x84

#define INTF_HID                0
#define INTF_CDC_COMM           1
#define INTF_CDC_DATA           2

typedef void (*HostOutFunction)(uint8_t* data, uint16_t len);
typedef void (*HostInFunction)(uint8_t* data, uint16_t* len);

extern void cmp_set_usb_serial_number(const char* serial);
extern usbd_device* cmp_setup(HostInFunction report_send_cb,
                              HostOutFunction report_recv_cb,
                              HostOutFunction cdc_rx_cb);

#endif
