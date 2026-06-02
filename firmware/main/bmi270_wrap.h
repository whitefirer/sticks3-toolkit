#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

bool bmi270_probe(i2c_master_bus_handle_t bus, uint8_t *out_addr);
void bmi270_init(void);
bool bmi270_read_accel(float *ax, float *ay, float *az);
bool bmi270_read_gyro(float *gx, float *gy, float *gz);

#ifdef __cplusplus
}
#endif
