/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"

#if CONFIG_LCD_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif

#include "esp_lcd_panel_xf024qv06a.h"

#define ST7789_CMD_RAMCTRL               0xb0
#define ST7789_DATA_LITTLE_ENDIAN_BIT    (1 << 3)

static const char *TAG = "lcd_panel.st7789";

static esp_err_t panel_xf024qv06a_del(esp_lcd_panel_t *panel);
static esp_err_t panel_xf024qv06a_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_xf024qv06a_init(esp_lcd_panel_t *panel);
static esp_err_t panel_xf024qv06a_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end,
                                          const void *color_data);
static esp_err_t panel_xf024qv06a_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_xf024qv06a_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_xf024qv06a_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_xf024qv06a_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_xf024qv06a_disp_on_off(esp_lcd_panel_t *panel, bool off);
static esp_err_t panel_xf024qv06a_sleep(esp_lcd_panel_t *panel, bool sleep);

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
} xf024qv06a_panel_t;

esp_err_t 
panel_xf024qv06a_bus_QSPI_init(size_t buffer_size, const esp_lcd_panel_io_handle_t *lcd_io)
{
    esp_err_t ret = ESP_OK;
#if defined(CONFIG_IDF_TARGET_ESP32S3) 
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_GPIO_SPI_SCLK,
        .data0_io_num = EXAMPLE_LCD_GPIO_QSPI_IO0,//=data0_io_num
        .data1_io_num = EXAMPLE_LCD_GPIO_QSPI_IO1,//=data1_io_num
        .data2_io_num = EXAMPLE_LCD_GPIO_QSPI_IO2,//=data2_io_num 
        .data3_io_num = EXAMPLE_LCD_GPIO_QSPI_IO3,//=data3_io_num
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS | SPICOMMON_BUSFLAG_QUAD ,
        .max_transfer_sz = buffer_size,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_GPIO_SPI_DC,
        .cs_gpio_num = EXAMPLE_LCD_GPIO_SPI_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = 32,//EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 3,
        .flags.quad_mode = 1,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, lcd_io), err, TAG, "New panel IO failed");
#else
    ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "QSPI LCD only supported on ESP32-S3 with proper configuration and drivers.");
    //#error "QSPI LCD only supported on ESP32-S3 with proper configuration and drivers."
#endif
    return ESP_OK;

err:
    if (*lcd_io) {
        esp_lcd_panel_io_del(*lcd_io);
    }
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    return ret;
}

