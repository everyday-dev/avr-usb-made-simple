#ifndef _USB_H_
#define _USB_H_

#include <stdint.h>

typedef void (*usb_controlWrite_rx_cb_t)(uint16_t rxData);
typedef uint16_t (*usb_controlRead_tx_cb_t)(uint8_t *txData, const uint16_t requestedTxLen);

void usb_init(usb_controlWrite_rx_cb_t onControlWriteCb, usb_controlRead_tx_cb_t onControlReadCb);
void usb_update(void);

#endif //_USB_H_