#ifndef HAL_ZIGBEE_OTA_H
#define HAL_ZIGBEE_OTA_H

#include "hal/zigbee.h"

/**
 * Configure Zigbee cluster for over-the-air (OTA) firmware updates
 * @param cluster Zigbee cluster structure to configure
 */
void hal_ota_cluster_setup(hal_zigbee_cluster *cluster);

/** Initialize over-the-air firmware update functionality */
void hal_zigbee_init_ota();

/** @return 1 while a firmware image is being downloaded */
uint8_t hal_zigbee_ota_in_progress(void);

#endif
