#ifndef __ESP_LCD_PANEL_XF024QV16A_H__
#define __ESP_LCD_PANEL_XF024QV16A_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#define CONFIG_ESP32_I8080_PIN_D0 23
#define CONFIG_ESP32_I8080_PIN_D1 32
#define CONFIG_ESP32_I8080_PIN_D2 22
#define CONFIG_ESP32_I8080_PIN_D3 33
#define CONFIG_ESP32_I8080_PIN_D4 19
#define CONFIG_ESP32_I8080_PIN_D5 25
#define CONFIG_ESP32_I8080_PIN_D6 18
#define CONFIG_ESP32_I8080_PIN_D7 26
#define CONFIG_ESP32_I8080_PIN_PCK 2
#define CONFIG_ESP32_I8080_PIN_CS 5
#define CONFIG_ESP32_I8080_PIN_DC 21
#define CONFIG_ESP32_I8080_PIN_RST 3//17
#define CONFIG_ESP32_I8080_BUS_WIDTH 8
#define CONFIG_ESP32_I8080_BUS_PSRAM_DATA_ALIGNMENT 64
#define CONFIG_ESP32_I8080_PIXEL_CLOCK_HZ 10000000
#define CONFIG_ESP32_I8080_TRANSFER_BYTES_MAX 4096

esp_err_t panel_xf024qv16a_bus_i80_init(size_t buffer_size, const esp_lcd_panel_io_handle_t *lcd_io);
extern esp_err_t esp_lcd_new_panel_xf024qv16a_i80(const esp_lcd_panel_io_handle_t *io, const esp_lcd_panel_dev_config_t *panel_dev_config,esp_lcd_panel_handle_t *ret_panel,size_t interface_buffer_size);

#endif
