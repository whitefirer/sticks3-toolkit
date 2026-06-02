#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// Returns true if PMIC found and initialized
bool pmic_init(i2c_master_bus_handle_t *out_bus, i2c_port_t *out_port,
               int *out_sda, int *out_scl);

#ifdef __cplusplus
}
#endif
