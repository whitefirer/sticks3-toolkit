#include "bmi270_wrap.h"
#include <stdio.h>
#include <stdlib.h>
#include "bmi270.h"  // official driver

static bmi270_handle_t *g_handle = NULL;

bool bmi270_probe(i2c_master_bus_handle_t bus, uint8_t *out_addr) {
    for (int i = 0; i < 2; i++) {
        uint8_t addr = (i == 0) ? 0x68 : 0x69;
        bmi270_driver_config_t cfg = {
            .interface = BMI270_USE_I2C,
            .i2c_bus = bus,
            .addr = addr,
        };
        bmi270_handle_t *h = NULL;
        if (bmi270_create(&cfg, &h) != ESP_OK) continue;
        uint8_t id;
        if (bmi270_get_chip_id(h, &id) == ESP_OK && id == 0x24) {
            g_handle = h;
            *out_addr = addr;
            printf("[BMI270] Found at 0x%02X (official)\n", addr);
            return true;
        }
        free(h);
    }
    return false;
}

void bmi270_init(void) {
    if (!g_handle) return;
    bmi270_config_t cfg = {};
    cfg.acce_range = BMI270_ACC_RANGE_16_G;
    cfg.gyro_range = BMI270_GYR_RANGE_2000_DPS;
    cfg.acce_odr = BMI270_ACC_ODR_100_HZ;
    cfg.gyro_odr = BMI270_GYR_ODR_100_HZ;
    esp_err_t r = bmi270_start(g_handle, &cfg);
    printf("[BMI270] Start: %s\n", r == ESP_OK ? "OK" : "FAIL");
}

bool bmi270_read_accel(float *ax, float *ay, float *az) {
    if (!g_handle) { *ax=*ay=*az=0; return false; }
    return bmi270_get_acce_data(g_handle, ax, ay, az) == ESP_OK;
}

bool bmi270_read_gyro(float *gx, float *gy, float *gz) {
    if (!g_handle) { *gx=*gy=*gz=0; return false; }
    return bmi270_get_gyro_data(g_handle, gx, gy, gz) == ESP_OK;
}
