/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"
#include "esp_lcd_panel_240320W-C001.h"

#if CONFIG_LCD_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif

#define ST7789_CMD_RAMCTRL               0xb0
#define ST7789_DATA_LITTLE_ENDIAN_BIT    (1 << 3)

static const char *TAG = "lcd_panel.st7789";

static esp_err_t panel_240320WC001_del(esp_lcd_panel_t *panel);
static esp_err_t panel_240320WC001_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_240320WC001_init(esp_lcd_panel_t *panel);
static esp_err_t panel_240320WC001_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end,
                                          const void *color_data);
static esp_err_t panel_240320WC001_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_240320WC001_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_240320WC001_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_240320WC001_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_240320WC001_disp_on_off(esp_lcd_panel_t *panel, bool off);
static esp_err_t panel_240320WC001_sleep(esp_lcd_panel_t *panel, bool sleep);

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t madctl_val;    // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_val;    // save current value of LCD_CMD_COLMOD register
    uint8_t ramctl_val_1;
    uint8_t ramctl_val_2;
} _240320WC001_panel_t;

esp_err_t panel_240320WC001_bus_SPI_init(size_t buffer_size, const esp_lcd_panel_io_handle_t *lcd_io)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_GPIO_SPI_SCLK,
        .mosi_io_num = EXAMPLE_LCD_GPIO_SPI_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = buffer_size,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_GPIO_SPI_DC,
        .cs_gpio_num = EXAMPLE_LCD_GPIO_SPI_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 3,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, lcd_io), err, TAG, "New panel IO failed");

    return ESP_OK;
err:
    if (*lcd_io) {
        esp_lcd_panel_io_del(*lcd_io);
    }
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    return ret;
}

esp_err_t
esp_lcd_new_panel_240320WC001(const esp_lcd_panel_io_handle_t *io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                         esp_lcd_panel_handle_t *ret_panel,int interfasce,size_t interfasce_buffer_size)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    _240320WC001_panel_t *LCD = NULL;
    ESP_GOTO_ON_FALSE(panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    LCD = calloc(1, sizeof(_240320WC001_panel_t));
    ESP_GOTO_ON_FALSE(LCD, ESP_ERR_NO_MEM, err, TAG, "no mem for _240320WC001 panel");

    if(interfasce == 0)//SPI    
    {
        panel_240320WC001_bus_SPI_init(interfasce_buffer_size, io);
    }
    else 
    {
        //not support yet
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "interfasce not supported yet");
    }

    ESP_GOTO_ON_FALSE(*io , ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->rgb_endian) {
    case LCD_RGB_ENDIAN_RGB:
        LCD->madctl_val = 0;
        break;
    case LCD_RGB_ENDIAN_BGR:
        LCD->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    uint8_t fb_bits_per_pixel = 0;
    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        LCD->colmod_val = 0x55;
        fb_bits_per_pixel = 16;
        break;
    case 18: // RGB666
        LCD->colmod_val = 0x66;
        // each color component (R/G/B) should occupy the 6 high bits of a byte, which means 3 full bytes are required for a pixel
        fb_bits_per_pixel = 24;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    LCD->ramctl_val_1 = 0x00;
    LCD->ramctl_val_2 = 0xf0;    // Use big endian by default
    if ((panel_dev_config->data_endian) == LCD_RGB_DATA_ENDIAN_LITTLE) {
        // Use little endian
        LCD->ramctl_val_2 |= ST7789_DATA_LITTLE_ENDIAN_BIT;
    }

    LCD->io = *io;
    LCD->fb_bits_per_pixel = fb_bits_per_pixel;
    LCD->reset_gpio_num = panel_dev_config->reset_gpio_num;
    LCD->reset_level = panel_dev_config->flags.reset_active_high;
    LCD->base.del = panel_240320WC001_del;
    LCD->base.reset = panel_240320WC001_reset;
    LCD->base.init = panel_240320WC001_init;
    LCD->base.draw_bitmap = panel_240320WC001_draw_bitmap;
    LCD->base.invert_color = panel_240320WC001_invert_color;
    LCD->base.set_gap = panel_240320WC001_set_gap;
    LCD->base.mirror = panel_240320WC001_mirror;
    LCD->base.swap_xy = panel_240320WC001_swap_xy;
    LCD->base.disp_on_off = panel_240320WC001_disp_on_off;
    LCD->base.disp_sleep = panel_240320WC001_sleep;
    *ret_panel = &(LCD->base);
    ESP_LOGD(TAG, "new LCD:240320WC001 panel @%p", LCD);

    return ESP_OK;

err:
    if (LCD) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(LCD);  
    }
    return ret;
}

