#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define VENDOR_ID   0xdead   // Replace with your USB device's vendor ID
#define PRODUCT_ID  0xbeef   // Replace with your USB device's product ID

typedef union {
    struct {
        uint8_t sw0     : 1;
        uint8_t sw1     : 1;
        uint8_t sw2     : 1;
        uint8_t rsvd    : 5;
    } bits;
    uint8_t byte;
} pb_status_t;

static bool sendControlTransfer(uint16_t val);
static int rxInterruptData(uint8_t *pb_report);

libusb_context* ctx = NULL;
libusb_device_handle* dev_handle = NULL;

int main() {
    pb_status_t pb_status = {0x00};
    pb_status_t prev_pb_status = {0x00};
    uint16_t led_flash_rate = 0x00;
    bool start_stop = true;

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

    // Claim the interface
    if(libusb_claim_interface(dev_handle, 0) < 0) {
        fprintf(stderr, "Could not claim interface\n");
        libusb_close(dev_handle);
        libusb_exit(ctx);
        return 1;
    }

    sendControlTransfer(0x00);

    int rxBytes = 0;
    while(1) {
        rxBytes = rxInterruptData(&pb_status.byte);

        if(rxBytes && pb_status.byte) {
            if(pb_status.bits.sw0 && !prev_pb_status.bits.sw0 && start_stop) {
                if(led_flash_rate < 2500)
                    led_flash_rate += 100;

                printf("Increase delay to %dms\n", led_flash_rate);

                sendControlTransfer(led_flash_rate);
            }
            else if(pb_status.bits.sw1 && !prev_pb_status.bits.sw1 && start_stop) {
                if(led_flash_rate >= 100)
                    led_flash_rate -= 100;

                printf("Decrease delay to %dms\n", led_flash_rate);

                sendControlTransfer(led_flash_rate);
            }
            else if(pb_status.bits.sw2 && !prev_pb_status.bits.sw2) {
                start_stop = !start_stop;

                printf("Start/Stop: %d\n", start_stop);

                sendControlTransfer((start_stop ? led_flash_rate : 0));
            }
        }

        // Clear our RX bytes
        rxBytes = 0;
        // Store our previous PB status
        prev_pb_status = pb_status;
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

static int rxInterruptData(uint8_t *pb_report) {
    int readBytes = 0;
    int ret = libusb_interrupt_transfer(
        dev_handle,
        LIBUSB_ENDPOINT_IN | 0x01,
        pb_report,
        1,
        &readBytes,
        0
    );

    if(ret == LIBUSB_ERROR_TIMEOUT) {
        fprintf(stderr, "Error reading interrupt data: %d\n", ret);
    }

    return readBytes;
}
