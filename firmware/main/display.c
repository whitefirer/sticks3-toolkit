#include "display.h"
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

static const char *TAG = "display";

#define PIN_MOSI 39
#define PIN_SCLK 40
#define PIN_DC   45
#define PIN_RST  21
#define PIN_CS   41
#define PIN_BL   38

// Exact match with authstick: native portrait 135x240
#define LCD_X_GAP 52
#define LCD_Y_GAP 40
#define PIXEL_CLOCK_HZ (20 * 1000 * 1000)

static esp_lcd_panel_handle_t g_panel;
// Framebuffer: panel native 135 cols x 240 rows, row-major
static uint16_t g_fb[135 * 240];

#include "font8x16.h"

void display_clear(uint16_t color) {
    for (int i = 0; i < 135 * 240; i++) g_fb[i] = color;
}

void display_fill(int x, int y, int w, int h, uint16_t color) {
    for (int row = y; row < y + h && row < 240; row++)
        for (int col = x; col < x + w && col < 135; col++)
            g_fb[row * 135 + col] = color;
}

void display_text(int x, int y, const char *str, uint16_t fg, uint16_t bg) {
    while (*str) {
        char c = *str;
        if (c == '\n') { y += 16; x = 0; str++; continue; }
        if (c < 32 || c > 126) c = '?';
        const uint8_t *glyph = font8x16[c - 32];
        for (int row = 0; row < 16; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                int px = x + col, py = y + row;
                if (px < 0 || px >= 135 || py < 0 || py >= 240) continue;
                if (bits & (0x80 >> col))
                    g_fb[py * 135 + px] = fg;
            }
        }
        x += 8;
        str++;
    }
}

void display_set_backlight(bool on) {
    gpio_set_level(PIN_BL, on ? 1 : 0);
}

void display_flush(void) {
    // ESP32 LE → ST7789 BE
    for (int i = 0; i < 135 * 240; i++) {
        uint16_t v = g_fb[i];
        g_fb[i] = (v >> 8) | (v << 8);
    }
    esp_lcd_panel_draw_bitmap(g_panel, 0, 0, 135, 240, g_fb);
    for (int i = 0; i < 135 * 240; i++) {
        uint16_t v = g_fb[i];
        g_fb[i] = (v >> 8) | (v << 8);
    }
}

void display_init(void) {
    gpio_config_t bl_cfg = {};
    bl_cfg.pin_bit_mask = (1ULL << PIN_BL);
    bl_cfg.mode = GPIO_MODE_OUTPUT;
    gpio_config(&bl_cfg);
    gpio_set_level(PIN_BL, 1);

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = PIN_MOSI;
    bus_cfg.miso_io_num = -1;
    bus_cfg.sclk_io_num = PIN_SCLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 135 * 240 * 2 + 8;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num = PIN_DC;
    io_cfg.cs_gpio_num = PIN_CS;
    io_cfg.spi_mode = 0;
    io_cfg.pclk_hz = PIXEL_CLOCK_HZ;
    io_cfg.trans_queue_depth = 10;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.lcd_param_bits = 8;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io));

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = PIN_RST;
    panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_cfg.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_cfg, &g_panel));

    esp_lcd_panel_reset(g_panel);
    esp_lcd_panel_init(g_panel);
    esp_lcd_panel_invert_color(g_panel, true);
    // NO swap_xy, NO mirror — exact match with authstick
    esp_lcd_panel_mirror(g_panel, false, false);
    esp_lcd_panel_set_gap(g_panel, LCD_X_GAP, LCD_Y_GAP);

    // Clear to black
    display_clear(0x0000);
    display_flush();
    esp_lcd_panel_disp_on_off(g_panel, true);
    ESP_LOGI(TAG, "ST7789 ready %dx%d gaps=%d,%d", 135, 240, LCD_X_GAP, LCD_Y_GAP);
}
