#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "usb.h"
#include "tick.h"

#define LED_STAT_DDR        (DDRD)
#define LED_STAT_PORT       (PORTD)
#define LED_STAT_PIN        (4)

#define PB_DDR              (DDRB)
#define PB_PORT             (PORTB)
#define PB_PIN              (6)

typedef union {
    struct {
        uint8_t sw0     : 1;
        uint8_t sw1     : 1;
        uint8_t sw2     : 1;
        uint8_t rsvd    : 5;
    } bits;
    uint8_t byte;
} pb_status_t;

uint16_t led_flash_rate = 0;
pb_status_t buttons = {0x00};

void onUsbControlWrite(uint16_t rxData) {
    led_flash_rate = rxData;
}

uint16_t onUsbControlRead(uint8_t *txData, const uint16_t requestedTxLen) {
    uint16_t txLen = 1;

    txData[0] = 0x03;

    return txLen;
}

int main(void) {
    uint32_t ledFlashRefTime = 0x0000;

    // Set our LED port as an output
    LED_STAT_DDR |= (1 << LED_STAT_PIN);
    // Set our PB pin as an input
    PB_DDR &= ~(1 << PB_PIN);

    // Init our Tick module allowing for async-like
    // delay functionality
    tick_init();

    // Init USB and provide it our callback function
    // to be called when data is received via a
    // Control Write transfer
    usb_init(onUsbControlWrite, onUsbControlRead);

    // Enable global interrupts
    sei();

    // Store our initial led flash reference time
    ledFlashRefTime = tick_getTick();

    while(1) {
        // Read our PIND and mask off the bottom 3 bits
        buttons.byte = (PIND & 0x07);

        // Update our status for the next interrupt transfer
        usb_sendInterruptData(&buttons.byte, 1);

        // Flash the LED if necessary
        if(led_flash_rate && (tick_timeSince(ledFlashRefTime) >= led_flash_rate)) {
            // Toggle our LED
            LED_STAT_PORT ^= (1 << LED_STAT_PIN);
            // Update our reference time
            ledFlashRefTime = tick_getTick();
        }
        else if(!led_flash_rate) {
            // Turn off our LED
            LED_STAT_PORT &= ~(1 << LED_STAT_PIN);
        }
    }
}
