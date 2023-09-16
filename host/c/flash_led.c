#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID   0xdead   // Replace with your USB device's vendor ID
#define PRODUCT_ID  0xbeef   // Replace with your USB device's product ID

static bool sendControlTransfer(uint16_t val);

libusb_context* ctx = NULL;
libusb_device_handle* dev_handle = NULL;

int main() {
    // Initialize libusb
    if (libusb_init(&ctx) != 0) {
        fprintf(stderr, "libusb initialization failed\n");
        return 1;
    }

    // Open the USB device using vendor and product ID
    dev_handle = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);

    if (dev_handle == NULL) {
        fprintf(stderr, "Could not open USB device\n");
        libusb_exit(ctx);
        return 1;
    }

    while(1) {
        if(sendControlTransfer(0x01)) return 1;
        sleep(1);
        if(sendControlTransfer(0x00)) return 1;
        sleep(1);
    }

    // Close the device and exit
    libusb_close(dev_handle);
    libusb_exit(ctx);

    return 0;
}

static bool sendControlTransfer(uint16_t val) {
    int result = libusb_control_transfer(
        dev_handle,
        LIBUSB_REQUEST_TYPE_VENDOR, // Request type
        0x01,                       // Request
        val,                        // Value
        0x00,                       // Index
        NULL,                       // Data to send
        0,                          // Length of data
        1000                        // Timeout (in milliseconds)
    );

    if (result < 0) {
        fprintf(stderr, "Control transfer error: %s\n", libusb_error_name(result));
        libusb_close(dev_handle);
        libusb_exit(ctx);
        return 1;
    }
}
