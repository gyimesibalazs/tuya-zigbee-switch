#ifndef _HAL_SYSTEM_H_
#define _HAL_SYSTEM_H_

/**
 * Reset the system/microcontroller
 */
#include <stdint.h>

void __attribute__((noreturn)) hal_system_reset(void);

void hal_factory_reset(void);

/**
 * Keep the device from going to sleep for the given time (battery devices). Used while the user
 * is operating a control whose signals must be seen without the delay of a wake-up.
 */
void hal_system_stay_awake(uint32_t ms);

#endif /* _HAL_SYSTEM_H_ */
