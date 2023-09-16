#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID   0xdead   // Replace with your USB device's vendor ID
#define PRODUCT_ID  0xbeef   // Replace with your USB device's product ID

libusb_context* ctx = NULL;
libusb_device_handle* dev_handle = NULL;

static char sendControlRead(void);

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

    char rxValue = 0x0000;

    while(1) {
        rxValue = sendControlRead();

        printf("Value: %d\n", rxValue);

        sleep(1);
    }

    // Close the device and exit
    libusb_close(dev_handle);
    libusb_exit(ctx);

    return 0;
}

static char sendControlRead(void) {
    char rx;

    int result = libusb_control_transfer(
        dev_handle,
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN, // Request type with IN direction
        0x02,                                            // Request
        0x00,                                            // Value
        0x00,                                            // Index
        &rx,                                             // Data buffer to receive
        0x01,                                            // Length of data
        1000                                             // Timeout (in milliseconds)
    );

    if (result < 0) {
        fprintf(stderr, "Control transfer error: %s\n", libusb_error_name(result));
        libusb_close(dev_handle);
        libusb_exit(ctx);
        exit(1);
    }

    return rx;
}
