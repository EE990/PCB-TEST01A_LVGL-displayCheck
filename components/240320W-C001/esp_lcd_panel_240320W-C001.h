#ifndef __ESP_LCD_PANEL_240320W_C001_H__
#define __ESP_LCD_PANEL_240320W_C001_H__

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

#include "driver/spi_master.h"

//列舉240320W-C001的介面
typedef enum{
    _240320WC001_INTERFACE_SPI = 0,
} _240320WC001_interface_t;

#define EXAMPLE_LCD_SPI_NUM         (SPI2_HOST)
#define EXAMPLE_LCD_PIXEL_CLK_HZ    (40 * 1000 * 1000)
#define EXAMPLE_LCD_CMD_BITS        (8)
#define EXAMPLE_LCD_PARAM_BITS      (8)

#if defined(CONFIG_IDF_TARGET_ESP32S3)
    #define EXAMPLE_LCD_GPIO_SPI_SCLK       (GPIO_NUM_12)
    #define EXAMPLE_LCD_GPIO_SPI_MOSI       (GPIO_NUM_11)
    #define EXAMPLE_LCD_GPIO_SPI_RST        (GPIO_NUM_17)//(GPIO_NUM_20)
    #define EXAMPLE_LCD_GPIO_SPI_DC         (GPIO_NUM_21)
    #define EXAMPLE_LCD_GPIO_SPI_CS         (GPIO_NUM_10)
    #define EXAMPLE_LCD_GPIO_SPI_BL         (GPIO_NUM_1)
//    #if CONFIG_LCD_QSPI_ENABLE
    #define EXAMPLE_LCD_GPIO_QSPI_IO0   EXAMPLE_LCD_GPIO_SPI_MOSI //mosi_io_num 
    #define EXAMPLE_LCD_GPIO_QSPI_IO1   (GPIO_NUM_13) //miso_io_num
    #define EXAMPLE_LCD_GPIO_QSPI_IO2   (GPIO_NUM_14) //quadwp_io_num
    #define EXAMPLE_LCD_GPIO_QSPI_IO3   (GPIO_NUM_9) //quadhd_io_num
//    #endif
#elif defined(CONFIG_IDF_TARGET_ESP32)
#define EXAMPLE_LCD_GPIO_SPI_SCLK       (GPIO_NUM_18)
#define EXAMPLE_LCD_GPIO_SPI_MOSI       (GPIO_NUM_23)
#define EXAMPLE_LCD_GPIO_SPI_RST        (GPIO_NUM_22)
#define EXAMPLE_LCD_GPIO_SPI_DC         (GPIO_NUM_21)
#define EXAMPLE_LCD_GPIO_SPI_CS         (GPIO_NUM_5)
#endif

extern esp_err_t esp_lcd_new_panel_240320WC001(const esp_lcd_panel_io_handle_t *io, const esp_lcd_panel_dev_config_t *panel_dev_config,esp_lcd_panel_handle_t *ret_panel,int interfasce,size_t interfasce_buffer_size);

#endif