/*
    interfasce: 0:SPI 1:QSPI
*/
esp_err_t
esp_lcd_new_panel_xf024qv06a(const esp_lcd_panel_io_handle_t *io, const esp_lcd_panel_dev_config_t *panel_dev_config,
                         esp_lcd_panel_handle_t *ret_panel,char *interfasce_str,size_t interfasce_buffer_size)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    xf024qv06a_panel_t *LCD = NULL;
    ESP_GOTO_ON_FALSE(panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    LCD = calloc(1, sizeof(xf024qv06a_panel_t));
    ESP_GOTO_ON_FALSE(LCD, ESP_ERR_NO_MEM, err, TAG, "no mem for xf024qv06a panel");

    /* Initialize bus */
    if(strcmp(interfasce_str, "SPI") == 0)//SPI
    {
        //panel_xf024qv06a_bus_SPI_init(buffer_size, io);
        //not support yet
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "SPI not supported yet");
    }else//QSPI
    {
        panel_xf024qv06a_bus_QSPI_init(interfasce_buffer_size, io);
    }

    ESP_GOTO_ON_FALSE(*io, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");

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
    LCD->base.del = panel_xf024qv06a_del;
    LCD->base.reset = panel_xf024qv06a_reset;
    LCD->base.init = panel_xf024qv06a_init;
    LCD->base.draw_bitmap = panel_xf024qv06a_draw_bitmap;
    LCD->base.invert_color = panel_xf024qv06a_invert_color;
    LCD->base.set_gap = panel_xf024qv06a_set_gap;
    LCD->base.mirror = panel_xf024qv06a_mirror;
    LCD->base.swap_xy = panel_xf024qv06a_swap_xy;
    LCD->base.disp_on_off = panel_xf024qv06a_disp_on_off;
    LCD->base.disp_sleep = panel_xf024qv06a_sleep;
    *ret_panel = &(LCD->base);
    ESP_LOGD(TAG, "new LCD:xf024qv06a panel @%p", LCD);

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

static esp_err_t panel_xf024qv06a_del(esp_lcd_panel_t *panel)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
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
    ESP_LOGD(TAG, "del LCD:xf024qv06a panel @%p", LCD);
    free(LCD);
    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_reset(esp_lcd_panel_t *panel)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;

    // perform hardware reset
    if (LCD->reset_gpio_num >= 0) {
        gpio_set_level(LCD->reset_gpio_num, LCD->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(LCD->reset_gpio_num, !LCD->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else { // perform software reset
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (LCD_CMD_SWRESET <<8 | 0x02000000) , NULL, 0), TAG,
                            "io tx param failed");
        vTaskDelay(pdMS_TO_TICKS(20)); // spec, wait at least 5m before sending new command
    }

    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_init(esp_lcd_panel_t *panel)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;   

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x01 <<8 | 0x02000000), NULL, 0), TAG,"io tx param failed");
    vTaskDelay(pdMS_TO_TICKS(150));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xDF <<8 | 0x02000000), (uint8_t[]) {0x98,0x53}, 2), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xDE <<8 | 0x02000000), (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xB2 <<8 | 0x02000000), (uint8_t[]) {0x25}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xB7 <<8 | 0x02000000), (uint8_t[]) {0x00,0x21,0x00,0x49}, 4), TAG,"io tx param failed");//VGMP 4.3V  //VGSP 4.3V

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xBB <<8 | 0x02000000), (uint8_t[]) {0x4F,0x9A,0x55,0x73,0x63,0xF0}, 6), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xBB <<8 | 0x02000000), (uint8_t[]) {0x4F,0x9A,0x55,0x73,0x63,0xF0}, 6), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC0 <<8 | 0x02000000), (uint8_t[]) {0x44,0xA4}, 2), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC1 <<8 | 0x02000000), (uint8_t[]) {0x12}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC3 <<8 | 0x02000000), (uint8_t[]) {0x7D,0x07,0x14,0x06,0xC8,0x71,0x6C,0x77}, 8), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC4 <<8 | 0x02000000), (uint8_t[]) {0x00,0x00,0xA0,0x79,0x0E,0x0A,0x16,0x79,0x25,0x0A,0x16,0x82}, 12), TAG,"io tx param failed");//LN=320  Line//0x0E=60Hz 0x4E=50Hz 7E=45Hz

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC8 <<8 | 0x02000000), (uint8_t[]) {0x3F,0x34,0x2B,0x20,0x2A,0x2C,0x24,0x24,0x21,0x22,0x20,0x15,0x10,0x0B,0x06,0x00,0x3F,0x34,0x2B,0x20,0x2A,0x2C,0x24,0x24,0x21,0x22,0x20,0x15,0x10,0x0B,0x06,0x00}, 32), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xD0 <<8 | 0x02000000), (uint8_t[]) {0x04,0x06,0x6B,0x0F,0x00}, 5), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xD7 <<8 | 0x02000000), (uint8_t[]) {0x00,0x30}, 2), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xE6 <<8 | 0x02000000), (uint8_t[]) {0x10}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xDE <<8 | 0x02000000), (uint8_t[]) {0x01}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xB7 <<8 | 0x02000000), (uint8_t[]) {0x03,0x13,0xEF,0x35,0x35}, 5), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC1 <<8 | 0x02000000), (uint8_t[]) {0x14,0x15,0xC0}, 3), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC2 <<8 | 0x02000000), (uint8_t[]) {0x06,0x3A,0xC7}, 3), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC4 <<8 | 0x02000000), (uint8_t[]) {0x72,0x12}, 2), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xBE <<8 | 0x02000000), (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xDE <<8 | 0x02000000), (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC2 <<8 | 0x02000000), (uint8_t[]) {0x06,0x3A,0xC7}, 3), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xC4 <<8 | 0x02000000), (uint8_t[]) {0x72,0x12}, 2), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xBE <<8 | 0x02000000), (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0xDE <<8 | 0x02000000), (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x35 <<8 | 0x02000000), (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x36 <<8 | 0x02000000), (uint8_t[]) {0x00}, 1), TAG,"io tx param failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x3A <<8 | 0x02000000), (uint8_t[]) {0x05}, 1), TAG,"io tx param failed");//0x06=RGB666  0x05=RGB565

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x2A <<8 | 0x02000000), (uint8_t[]) {0x00,0x00,0x00,0xEF}, 4), TAG,"io tx param failed");//Start_X=0 //End_X=239

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x2B <<8 | 0x02000000), (uint8_t[]) {0x00,0x00,0x01,0x3F}, 4), TAG,"io tx param failed");//Start_Y=0 //End_Y=319

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x11 <<8 | 0x02000000), (uint8_t[]) {0x00}, 0), TAG,"io tx param failed");
    vTaskDelay(pdMS_TO_TICKS(120));

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (0x29 <<8 | 0x02000000), (uint8_t[]) {0x00}, 0), TAG,"io tx param failed");
    vTaskDelay(pdMS_TO_TICKS(20));

    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end,
                                          const void *color_data)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = LCD->io;

    x_start += LCD->x_gap;
    x_end += LCD->x_gap;
    y_start += LCD->y_gap;
    y_end += LCD->y_gap;

    // define an area of frame memory where MCU can access
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (LCD_CMD_CASET <<8 | 0x02000000) , (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4), TAG, "io tx param failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (LCD_CMD_RASET <<8 | 0x02000000) , (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4), TAG, "io tx param failed");
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * LCD->fb_bits_per_pixel / 8;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, (LCD_CMD_RAMWR <<8 | 0x32000000), color_data, len), TAG, "io tx color failed");

    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    int command = 0;
    if (invert_color_data) {
        command = (LCD_CMD_INVON <<8 | 0x02000000) ;
    } else {
        command = (LCD_CMD_INVOFF <<8 | 0x02000000) ;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG,
                        "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)     
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
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
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (LCD_CMD_MADCTL <<8 | 0x02000000) , (uint8_t[]) {
        LCD->madctl_val
    }, 1), TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    if (swap_axes) {
        LCD->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        LCD->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, (LCD_CMD_MADCTL <<8 | 0x02000000) , (uint8_t[]) {
        LCD->madctl_val
    }, 1), TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)     
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    LCD->x_gap = x_gap;
    LCD->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    int command = 0;
    if (on_off) {
        command = (LCD_CMD_DISPON <<8 | 0x02000000) ;
    } else {
        command = (LCD_CMD_DISPOFF <<8 | 0x02000000) ;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG,
                        "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_xf024qv06a_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    xf024qv06a_panel_t *LCD = __containerof(panel, xf024qv06a_panel_t, base);
    esp_lcd_panel_io_handle_t io = LCD->io;
    int command = 0;
    if (sleep) {
        command = (LCD_CMD_SLPIN <<8 | 0x02000000) ;
    } else {
        command = (LCD_CMD_SLPOUT <<8 | 0x02000000) ;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), TAG,
                        "io tx param failed");
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}
