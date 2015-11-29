#ifndef CDC_USB_CONF_H
#define CDC_USB_CONF_H

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#define USB_CDC_MAX_PACKET_SIZE 64
#define USB_SERIAL_NUM_LENGTH   24

typedef void (*ReceiveDataFunction)(uint8_t* data, uint16_t len);
typedef void (*TransmitDataFunction)(uint8_t* data, uint16_t* len);

extern void cdc_set_usb_serial_number(const char* serial);
extern usbd_device* cdc_setup(ReceiveDataFunction rx_cb, TransmitDataFunction tx_cb);

#endif
