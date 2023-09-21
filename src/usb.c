#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "usb.h"

#define CONTROL_EP_BANK_SIZE 8

// USB standard request codes
#define GET_STATUS 0x00
#define CLEAR_FEATURE 0x01
#define SET_FEATURE 0x03
#define SET_ADDRESS 0x05
#define GET_DESCRIPTOR 0x06
#define SET_DESCRIPTOR 0x07
#define GET_CONFIGURATION 0x08
#define SET_CONFIGURATION 0x09
#define GET_INTERFACE 0x0A
#define SET_INTERFACE 0x0B
#define SYNCH_FRAME 0x0C

// USB descriptor types
#define DESC_DEVICE 1
#define DESC_CONFIG 2
#define DESC_STRING 3

// USB String descriptor indexes
#define DESC_STRING_LANG    0   // Language descriptor index
#define DESC_STRING_MANUF   1   // Manufacturer string descriptor index
#define DESC_STRING_PROD    2   // Product string descriptor index
#define DESC_STRING_SERIAL  3   // Serial number string descriptor index

static bool _endpoint_init(void);
static void _sendDescriptor(const uint8_t* descriptor, uint16_t length);
static void _processSetupPacket(void);

// USB descriptors (example, replace with your own)
const uint8_t PROGMEM DeviceDescriptor[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device == 1)
    0x01, 0x01, // bcdUSB (USB 1.1 for Full Speed)
    0x00,       // bDeviceClass (0 for composite device)
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x08,       // bMaxPacketSize0 (8 bytes)
    0xad, 0xde, // idVendor (0xdead)
    0xef, 0xbe, // idProduct (0xbeef)
    0x01, 0x00, // bcdDevice (Device version)
    0x01,       // iManufacturer (Index of manufacturer string descriptor)
    0x02,       // iProduct (Index of product string descriptor)
    0x03,       // iSerialNumber (Index of serial number string descriptor)
    0x01        // bNumConfigurations (Number of configurations)
};

const uint8_t PROGMEM ConfigDescriptor[] = {
    0x09,       // bLength
    0x02,       // bDescriptorType (Configuration == 2)
    0x12, 0x00, // wTotalLength (Total length of configuration descriptor and sub-descriptors)
    0x01,       // bNumInterfaces (Number of interfaces in this configuration)
    0x01,       // bConfigurationValue (Configuration value, must be 1)
    0x00,       // iConfiguration (Index of string descriptor for this configuration)
    0x80,       // bmAttributes (Bus-powered, no remote wakeup)
    0xFA,       // bMaxPower (Maximum power consumption, 500mA)
    0x09,       // bLength = 0x09, length of descriptor in bytes
    0x04,       // bDescriptorType = 0x04, (Interface == 4)
    0x00,       // bInterfaceNumber = 0;
    0x00,       // bAlternateSetting = 0;
    0x00,       // bNumEndpoints = USB_Endpoints;
    0xFF,       // bInterfaceClass = 0xFF,
    0xFF,       // bInterfaceSubClass = 0xFF
    0xFF,       // bInterfaceProtocol = 0xFF
    0x00        // iInterface = 0, Index for string descriptor interface
};

const uint8_t PROGMEM LanguageDescriptor[] = {
    0x04,     // bLength - Length of sting language descriptor including this byte
    0x03,     // bDescriptorType - (String == 3)
    0x09,0x04 // wLANGID[x] - (0x0409 = English USA)
};

const uint8_t PROGMEM ManufacturerStringDescriptor[] = {
    24,         // bLength - Length of string descriptor (including this byte)
    0x03,       // bDescriptorType - (String == 3)
    'e',0x00,   // bString - Unicode Encoded String (16 Bit)
    'v',0x00,
    'e',0x00,
    'r',0x00,
    'y',0x00,
    'd',0x00,
    'a',0x00,
    'y',0x00,
    'd',0x00,
    'e',0x00,
    'v',0x00
};

const uint8_t PROGMEM ProductStringDescriptor[] = {
    40,         // bLength - Length of string descriptor (including this byte)
    0x03,       // bDescriptorType - (String == 3)
    'a',0x00,   // bString - Unicode Encoded String (16 Bit)
    'v',0x00,
    'r',0x00,
    ' ',0x00,
    'u',0x00,
    's',0x00,
    'b',0x00,
    ' ',0x00,
    'm',0x00,
    'a',0x00,
    'd',0x00,
    'e',0x00,
    ' ',0x00,
    's',0x00,
    'i',0x00,
    'm',0x00,
    'p',0x00,
    'l',0x00,
    'e',0x00,
};

