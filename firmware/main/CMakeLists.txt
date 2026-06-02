idf_component_register(SRCS "main.cpp" "display.c" "pmic.c" "bmi270_wrap.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash driver esp_lcd esp_driver_spi espressif__bmi270)
