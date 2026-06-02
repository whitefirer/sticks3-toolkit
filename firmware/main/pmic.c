#include "pmic.h"
#include <stdio.h>
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#define M5PM1_ADDR            0x6E
#define M5PM1_REG_DEVICE_ID   0x00
#define M5PM1_REG_PWR_CFG     0x06
#define M5PM1_REG_HOLD_CFG    0x07
#define M5PM1_REG_GPIO_MODE   0x10
#define M5PM1_REG_GPIO_OUT    0x11
#define M5PM1_REG_GPIO_DRV    0x13
#define M5PM1_REG_GPIO_FUNC0  0x16
#define M5PM1_REG_BAT_L       0x22
#define M5PM1_REG_IRQ_MASK1   0x43
#define M5PM1_REG_IRQ_MASK2   0x44
#define M5PM1_REG_IRQ_MASK3   0x45
#define M5PM1_PWR_CFG_LDO_EN   BIT(2)
#define M5PM1_PWR_CFG_LED_CTRL BIT(4)
#define M5PM1_GPIO2_L3B_EN     BIT(2)
#define M5PM1_IRQ_SYS_5VIN_INSERT  BIT(0)
#define M5PM1_IRQ_SYS_5VIN_REMOVE  BIT(1)

static i2c_master_dev_handle_t g_pmic = NULL;

static bool pmic_read(uint8_t reg, uint8_t *val) {
    return g_pmic && i2c_master_transmit_receive(g_pmic, &reg, 1, val, 1, 100) == ESP_OK;
}

static void pmic_write(uint8_t reg, uint8_t val) {
    if (g_pmic) i2c_master_transmit(g_pmic, (uint8_t[]){reg, val}, 2, 100);
}

static bool pmic_find(i2c_port_t port, gpio_num_t sda, gpio_num_t scl,
                      i2c_master_bus_handle_t *out_bus) {
    i2c_master_bus_config_t bc = {};
    bc.i2c_port = port; bc.sda_io_num = sda; bc.scl_io_num = scl;
    bc.clk_source = I2C_CLK_SRC_DEFAULT;
    bc.glitch_ignore_cnt = 7;
    bc.flags.enable_internal_pullup = true;
    if (i2c_new_master_bus(&bc, out_bus) != ESP_OK) return false;

    i2c_device_config_t dc = {};
    dc.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dc.device_address = M5PM1_ADDR;
    dc.scl_speed_hz = 100000;
    if (i2c_master_bus_add_device(*out_bus, &dc, &g_pmic) != ESP_OK) return false;

    uint8_t id;
    return pmic_read(M5PM1_REG_DEVICE_ID, &id);
}

bool pmic_init(i2c_master_bus_handle_t *out_bus, i2c_port_t *out_port,
               int *out_sda, int *out_scl) {
    const struct { i2c_port_t p; gpio_num_t s; gpio_num_t c; } t[] = {
        {I2C_NUM_1, GPIO_NUM_47, GPIO_NUM_48},
        {I2C_NUM_1, GPIO_NUM_48, GPIO_NUM_47},
        {I2C_NUM_0, GPIO_NUM_47, GPIO_NUM_48},
        {I2C_NUM_0, GPIO_NUM_48, GPIO_NUM_47},
    };
    for (int i = 0; i < 4; i++) {
        if (pmic_find(t[i].p, t[i].s, t[i].c, out_bus)) {
            *out_port = t[i].p; *out_sda = t[i].s; *out_scl = t[i].c;
            break;
        }
    }
    if (!g_pmic) { printf("[WARN] PMIC not found\n"); return false; }
    printf("[OK] PMIC found: port=%d SDA=%d SCL=%d\n", *out_port, *out_sda, *out_scl);

    uint8_t v;
    pmic_read(M5PM1_REG_PWR_CFG, &v);
    pmic_write(M5PM1_REG_PWR_CFG, (v | M5PM1_PWR_CFG_LDO_EN) & ~M5PM1_PWR_CFG_LED_CTRL);
    pmic_read(M5PM1_REG_HOLD_CFG, &v);
    pmic_write(M5PM1_REG_HOLD_CFG, v | BIT(0));
    // GPIO2 = LCD backlight
    pmic_read(M5PM1_REG_GPIO_FUNC0, &v);
    v &= ~M5PM1_GPIO2_L3B_EN; pmic_write(M5PM1_REG_GPIO_FUNC0, v);
    pmic_read(M5PM1_REG_GPIO_MODE, &v);
    v |= M5PM1_GPIO2_L3B_EN;  pmic_write(M5PM1_REG_GPIO_MODE, v);
    pmic_read(M5PM1_REG_GPIO_DRV, &v);
    v &= ~M5PM1_GPIO2_L3B_EN; pmic_write(M5PM1_REG_GPIO_DRV, v);
    pmic_read(M5PM1_REG_GPIO_OUT, &v);
    v |= M5PM1_GPIO2_L3B_EN;  pmic_write(M5PM1_REG_GPIO_OUT, v);
    pmic_write(M5PM1_REG_IRQ_MASK1, 0x1F);
    pmic_write(M5PM1_REG_IRQ_MASK3, 0x07);
    pmic_write(M5PM1_REG_IRQ_MASK2, 0x3F & ~(M5PM1_IRQ_SYS_5VIN_INSERT | M5PM1_IRQ_SYS_5VIN_REMOVE));
    return true;
}