const uint8_t PROGMEM SerialStringDescriptor[] = {
    0x0A,       // bLength - Length of string descriptor (including this byte)
    0x03,       // bDescriptorType - (String == 3)
    '2',0x00,   // bString - Unicode Encoded String (16 Bit)
    '0',0x00,
    '2',0x00,
    '3',0x00
};

usb_controlWrite_rx_cb_t _setupWrite_cb = NULL;
usb_controlRead_tx_cb_t _setupRead_cb = NULL;

ISR(USB_GEN_vect) {
    // Check if a USB reset sequence was received from the host
    if (UDINT & (1<<EORSTI)) {
        // Clear the interrupt flag
        UDINT &= ~(1<<EORSTI);
        // Init our device endpoints
        _endpoint_init();
        // Enable our Setup RX interrupt. This causes
        // an ISR to fire when a Setup packet is received
        UEIENX |= (1<<RXSTPE);
    }
}

ISR(USB_COM_vect) {
    // Check which endpoint caused the interrupt
    switch (UEINT) {
        case (1<<EPINT0):
            // Select EP 0 before checking the interrupt register
            UENUM = 0;
            // Check if we received a Setup packet
            if (UEINTX & (1<<RXSTPI)) {
                // Handle the setup packet
                _processSetupPacket();
            }
            break;

        default:
            break;
    }
}

void usb_init(usb_controlWrite_rx_cb_t onControlWriteCb, usb_controlRead_tx_cb_t onControlReadCb) {
    // Power-On USB pads regulator
    UHWCON |= (1 << UVREGE);

    // VBUS int enable, VBUS pad enable, USB Controller enable
    USBCON |= (1 << USBE) | (1 << OTGPADE) | (1 << FRZCLK);

    // Toggle the FRZCLK to get WAKEUP IRQ
    USBCON &= ~(1 << FRZCLK);
    USBCON |= (1 << FRZCLK);

    // Set the PLL input divisor to be 1:2 since we have
    // at 16MHz input (we want 8MHz output on the PLL)
    PLLCSR |= (1 << PINDIV);

    // Default USB postscaler is fine as it generates
    // the 48Mhz clock from the 8Mhz PLL input by default

    // Start the PLL
    PLLCSR |= (1 << PLLE);

    // Wait for the PLL to lock
    while (!(PLLCSR &(1<<PLOCK)));

    // Leave power saving mode
    USBCON &= ~(1 << FRZCLK);

    // Store our CB if we received one
    if(onControlWriteCb != NULL) {
        _setupWrite_cb = onControlWriteCb;
    }

    if(onControlReadCb != NULL) {
        _setupRead_cb = onControlReadCb;
    }

    // Attach the device by clearing the detach bit
    // This is acceptable in the case of a bus-powered device
    // otherwise you would initiate this step based on a
    // VBUS detection.
    UDCON &= ~(1 << DETACH);

    // Enable the USB Reset IRQ. This IRQ fires when the
    // host sends a USB reset to the device to kickoff
    // USB enumeration. Init of the USB will continue in the
    // ISRs when the reset signal is received.
    UDIEN |= (1 << EORSTE);
}

static bool _endpoint_init(void) {
    // Select Endpoint 0
    UENUM = 0x00;
    // Reset the endpoint fifo
    UERST = 0x7F;
    UERST = 0x00;
    // Enable the endpoint
    UECONX |= (1 << EPEN);
    // Configure endpoint as Control with OUT direction
    UECFG0X = 0;
    // Configure endpoint size
    UECFG1X = (1 << EPSIZE1) | (1 << EPSIZE0);
    // Allocate the endpoint buffers
    UECFG1X |= (1 << ALLOC);
    // Enable the endpoint interrupt
    UEIENX |= (1 << RXSTPE) | (1 << RXOUTE);
    // Check if endpoint configuration is ok
    return((UESTA0X & (1 << CFGOK)));
}

