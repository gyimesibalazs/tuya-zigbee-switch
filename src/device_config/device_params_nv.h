#ifndef DEVICE_CONFIG_DEVICE_PARAMS_NV_H_
#define DEVICE_CONFIG_DEVICE_PARAMS_NV_H_

#include <stdint.h>

/*
 * Global device parameter: multi-press reset count.
 *   0  => multi-press factory reset disabled
 *   N  => factory-reset after N consecutive presses
 * Default: 10, persisted in NVM.
 */

extern uint8_t g_multi_press_reset_count;

/*
 * Global device parameter: radio transmit power in dBm (TX_POWER_MIN_DBM .. TX_POWER_MAX_DBM).
 * Default: TX_POWER_MAX_DBM for mains devices, 3 for battery devices; a value set by the user is
 * persisted in NVM and then also applies to battery devices.
 */
#define TX_POWER_MIN_DBM    (-25)
#define TX_POWER_MAX_DBM    10

extern int8_t  g_tx_power_dbm;
extern uint8_t g_tx_power_stored; // 1: the value comes from the user

void device_params_load_from_nv(void);
void device_params_set_multi_press_reset_count(uint8_t value);
void device_params_set_tx_power(int8_t dbm);

#endif /* DEVICE_CONFIG_DEVICE_PARAMS_NV_H_ */
