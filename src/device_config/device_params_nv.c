#include "device_params_nv.h"
#include "hal/nvm.h"
#include "hal/zigbee.h"
#include "nvm_items.h"

uint8_t g_multi_press_reset_count = 10;
int8_t  g_tx_power_dbm            = TX_POWER_MAX_DBM;
uint8_t g_tx_power_stored         = 0;

void device_params_load_from_nv(void) {
    uint8_t          value;
    hal_nvm_status_t st =
        hal_nvm_read(NV_ITEM_MULTI_PRESS_RESET_COUNT, sizeof(value),
                     (uint8_t *)&value);

    if (st == HAL_NVM_SUCCESS) {
        g_multi_press_reset_count = value;
    }

    int8_t dbm;
    st = hal_nvm_read(NV_ITEM_TX_POWER, sizeof(dbm), (uint8_t *)&dbm);
    if (st == HAL_NVM_SUCCESS && dbm >= TX_POWER_MIN_DBM && dbm <= TX_POWER_MAX_DBM) {
        g_tx_power_dbm    = dbm;
        g_tx_power_stored = 1;
        hal_zigbee_set_tx_power(dbm);
    }
}

void device_params_set_tx_power(int8_t dbm) {
    if (dbm < TX_POWER_MIN_DBM) {
        dbm = TX_POWER_MIN_DBM;
    }
    if (dbm > TX_POWER_MAX_DBM) {
        dbm = TX_POWER_MAX_DBM;
    }
    g_tx_power_dbm    = dbm;
    g_tx_power_stored = 1;
    hal_nvm_write(NV_ITEM_TX_POWER, sizeof(dbm), (uint8_t *)&dbm);
    hal_zigbee_set_tx_power(dbm);
}

void device_params_set_multi_press_reset_count(uint8_t value) {
    g_multi_press_reset_count = value;
    hal_nvm_write(NV_ITEM_MULTI_PRESS_RESET_COUNT, sizeof(value),
                  (uint8_t *)&value);
}