static void _sendDescriptor(const uint8_t* descriptor, uint16_t length) {
    // See section 22.12.2 of https://ww1.microchip.com/downloads/en/devicedoc/atmel-7766-8-bit-avr-atmega16u4-32u4_datasheet.pdf
    // for an illustration of the "Control Read" process. Specifically the "DATA" and "STATUS"
    // portion of the timing diagram are handled here.

    // We are going to chunk the descriptor into 8 byte packets. Looping until we have finished
    for(uint16_t i = 1; i <= length; i++) {
        if(UEINTX & (1 << RXOUTI)) {
            // We received an OUT packet which means
            // the HOST wants us to abort.
            // Clear the RXOUTI bit to acknowledge the packet
            UEINTX &= ~(1 << RXOUTI);
            return;
        }

        // Load the next byte from our descriptor
        UEDATX = pgm_read_byte(&descriptor[i-1]);
        // If our packet is full
        if(((i%8) == 0)) {
            // Clear the TXINI bit to initiate the transfer
            UEINTX &= ~(1 << TXINI);
            // Wait for transmission to complete (TXINI set) or
            // the HOST to abort (RXOUTI set)
            while (!(UEINTX & ((1 << RXOUTI) | (1<<TXINI))));
        }
    }
    // Go ahead and transmit the remaining data (if there is any) if the HOST
    // hasn't asked us to abort
    if((!(UEINTX & (1 << RXOUTI)))) {
        // Clear the TXINI bit to initiate the transfer
        // to send the remaining data we may have queued up
        UEINTX &= ~(1 << TXINI);
        // Wait for the ACK back from the host (RXOUTI set)
        while (!(UEINTX & (1 << RXOUTI)));
    }

    // Clear the RXOUTI bit to acknowledge the packet
    UEINTX &= ~(1 << RXOUTI);
}

