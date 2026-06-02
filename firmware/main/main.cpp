#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "display.h"
#include "pmic.h"
#include "bmi270_wrap.h"

static const char *TAG = "toolkit";

#define BTN_A GPIO_NUM_11
#define BTN_B GPIO_NUM_12

static i2c_master_bus_handle_t g_i2c_bus;
static i2c_port_t g_i2c_port;
static int g_i2c_sda, g_i2c_scl;
static bool g_i2c_ready = false;
static bool g_pmic_ok = false;
static bool g_bmi_found = false;
static uint8_t g_bmi_addr = 0;

// Pre-swapped for display_flush
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_GREEN  0xE007
#define C_RED    0x00F8
#define C_BLUE   0x1F00
#define C_CYAN   0xFF07
#define C_GRAY   0xEF7B

// ── Button helpers ──────────────────────────────────

static void button_init(void) {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << BTN_A) | (1ULL << BTN_B);
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&cfg);
}
static bool a_down(void) { return gpio_get_level(BTN_A) == 0; }
static bool b_down(void) { return gpio_get_level(BTN_B) == 0; }

// ── I2C scan helper ─────────────────────────────────

static bool i2c_probe(uint8_t addr) {
    i2c_master_dev_handle_t dev;
    i2c_device_config_t dc = {};
    dc.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dc.device_address = addr;
    dc.scl_speed_hz = 100000;
    if (i2c_master_bus_add_device(g_i2c_bus, &dc, &dev) != ESP_OK) return false;
    uint8_t d = 0;
    bool ok = i2c_master_transmit_receive(dev, &d, 1, &d, 1, 100) == ESP_OK;
    i2c_master_bus_rm_device(dev);
    return ok;
}

static const char *dev_name(uint8_t addr) {
    switch (addr) {
    case 0x6E: return "M5PM1";
    case 0x68: return "BMI270";
    case 0x69: return "BMI270 alt";
    case 0x18: return "QMP6988?";
    default:   return "";
    }
}

// ── Scan display ────────────────────────────────────

static void do_scan(void) {
    display_clear(C_BLACK);

    uint8_t m[6]; esp_read_mac(m, ESP_MAC_WIFI_STA);
    char buf[64];
    snprintf(buf, sizeof(buf), "MAC %02X:%02X:%02X:%02X:%02X:%02X",
             m[0],m[1],m[2],m[3],m[4],m[5]);

    int y = 0;
    display_text(0, y, "StickS3 Toolkit", C_CYAN, C_BLACK); y += 18;
    display_text(0, y, buf, C_WHITE, C_BLACK); y += 18;
    display_text(0, y, "ESP32-S3 I2C Scan", C_WHITE, C_BLACK); y += 24;

    int found = 0;
    for (int a = 1; a < 128 && y < 200; a++) {
        if (!i2c_probe(a)) continue;
        found++;
        const char *n = dev_name(a);
        snprintf(buf, sizeof(buf), "0x%02X %s", a, n[0] ? n : "");
        display_text(0, y, buf, C_GREEN, C_BLACK);
        y += 18;
    }
    if (!found) { display_text(0, y, "no devices", C_RED, C_BLACK); y += 20; }

    snprintf(buf, sizeof(buf), "BMI270: %s (0x%02X)",
             g_bmi_found ? "OK" : "--", g_bmi_addr);
    display_text(0, y, buf, g_bmi_found ? C_CYAN : C_RED, C_BLACK); y += 20;

    display_text(0, y, g_pmic_ok ? "PMIC M5PM1 OK" : "PMIC not found",
                 g_pmic_ok ? C_CYAN : C_RED, C_BLACK);

    static int n = 0;
    snprintf(buf, sizeof(buf), "Scan #%d", ++n);
    display_text(0, 210, buf, C_GRAY, C_BLACK);
    display_text(0, 222, "A:Rescan  B:IMU", C_GRAY, C_BLACK);
    display_flush();

    printf("\n=== Scan #%d ===\n", n);
    printf("  I2C port=%d SDA=%d SCL=%d  devices:%d\n", g_i2c_port, g_i2c_sda, g_i2c_scl, found);
    printf("  BMI270: %s  PMIC: %s\n", g_bmi_found ? "OK" : "--", g_pmic_ok ? "OK" : "--");
}

// ── IMU display ─────────────────────────────────────

static void show_imu(void) {
    bmi270_init();
    char buf[64];
    float ax, ay, az, gx, gy, gz;
    bool on = true, running = true;

    while (running) {
        bmi270_read_accel(&ax, &ay, &az);
        bmi270_read_gyro(&gx, &gy, &gz);

        display_clear(C_BLACK);
        display_text(0, 0, "BMI270 6-Axis IMU", C_CYAN, C_BLACK);
        display_text(0, 20, "Accel (g)", C_WHITE, C_BLACK);
        snprintf(buf, sizeof(buf), "X:%+6.2f", ax); display_text(0, 38, buf, C_GREEN, C_BLACK);
        snprintf(buf, sizeof(buf), "Y:%+6.2f", ay); display_text(0, 56, buf, C_GREEN, C_BLACK);
        snprintf(buf, sizeof(buf), "Z:%+6.2f", az); display_text(0, 74, buf, C_GREEN, C_BLACK);
        display_text(0, 96, "Gyro (deg/s)", C_WHITE, C_BLACK);
        snprintf(buf, sizeof(buf), "X:%+7.1f", gx); display_text(0, 114, buf, C_BLUE, C_BLACK);
        snprintf(buf, sizeof(buf), "Y:%+7.1f", gy); display_text(0, 132, buf, C_BLUE, C_BLACK);
        snprintf(buf, sizeof(buf), "Z:%+7.1f", gz); display_text(0, 150, buf, C_BLUE, C_BLACK);
        display_text(0, 222, "A:Back  B:Light", C_GRAY, C_BLACK);
        display_flush();

        if (a_down()) { vTaskDelay(pdMS_TO_TICKS(50)); if (a_down()) { while (a_down()) vTaskDelay(pdMS_TO_TICKS(10)); running = false; } }
        if (b_down()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (b_down()) { on = !on; display_set_backlight(on); while (b_down()) vTaskDelay(pdMS_TO_TICKS(10)); }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ── Main ────────────────────────────────────────────

extern "C" void app_main(void) {
    nvs_flash_init();

    g_pmic_ok = pmic_init(&g_i2c_bus, &g_i2c_port, &g_i2c_sda, &g_i2c_scl);
    g_i2c_ready = g_pmic_ok;  // bus is ready if PMIC found
    display_init();
    button_init();
    vTaskDelay(pdMS_TO_TICKS(300));

    if (g_i2c_ready) {
        g_bmi_found = bmi270_probe(g_i2c_bus, &g_bmi_addr);
        do_scan();
    } else {
        display_clear(C_BLACK);
        display_text(0, 40, "I2C bus error!", C_RED, C_BLACK);
        display_text(0, 60, "No PMIC detected.", C_RED, C_BLACK);
        display_text(0, 80, "Check serial log.", C_GRAY, C_BLACK);
        display_flush();
    }

    while (1) {
        if (a_down()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (a_down() && g_i2c_ready) { g_bmi_found = bmi270_probe(g_i2c_bus, &g_bmi_addr); do_scan(); }
            while (a_down()) vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (b_down()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (b_down()) {
                if (g_bmi_found) show_imu();
                else { static bool bl = true; bl = !bl; display_set_backlight(bl); }
            }
            while (b_down()) vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
