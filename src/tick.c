#include <stdio.h>
#include <stdio.h>
#include <avr/interrupt.h>

#define TICK_PERIOD (1) // ms

/*! @brief Current tick val in 2ms increments */
static uint32_t tick_val = 0x0000;

/*!
 * @brief This API initiliazes the tick module and timer
 */
void tick_init(void) {
    // Disable all interrupts
    cli();

    // Configure TIMER0 for a CLK/64 pre-scaler
    TCCR0A &= 0x00;
    TCCR0B |= (0x01 << CS01) | (0x01 << CS00);
    // Enable the overflow interrupt
    TIMSK0 |= (0x01 << TOIE0);

    // Enable all interrupts
    sei();
}

/*!
 * @brief This API returns the current tick value
 */
uint32_t tick_getTick(void) {
    return tick_val;
}

/*!
 * @brief This API returns time since a passed in ref time
 */
uint32_t tick_timeSince(const uint32_t ref) {
    // If the ref time given is a time
    // greater than our current tick val
    // return 0.
    if( ref > tick_val )
        return 0;

    // Otherwise, return the difference of the ref
    // and the current tick.
    return (tick_val - ref);
}

/*!
 * @brief ISR for the Timer0 overflow interrupt
 */
ISR(TIMER0_OVF_vect)
{
    if( tick_val >= UINT32_MAX ) {
        tick_val = 0x0000;
    }

    tick_val += TICK_PERIOD;
}
