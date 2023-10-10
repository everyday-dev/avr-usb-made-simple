#ifndef _TICK_H_
#define _TICK_H_

/*!
 * @brief This API initiliazes the tick module and timer
 *
 * @param[in] void
 *
 * @returns Returns void
 */
void tick_init(void);

/*!
 * @brief This API returns the current tick value
 *
 * @param[in] void
 *
 * @returns Returns void
 */
uint32_t tick_getTick(void);

/*!
 * @brief This API returns time since a passed in ref time
 *
 * @param[in] ref : The reference in which you would like to
 * know how much time as elapsed since.
 *
 * @returns Returns the time since the passed in ref value.
 */
uint32_t tick_timeSince(const uint32_t ref);

#endif // _TICK_H_