static esp_err_t panel_240320WC001_del(esp_lcd_panel_t *panel)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
//***************** 20250704  新增 *****************
    esp_lcd_panel_io_handle_t io = LCD->io;

    if (io) {
        esp_lcd_panel_io_del(io);
    }
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
//***************** 20250704  新增 *****************

    if (LCD->reset_gpio_num >= 0) {
        gpio_reset_pin(LCD->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del LCD:240320WC001 panel @%p", LCD);
    free(LCD);
    return ESP_OK;
}

static esp_err_t panel_240320WC001_reset(esp_lcd_panel_t *panel)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;

    // perform hardware reset
    if (LCD->reset_gpio_num >= 0) {
        gpio_set_level(LCD->reset_gpio_num, LCD->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(LCD->reset_gpio_num, !LCD->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else { // perform software reset
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), TAG,
                            "io tx param failed");
        vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5m before sending new command
    }

    return ESP_OK;
}

static esp_err_t panel_240320WC001_init(esp_lcd_panel_t *panel)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x01, NULL, 0), TAG,"io tx param failed");//Software Reset
    vTaskDelay(pdMS_TO_TICKS(150));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x11, NULL, 0), TAG,"io tx param failed");
	vTaskDelay(pdMS_TO_TICKS(150));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x36, (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x3A, (uint8_t[]) {0x05}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xB2, (uint8_t[]) {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xBB, (uint8_t[]) {0x1f}, 1), TAG,"io tx param failed");//VCOM 

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC0, (uint8_t[]) {0x2C}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC2, (uint8_t[]) {0x01}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC3, (uint8_t[]) {0x19}, 1), TAG,"io tx param failed");//GVDD //4.3V 

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC4, (uint8_t[]) {0x20}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xC6, (uint8_t[]) {0x0F}, 1), TAG,"io tx param failed");   //Dot INV, 60Hz  

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xD0, (uint8_t[]) {0xA4, 0xA1}, 2), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xE0, (uint8_t[]) {0xD0, 0x06, 0x0C, 0x0A, 0x09, 0x0A, 0x32, 0x33, 0x49, 0x19, 0x14, 0x15, 0x2B, 0x34}, 14), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0xE1, (uint8_t[]) {0xD0, 0x06, 0x0C, 0x0A, 0x09, 0x11, 0x37, 0x33, 0x49, 0x19, 0x14, 0x15, 0x2D, 0x34}, 14), TAG,"io tx param failed");  

	ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x21, NULL, 0), TAG,"io tx param failed");     

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x29, NULL, 0), TAG,"io tx param failed");     
	vTaskDelay(pdMS_TO_TICKS(120));            

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x3A, (uint8_t[]) {0x55}, 1), TAG,"io tx param failed");//Interface Pixel Format
	vTaskDelay(pdMS_TO_TICKS(10));
	
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x36, (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");//Memory Data Access Control

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x2A, (uint8_t[]) {0x00,0x00,0x00,0xF0}, 4), TAG,"io tx param failed");//Column Address Set

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x2B, (uint8_t[]) {0x00,0x00,0x00,0xF0}, 4), TAG,"io tx param failed");//Row Address Set

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x21, NULL, 0), TAG,"io tx param failed");//Display Inversion On
	vTaskDelay(pdMS_TO_TICKS(10));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x13, NULL, 0), TAG,"io tx param failed");//Normal Display Mode On
	vTaskDelay(pdMS_TO_TICKS(10));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x29, NULL, 0), TAG,"io tx param failed");//Display ON
	vTaskDelay(pdMS_TO_TICKS(255));

    return ESP_OK;
}

static esp_err_t panel_240320WC001_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end,
                                          const void *color_data)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = LCD->io;

    x_start += LCD->x_gap;
    x_end += LCD->x_gap;
    y_start += LCD->y_gap;
    y_end += LCD->y_gap;

    // define an area of frame memory where MCU can access
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4), TAG, "io tx param failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4), TAG, "io tx param failed");
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * LCD->fb_bits_per_pixel / 8;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len), TAG, "io tx color failed");

    return ESP_OK;
}

static esp_err_t panel_240320WC001_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    int command = 0;
    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG,
                        "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_240320WC001_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    if (mirror_x) {
        LCD->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        LCD->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        LCD->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        LCD->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        LCD->madctl_val
    }, 1), TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_240320WC001_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    if (swap_axes) {
        LCD->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        LCD->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        LCD->madctl_val
    }, 1), TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_240320WC001_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)     
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    LCD->x_gap = x_gap;
    LCD->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_240320WC001_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    int command = 0;
    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG,
                        "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_240320WC001_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    _240320WC001_panel_t *LCD = __containerof(panel, _240320WC001_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    int command = 0;
    if (sleep) {
        command = LCD_CMD_SLPIN;
    } else {
        command = LCD_CMD_SLPOUT;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG,
                        "io tx param failed");
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}
