#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "usb.h"

#define LED_STAT_DDR        (DDRD)
#define LED_STAT_PORT       (PORTD)
#define LED_STAT_PIN        (4)

uint16_t my_led_state = 0;

void onUsbControlWrite(uint16_t rxData) {
    my_led_state = rxData;
}

uint16_t onUsbControlRead(uint8_t *txData, const uint16_t requestedTxLen) {
    uint16_t txLen = 1;

    txData[0] = 0x03;

    return txLen;
}

int main(void) {
    // Set our LED port as an output
    LED_STAT_DDR |= (1 << LED_STAT_PIN);

    // Init USB and provide it our callback function
    // to be called when data is received via a
    // Control Write transfer
    usb_init(onUsbControlWrite, onUsbControlRead);

    // Enable global interrupts
    sei();

    while(1) {
        if(my_led_state) {
            LED_STAT_PORT |= (1 << LED_STAT_PIN);
        }
        else {
            LED_STAT_PORT &= ~(1 << LED_STAT_PIN);
        }
    }
}