static void  _processSetupPacket(void) {
    // Read the 8 bytes from the setup packet. Depending on the type of request
    // each value may have a different use/meaning. Reference "The SETUP Packet" section
    // of https://www.usbmadesimple.co.uk/ums_4.htm for more information on what each byte
    // means for a given request type.
    uint8_t bmRequestType = UEDATX;
    uint8_t bRequest = UEDATX;
    uint8_t wValue_l = UEDATX;
    uint8_t wValue_h = UEDATX;
    uint8_t wIndex_l = UEDATX;
    uint8_t wIndex_h = UEDATX;
    uint8_t wLength_l = UEDATX;
    uint8_t wLength_h = UEDATX;
    uint16_t descriptorLength = 0;
    uint16_t wLength = wLength_l | (wLength_h << 8);
    uint16_t wValue = wValue_l | (wValue_h << 8);
    uint8_t _setup_read_buff[CONTROL_EP_BANK_SIZE] = {0x00};

    // Ack the received setup package by clearing the RXSTPI bit
    UEINTX &= ~(1 << RXSTPI);

    if ((bmRequestType & 0x60) == 0) { // Standard request type
        switch (bRequest) {
            case GET_STATUS:
                // Reply with 16 bits for our status. We are self powered, no remote-wakeup
                // and we are not halted.
                UEDATX = 0;
                UEDATX = 0;
                // Clear the TXINI bit to initiate the transfer
                UEINTX &= ~(1 << TXINI);
                // Wait for ZLP from host
                while (!(UEINTX & (1 << RXOUTI)));
                // Clear the RXOUTI bit to acknowledge the packet
                UEINTX &= ~(1 << RXOUTI);
                break;

            case SET_ADDRESS:
                // Section 22.7 of https://ww1.microchip.com/downloads/en/devicedoc/atmel-7766-8-bit-avr-atmega16u4-32u4_datasheet.pdf
                // Device stores received address in the UDADDR register.
                UDADDR = (wValue_l & 0x7F);
                // Device should then respond with a ZLP to acknowledge the request
                UEINTX &= ~(1 << TXINI);
                // Wait for the bank to become ready again (TIXINI set)
                while (!(UEINTX & (1<<TXINI)));
                // After sending the ZLP, the device should apply the address by setting the ADDEN bit
                UDADDR |= (1 << ADDEN);
                break;

            case GET_DESCRIPTOR:
                switch (wValue_h) {
                    case DESC_DEVICE:
                        // Retrieve the Device desc length from the descriptor
                        // structure. This is the first byte of the descriptor.
                        descriptorLength = pgm_read_byte(&DeviceDescriptor[0]);
                        // Send it back to the host
                        _sendDescriptor((uint8_t*) DeviceDescriptor, descriptorLength);
                        break;

                    case DESC_CONFIG:
                        // The Host will first request the base Config descriptor which is of length
                        // 9 to determine how many interfaces are available.
                        // However, once the host determines how many interfaces are available it will then
                        // do another Config descriptor read with the full length of the descriptor.
                        descriptorLength = wLength_l + (wLength_h << 8);
                        // Send the descriptor with the requested length
                        _sendDescriptor((uint8_t*)ConfigDescriptor, descriptorLength);
                        break;

                    case DESC_STRING:
                        switch (wValue_l) {
                            case DESC_STRING_LANG:
                                // Retrieve the Language desc length from the descriptor
                                // structure. This is the first byte of the descriptor.
                                descriptorLength = pgm_read_byte(&LanguageDescriptor[0]);
                                // Send it back to the host
                                _sendDescriptor((uint8_t*) LanguageDescriptor,descriptorLength);
                                break;

                            case DESC_STRING_MANUF:
                                // Retrieve the Manufacturer string desc length from the descriptor
                                // structure. This is the first byte of the descriptor.
                                descriptorLength = pgm_read_byte(&ManufacturerStringDescriptor[0]);
                                // Send it back to the host
                                _sendDescriptor((uint8_t*) ManufacturerStringDescriptor,descriptorLength);
                                break;

                            case DESC_STRING_PROD:
                                // Retrieve the Product string desc length from the descriptor
                                // structure. This is the first byte of the descriptor.
                                descriptorLength = pgm_read_byte(&ProductStringDescriptor[0]);
                                // Send it back to the host
                                _sendDescriptor((uint8_t*) ProductStringDescriptor,descriptorLength);
                                break;

                            case DESC_STRING_SERIAL:
                                // Retrieve the Serial string desc length from the descriptor
                                // structure. This is the first byte of the descriptor.
                                descriptorLength = pgm_read_byte(&SerialStringDescriptor[0]);
                                // Send it back to the host
                                _sendDescriptor((uint8_t*) SerialStringDescriptor,descriptorLength);
                                break;

                            default:
                                break;
                        }
                        break;

                    default:
                        break;
                }
                break;

            case SET_CONFIGURATION:
                // Select EP 0
                UENUM = 0;
                // Reply with a ZLP to acknowledge the request
                UEINTX &= ~(1 << TXINI);
                // Wait for the bank to be ready again
                while (!(UEINTX & (1<<TXINI)));
                break;

            default:
                // Invalid request was sent. Reply with a STALl
                UECONX |= (1 << STALLRQ);
                break;
        }
    }
    else if((bmRequestType & 0x60) == 0x40) { // Vendor specific request type
        switch(bRequest) {
            case 0x01:
                // If we have a callback stored, call it with the value
                if(_setupWrite_cb != NULL) {
                    _setupWrite_cb(wValue);
                }

                // Reply with a ZLP
                UEINTX &= ~(1 << TXINI);
                // Wait for the bank to become ready again
                while (!(UEINTX & (1<<TXINI)));
                break;

            case 0x02:
                if(_setupRead_cb != NULL) {
                    // Call our callback to get the data to send back
                    uint16_t txLen = _setupRead_cb(_setup_read_buff,
                        (wLength > CONTROL_EP_BANK_SIZE ?
                            CONTROL_EP_BANK_SIZE :
                            wLength));
                    // Send the data back to the host
                    for(uint16_t i = 0; i < txLen; i++) {
                        UEDATX = _setup_read_buff[i];
                    }
                    // Clear the TXINI bit to initiate the transfer
                    UEINTX &= ~(1 << TXINI);
                    // Wait for the bank to become ready again
                    while (!(UEINTX & (1<<TXINI)));
                }
                else {
                    // No callbakc was provided so
                    // reply with a stall
                    UECONX |= (1 << STALLRQ);
                }
                break;

            default:
                // Unsupported vendor specific request. Reply with a STALL
                UECONX |= (1 << STALLRQ);
                break;
        }
    }
    else { // Invalid request type
        // Reply with a STALL
        UECONX |= (1 << STALLRQ);
    }
}
