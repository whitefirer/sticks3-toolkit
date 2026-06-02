#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_W 135
#define DISPLAY_H 240

void display_init(void);
void display_clear(uint16_t color);
void display_fill(int x, int y, int w, int h, uint16_t color);
void display_text(int x, int y, const char *str, uint16_t fg, uint16_t bg);
void display_set_backlight(bool on);
void display_flush(void);

#ifdef __cplusplus
}
#endif
